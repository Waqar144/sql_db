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
	}
}

void Table::leafNodeInsert(Cursor *c, uint32_t key, Row *value) {
	char *node = pager->getPage(c->pageNum);
	uint32_t numCells = *leaf_node_num_cells(node);
	if (numCells >= LEAF_NODE_MAX_CELLS) {
		std::cout << "Node splitting not implemented.\n";
		exit(EXIT_FAILURE);
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