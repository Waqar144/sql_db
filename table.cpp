#include <iostream>

#include "table.hpp"
#include "pager.hpp"
#include "node.hpp"
#include "cursor.hpp"

Table::Table() {
    pager = new Pager;
}

Table::~Table() {
    delete pager;
}

void Table::dbOpen(std::string filename) {
    pager->_open(filename);
    rootPageNum = 0;
    if (pager->getNumOfPages() == 0) {
        char *rootNode = pager->getPage(0);
        initialize_leaf_node(rootNode);
		set_node_root(rootNode, true);
    }
}

Cursor* Table::tableStart() {
	Cursor *c = new Cursor;
	c->table = this;
	c->pageNum = rootPageNum;
	c->cellNum = 0;

	char *node = pager->getPage(rootPageNum);
	uint32_t numCells = *leaf_node_num_cells(node);

	c->endOfTable = (numCells == 0);
	return c;
}

Cursor* Table::tableFind(uint32_t key) {
	char *rootNode = pager->getPage(rootPageNum);

	if (get_node_type(rootNode) == NodeType::NodeLeaf) {
		return leafNodeFind(rootPageNum, key);
	} else {
		std::cout << "Searching internal nodes not implemented yet.\n";
		exit(EXIT_FAILURE);
	}
}

void Table::createNewRoot(uint32_t rightChildPageNum) {
	char *root = pager->getPage(rootPageNum);
	char *rightChild = pager->getPage(rightChildPageNum);
	uint32_t leftChildPageNum = pager->getUnusedPageNum();
	char *leftChild = pager->getPage(leftChildPageNum);

	memcpy(leftChild, root, PAGE_SIZE);
	set_node_root(leftChild, false);

	initialize_internal_node(root);
	set_node_root(root, true);
	*internal_node_num_keys(root) = 1;
	*internal_node_child(root, 0) = leftChildPageNum;
	uint32_t leftChildMaxKey = get_node_max_key(leftChild);
	*internal_node_key(root, 0) = leftChildMaxKey;
	*internal_node_right_child(root) = rightChildPageNum;

}

void Table::leafNodeInsert(Cursor *c, uint32_t key, Row *value) {
	char *node = pager->getPage(c->pageNum);
	uint32_t numCells = *leaf_node_num_cells(node);
	//the node is full
	if (numCells >= LEAF_NODE_MAX_CELLS) {
		//split and insert
		leafNodeSplitAndInsert(c, key, value);
		return;
	}

	if (c->cellNum < numCells) {
		for (uint32_t i = 0; i > c->cellNum; --i) {
			memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
		}
	}

	*(leaf_node_num_cells(node)) += 1;
	*(leaf_node_key(node, c->cellNum)) = key;
	value->serialize(leaf_node_value(node, c->cellNum));
}

/**
 * @brief split a leaf node into two and insert
 * @details If there is no space on the leaf node, we would split the existing
 * entries residing there and the new one (being inserted) into two equal halves:
 * lower and upper halves. (Keys on the upper half are strictly greater than those
 * on the lower half.) We allocate a new leaf node, and move the upper half into the
 * new node.
 */
void Table::leafNodeSplitAndInsert(Cursor *c, uint32_t key, Row *value) {
  	/*
  	Create a new node and move half the cells over.
  	Insert the new value in one of the two nodes.
  	Update parent or create a new parent.
 	*/
 	char *oldNode = pager->getPage(c->pageNum);
	uint32_t newPageNum = pager->getUnusedPageNum();
	char *newNode = pager->getPage(newPageNum);
	initialize_leaf_node(newNode);
  	/*
  	All existing keys plus new key should be divided
  	evenly between old (left) and new (right) nodes.
  	Starting from the right, move each key to correct position.
  	*/
  	for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; --i) {
		  char *destNode;
		  if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
			  destNode = newNode;
		} else {
			destNode = oldNode;
		}
		uint32_t indexWithinNode = i % LEAF_NODE_LEFT_SPLIT_COUNT;
		char *dest = leaf_node_cell(destNode, indexWithinNode);

		if (i == c->cellNum) {
			value->serialize(dest);
		} else if (i > c->cellNum) {
			memcpy(dest, leaf_node_cell(oldNode, i - 1), LEAF_NODE_CELL_SIZE);
		} else {
			memcpy(dest, leaf_node_cell(oldNode, i), LEAF_NODE_CELL_SIZE);
		}
	}

	//update cell count on both leaf nodes
	*(leaf_node_num_cells(oldNode)) = LEAF_NODE_LEFT_SPLIT_COUNT;
	*(leaf_node_num_cells(newNode)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

	if (is_node_root(oldNode)) {
		return createNewRoot(newPageNum);
	} else {
		std::cout << "Need to implement updating parent after splitting\n";
		exit(EXIT_FAILURE);
	}
}

Cursor* Table::leafNodeFind(uint32_t pageNum, uint32_t key) {
	char *node = pager->getPage(pageNum);
	uint32_t numOfCells = *leaf_node_num_cells(node);

	Cursor *c = new Cursor;
	c->table = this;
	c->pageNum = pageNum;

	//Binary search
	uint32_t minIndex = 0;
	uint32_t onePastMaxIndex = numOfCells;
	while (onePastMaxIndex != minIndex) {
		uint32_t index = (onePastMaxIndex + minIndex) / 2;
		uint32_t keyAtIndex = *(leaf_node_key(node, index));
		if (key == keyAtIndex) {
			c->cellNum = index;
			return c;
		}
		if (key < keyAtIndex) {
			onePastMaxIndex = index;
		} else {
			minIndex = index + 1;
		}
	}

	c->cellNum = minIndex;
	return c;
}


void Table::dbClose() {
	std::cout << pager->getNumOfPages();
	for (uint32_t i = 0; i < pager->getNumOfPages(); ++i) {
		if (pager->getPage(i) == nullptr)
			continue;
		pager->_flush(i);
	}

	int result = pager->_close();
	if (result == -1) {
		std::cout << "Error closing file. Exiting..." << std::endl;
		exit(EXIT_FAILURE);
	}
}

void indent(uint32_t level) {
	for (uint32_t i = 0; i < level; i++) {
		printf("  ");
	}
}

void Table::print(uint32_t page, uint32_t indentationLevel) {
	char *node = pager->getPage(page);
	uint32_t numOfKeys{},
			 child{};
	switch(get_node_type(node)) {
		case NodeType::NodeInternal:
			numOfKeys = *leaf_node_num_cells(node);
			indent(indentationLevel);
			printf("- leaf (size %d)\n", numOfKeys);
			for (uint32_t i = 0; i < numOfKeys; i++) {
				indent(indentationLevel + 1);
				printf("- %d\n", *leaf_node_key(node, i));
			}
		break;
		case NodeType::NodeLeaf:
			numOfKeys = *internal_node_num_keys(node);
			indent(indentationLevel);
			printf("- internal (size %d)\n", numOfKeys);
			for (uint32_t i = 0; i < numOfKeys; i++) {
				child = *internal_node_child(node, i);
				print(child, indentationLevel + 1);

				indent(indentationLevel + 1);
				printf("- key %d\n", *internal_node_key(node, i));
			}
			child = *internal_node_right_child(node);
			print(child, indentationLevel + 1);
		break;
	}
}