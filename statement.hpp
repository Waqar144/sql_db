#ifndef STATEMENT_H
#define STATEMENT_H

#include <string>
#include "row.hpp"

class Table;

enum ExecuteResult {
	ExecuteSucess,
	ExecuteTableFull,
	ExecuteDuplicateKey
};

enum PrepareResult {
	PrepareSuccess,
	PrepareUnrecognized,
	PrepareSyntaxError,
	PrepareStringTooLong,
	PrepareNegativeId
};


enum StatementType {
	Insert,
	Select,
	Delete,
	Update
};

class Statement {
	StatementType type;
	Row rowToInsert;
public:
	Statement() {}

	PrepareResult prepareInsert(std::string str);

	PrepareResult prepareStatement(std::string st);

	ExecuteResult executeInsert(Table *t);

	ExecuteResult executeSelect(Table *t);

	ExecuteResult executeStatement(Table *t);

};

#endif