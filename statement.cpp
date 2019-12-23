#include <vector>
#include <string>
#include <sstream>
#include <cstring>

#include "statement.hpp"
#include "table.hpp"
#include "node.hpp"
#include "cursor.hpp"

PrepareResult Statement::prepareInsert(std::string str) {
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

PrepareResult Statement::prepareStatement(std::string st) {
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

ExecuteResult Statement::executeInsert(Table *t) {
	uint32_t keyToInsert = rowToInsert.id;
	Cursor *c = t->tableFind(keyToInsert);

	char *node = t->getPager()->getPage(c->pageNum);
	uint32_t numOfCells = *(leaf_node_num_cells(node));

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

ExecuteResult Statement::executeSelect(Table *t) {
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

ExecuteResult Statement::executeStatement(Table *t) {
	switch(type) {
		case Insert:
			return executeInsert(t);
		case Select:
			return executeSelect(t);
	}
}