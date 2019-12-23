#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include <string>

class Pager;
struct Cursor;
struct Row;

class Table {
	uint32_t numRows;
	uint32_t rootPageNum;
	Pager *pager;

public:
    Table();
    ~Table();

	void dbOpen(std::string filename);

	inline uint32_t getNumRows() {
		return numRows; 
	}

	inline Pager* getPager() {
		return pager;
	}

	inline uint32_t getRootPageNum() {
		return rootPageNum;
	}

	Cursor *tableStart();

	//Return the position of a given key.
	//In case the key is not found, return the
	//position where it should be inserted
	Cursor *tableFind(uint32_t key);

	void leafNodeInsert(Cursor *c, uint32_t key, Row *value);

	Cursor *leafNodeFind(uint32_t pageNum, uint32_t key);

	void dbClose();

	inline constexpr uint32_t rows() const {
		return numRows;
	}

	inline void incrementRowNum() {
		numRows += 1;
	}

};

#endif