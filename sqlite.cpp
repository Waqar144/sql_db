#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "pager.hpp"

// static constexpr uint32_t PAGE_SIZE = 4096;
// static constexpr uint32_t MAX_PAGES = 100;

enum MetaCommandResult {
	CommandSuccess,
	CommandUnrecognized
};

enum StatementType {
	Insert,
	Select,
	Delete,
	Update
};

enum PrepareResult {
	PrepareSuccess,
	PrepareUnrecognized,
	PrepareSyntaxError,
	PrepareStringTooLong,
	PrepareNegativeId
};

enum ExecuteResult {
	ExecuteSucess,
	ExecuteTableFull,
	ExecuteDuplicateKey
};



struct Row {
	uint32_t id;
	char username[32];
	char email[64];
	static constexpr size_t ID_SIZE = sizeof(id);
	static constexpr size_t USERNAME_SIZE = sizeof(username);
	static constexpr size_t EMAIL_SIZE = sizeof(email);
	static constexpr uint32_t ID_OFFSET = 0;
	static constexpr uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
	static constexpr uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
	static constexpr uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

	void print() {
		std::cout << "(" << id << ", "<< username << ", " << email << ")\n";
	}

	void serialize(char *dest) {
		memcpy(dest + ID_OFFSET, &(id), ID_SIZE);
		memcpy(dest + USERNAME_OFFSET, &(username), USERNAME_SIZE);
		memcpy(dest + EMAIL_OFFSET, &(email), EMAIL_SIZE);
	}

	void deserialize(char *src) {
		memcpy(&(id), src + ID_OFFSET, ID_SIZE);
		memcpy(&(username), src + USERNAME_OFFSET, USERNAME_SIZE);
		memcpy(&(email), src + EMAIL_OFFSET, EMAIL_SIZE);
	}

	static constexpr uint32_t rowSize() {
		return ROW_SIZE;
	}
};

/***************
 * NODE DATA
 * ************/

enum NodeType {
	NodeInternal,
	NodeLeaf
};

/*
 * Common Node Header Layout
 */
constexpr uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
constexpr uint32_t NODE_TYPE_OFFSET = 0;
constexpr uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
constexpr uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
constexpr uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
constexpr uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
constexpr uint8_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

/**
 * Leaf Node Header Layout
 */
constexpr uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

/*
 * Leaf Node Body Layout
 */
constexpr uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_KEY_OFFSET = 0;
constexpr uint32_t LEAF_NODE_VALUE_SIZE = Row::ROW_SIZE;
constexpr uint32_t LEAF_NODE_VALUE_OFFSET =
    LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
constexpr uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
constexpr uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
constexpr uint32_t LEAF_NODE_MAX_CELLS =
    LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

uint32_t* leaf_node_num_cells(char* node) {
  return (uint32_t*)(node + LEAF_NODE_NUM_CELLS_OFFSET);
}

char* leaf_node_cell(char* node, uint32_t cell_num) {
  return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(char* node, uint32_t cell_num) {
  return (uint32_t*)leaf_node_cell(node, cell_num);
}

char* leaf_node_value(char* node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

NodeType get_node_type(char *node) {
	uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
	return (NodeType)value;
}

void set_node_type(char *node, NodeType type) {
	uint8_t value = type;
	*((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

void initialize_leaf_node(char* node) {
	set_node_type(node, NodeType::NodeLeaf);
	*leaf_node_num_cells(node) = 0;
}

void print_constants() {
	std::cout << "ROW_SIZE: " << Row::ROW_SIZE << std::endl;
	std::cout << "COMMON_NODE_HEADER_SIZE: " << COMMON_NODE_HEADER_SIZE << std::endl;
	std::cout << "LEAF_NODE_HEADER_SIZE: " << LEAF_NODE_HEADER_SIZE << std::endl;
	std::cout << "LEAF_NODE_CELL_SIZE: " << LEAF_NODE_CELL_SIZE << std::endl;
	std::cout << "LEAF_NODE_SPACE_FOR_CELLS: " << LEAF_NODE_SPACE_FOR_CELLS << std::endl;
	std::cout << "LEAF_NODE_MAX_CELLS: " << LEAF_NODE_MAX_CELLS << std::endl;
}

void print_leaf_node(char *node) {
	uint32_t numCells = *(leaf_node_num_cells(node));
	std::cout<<"leaf (size " << numCells << ")" << std::endl;
	for (uint32_t i = 0; i < numCells; ++i) {
		uint32_t key = *leaf_node_key(node, i);
		std::cout<<"  - " << i << " : " << key << std::endl;
	}
}

//Forward declaration
class Table;

/***************
 CURSOR CLASS
***************/
struct Cursor {
	uint32_t pageNum;
	uint32_t cellNum;
	bool endOfTable;
	Table *table;

	char *value();
	void advance();
};

/************
 TABLE CLASS
************/
class Table {
	uint32_t numRows;
	uint32_t rootPageNum;
	Pager *pager;

public:
	Table() {
		pager = new Pager;
	}
	~Table() {
		delete pager;
	}

	void dbOpen(std::string filename) {
		pager->_open(filename);
		rootPageNum = 0;
		if (pager->getNumOfPages() == 0) {
			char *rootNode = pager->getPage(0);
			initialize_leaf_node(rootNode);
		}
	}

	inline uint32_t getNumRows() {
		return numRows; 
	}

	inline Pager* getPager() {
		return pager;
	}

	inline uint32_t getRootPageNum() {
		return rootPageNum;
	}

	Cursor *tableStart() {
		Cursor *c = new Cursor;
		c->table = this;
		c->pageNum = rootPageNum;
		c->cellNum = 0;

		char *node = pager->getPage(rootPageNum);
		uint32_t numCells = *leaf_node_num_cells(node);

		c->endOfTable = (numCells == 0);
		return c;
	}

	//Return the position of a given key.
	//In case the key is not found, return the
	//position where it should be inserted
	Cursor *tableFind(uint32_t key) {
		char *rootNode = pager->getPage(rootPageNum);

		if (get_node_type(rootNode) == NodeType::NodeLeaf) {
			return leafNodeFind(rootPageNum, key);
		} else {
			std::cout << "Searching internal nodes not implemented yet.\n";
		}
	}

	void leafNodeInsert(Cursor *c, uint32_t key, Row *value) {
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

	Cursor *leafNodeFind(uint32_t pageNum, uint32_t key) {
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

	void dbClose() {
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

	constexpr uint32_t rows() const {
		return numRows;
	}

	void incrementRowNum() {
		numRows += 1;
	}

};


char* Cursor::value(){
	char *page = table->getPager()->getPage(pageNum);
	return leaf_node_value(page, cellNum);
}
void Cursor::advance() {
	char *node = table->getPager()->getPage(pageNum);
	cellNum++;
	if (cellNum >= (*leaf_node_num_cells(node))) {
		endOfTable = true;
	}
}

/**************
GLOBAL FUNCTIONS
**************/

inline static void printPrompt() {
	std::cout << "db > ";
}

MetaCommandResult runCommand(std::string input, Table *t) {
	if (input == ".exit") {
		t->dbClose();
		delete t;
		exit(EXIT_SUCCESS);
	} else if(input == ".constants") {
		print_constants();
		return MetaCommandResult::CommandSuccess;
	} else if (input == ".btree") {
		std::cout<<"Tree: " << std::endl;
		print_leaf_node(t->getPager()->getPage(0));
		return MetaCommandResult::CommandSuccess;
	} else {
		return MetaCommandResult::CommandUnrecognized;
	}
}

class Statement {
	StatementType type;
	Row rowToInsert;
public:
	Statement() {}

	PrepareResult prepareInsert(std::string str) {
		std::stringstream ss(str);
		std::string token;
		std::vector<std::string> tokens;
		tokens.reserve(4);
		std::string keyword, id, username, email;
		while (getline(ss, token, ' ')) {
			tokens.push_back(token);
		}
		if (tokens.size() != 4)
			return PrepareSyntaxError;

        std::string cpp;
		try {
			rowToInsert.id = stoi(tokens[1]);
		} catch(...) {
			return PrepareSyntaxError;
		}

		if (stoi(tokens[1]) < 0) {
			return PrepareNegativeId;
		}

		if (tokens[2].length() < 32) {
			strcpy(rowToInsert.username, tokens[2].c_str());
		} else {
			return PrepareStringTooLong;
		}

		if (tokens[3].length() < 64) {
			strcpy(rowToInsert.email, tokens[3].c_str());
		} else {
			return PrepareStringTooLong;
		}

		return PrepareSuccess;
	}

	PrepareResult prepareStatement(std::string st) {
		if (strncmp(st.c_str(), "insert", 6) == 0) {
			type = Insert;
			return prepareInsert(st);
		}
		if (st == "select") {
			type = Select;
			return PrepareSuccess;
		}

		return PrepareUnrecognized;
	}

	ExecuteResult executeInsert(Table *t) {
		char *node = t->getPager()->getPage(t->getRootPageNum());
		uint32_t numOfCells = *(leaf_node_num_cells(node));
		if (numOfCells >= LEAF_NODE_MAX_CELLS) {
			return ExecuteTableFull;
		}

		uint32_t keyToInsert = rowToInsert.id;
		Cursor *c = t->tableFind(keyToInsert);

		if (c->cellNum < numOfCells) {
			uint32_t keyAtIndex = *leaf_node_key(node, c->cellNum);
			if (keyAtIndex == keyToInsert) {
				return ExecuteDuplicateKey;
			}
		}

		t->leafNodeInsert(c, rowToInsert.id, &rowToInsert);
		delete c;

		return ExecuteSucess;
	}

	ExecuteResult executeSelect(Table *t) {
		Row r;
		Cursor *c = t->tableStart();
		for (int i = 0; i < t->rows(); ++i) {
			r.deserialize(c->value());
			r.print();
			c->advance();
		}
		delete c;
		return ExecuteSucess;
	}

	ExecuteResult executeStatement(Table *t) {
		switch(type) {
			case Insert:
				return executeInsert(t);
			case Select:
				return executeSelect(t);
		}
	}

};

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "2nd arg not provided.\n";
		return 1;
	}
	std::string input;
	Table *table = new Table;
	table->dbOpen(argv[1]);

	while(true) {
		printPrompt();
		getline(std::cin, input);

		if (input[0] == '.') {
			switch(runCommand(input, table)) {
				case MetaCommandResult::CommandSuccess:
				break;
				case MetaCommandResult::CommandUnrecognized:
				std::cout << "Unrecognized command!\n";
                continue;
			}
		}

		Statement st;
		auto prepareResult = st.prepareStatement(input);
		switch(prepareResult) {
			case PrepareSuccess:
				break;
			case PrepareSyntaxError:
				std::cout << "Syntax error. Could not parse statement.\n";
                continue;
			case PrepareStringTooLong:
				std::cout << "String too long\n";
                continue;
			case PrepareNegativeId:
				std::cout << "Negative Id not allowed!\n";
				continue;
			case PrepareUnrecognized:
				std::cout << "Unrecognized Statement in " << input << std::endl;
				continue;
		}

		switch(st.executeStatement(table)) {
			case ExecuteSucess:
				std::cout << "Executed\n";
				break;
			case ExecuteDuplicateKey:
				std::cout << "Duplicate keys not allowed\n";
				break;
			case ExecuteTableFull:
				break;
		}
	}

	return 0;
}
