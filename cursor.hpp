#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>
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

#endif