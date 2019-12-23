#include <iostream>
#include <string>
#include <cstring>

#include "pager.hpp"
#include "table.hpp"
#include "node.hpp"
#include "row.hpp"
#include "statement.hpp"

enum MetaCommandResult {
	CommandSuccess,
	CommandUnrecognized
};



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
