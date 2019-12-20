#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

static constexpr uint32_t PAGE_SIZE = 4096;
static constexpr uint32_t MAX_PAGES = 100;

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
	ExecuteTableFull
};

/*********
 PAGER CLASS
 Pager class contains the memory we read/write to.
 We request the pager to give us a page(size: 4096 bytes)
 and it returns that page. It will first look in the cache,
 if it doesn't find the page there, it will get that page
 from the disk
*********/
class Pager{
	int fileDescriptor;
	uint32_t fileLength;
	char *pages[MAX_PAGES];

public:
	Pager() {
		for (int i=0; i < MAX_PAGES; ++i) {
			pages[i] = nullptr;
		}
		fileLength = 0;
	}

	~Pager() {
		for (int i = 0; i < MAX_PAGES; ++i) {
			if (pages[i]) {
				delete[] pages[i];
				pages[i] = nullptr;
			}
		}
	}

	void _open(std::string filename) {
		fileDescriptor = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fileDescriptor == -1) {
			std::cout << "Unable to open file\n";
		}

		fileLength= lseek(fileDescriptor, 0, SEEK_END);
	}

	//return the file length
	inline uint32_t getFileLength() {
		return fileLength;
	}

	/**
	 * @brief returns the page at pageNum
	 */
	char *getPage(uint32_t pageNum) {
		if (pageNum > MAX_PAGES) {
			std::cout << "This page number is out of bounds.\n";
		}

		char *page = nullptr;
		if (pages[pageNum] == nullptr) {
			page = new char[PAGE_SIZE];
			uint32_t numOfPages = fileLength / PAGE_SIZE;

			//incomplete page
			if(fileLength % PAGE_SIZE != 0) {
				numOfPages += 1;
			}

			if (pageNum <= numOfPages) {
				lseek(fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);
				ssize_t numOfBytesRead = read(fileDescriptor, page, PAGE_SIZE);
				if (numOfBytesRead == -1) {
					std::cout << "Error reading file\n";
					exit(EXIT_FAILURE);
				}
			}
			pages[pageNum] = page;
		}
		return pages[pageNum];
	}

	void _flush(uint32_t pageNum, uint32_t size) {
		if (pages[pageNum] == nullptr) {
			std::cout << "Tried to flush null page. Exiting..." << std::endl;
			exit(EXIT_FAILURE);
		}

		off_t offset = lseek(fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);
		if (offset == -1) {
			std::cout << "Error seeking file. Exiting... \n";
			exit(EXIT_FAILURE);
		}

		ssize_t numOfBytesWritten = write(fileDescriptor, pages[pageNum], size);
		if (numOfBytesWritten == -1) {
			std::cout << "Error writing to file. Exiting...\n";
			exit(EXIT_FAILURE);
		}
	}

	int _close() {
		return close(fileDescriptor);
	}
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

//Forward declaration
class Table;

/***************
 CURSOR CLASS
***************/
struct Cursor {
	uint32_t rowNum;
	bool endOfTable;
	Table *table;

	char *value();
	void advance();
};

/************
 TABLE CLASS
************/
class Table {
	static constexpr uint32_t ROWS_PER_PAGE = PAGE_SIZE / Row::rowSize();
	uint32_t numRows;
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
		numRows = pager->getFileLength() / Row::rowSize();
	}

	inline uint32_t getNumRows() {
		return numRows; 
	}

	inline uint32_t getRowsPerPage() {
		return ROWS_PER_PAGE;
	}

	inline Pager* getPager() {
		return pager;
	}

	Cursor *tableStart() {
		Cursor *c = new Cursor;
		c->table = this;
		c->rowNum = 0;
		c->endOfTable = (numRows == 0);
		return c;
	}

	Cursor *tableEnd() {
		Cursor *c = new Cursor;
		c->table = this;
		c->rowNum = numRows;
		c->endOfTable = true;
		return c;
	}

	void dbClose() {
		const uint32_t numOfFullPages = numRows / ROWS_PER_PAGE;
		for (uint32_t i = 0; i < numOfFullPages; ++i) {
			if (pager->getPage(i) == nullptr)
				continue;
			pager->_flush(i, PAGE_SIZE);
		}

		uint32_t numOFAdditionalRows = numRows % ROWS_PER_PAGE;
		if (numOFAdditionalRows > 0) {
     		uint32_t pageNum = numOfFullPages;
     		if (pager->getPage(pageNum) != nullptr) {
         		pager->_flush(pageNum, numOFAdditionalRows * Row::rowSize());
			}
		}

		int result = pager->_close();
		if (result == -1) {
			std::cout << "Error closing file. Exiting..." << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	char *slot(const uint32_t rowNum) {
		uint32_t pageNum = rowNum / ROWS_PER_PAGE;
		char *page = pager->getPage(pageNum);
		if (page == nullptr) {
			std::cout << "[Table::slot()] Failed to get page -- Unknown error" << std::endl;
		}
		uint32_t rowOffset = rowNum % ROWS_PER_PAGE;
		uint32_t byteOffset = rowOffset * Row::rowSize();
		return page + byteOffset;
	}

	constexpr uint32_t rows() const {
		return numRows;
	}

	void incrementRowNum() {
		numRows += 1;
	}

	static constexpr uint32_t maxRows() {
		return ROWS_PER_PAGE * MAX_PAGES;
	}
};


char* Cursor::value(){
		uint32_t pageNum = rowNum / table->getRowsPerPage();
	char *page = table->getPager()->getPage(pageNum);
	uint32_t rowOffset = rowNum % table->getRowsPerPage();
	uint32_t byteOffset = rowOffset * Row::rowSize();
	return page + byteOffset;
}
void Cursor::advance() {
		++rowNum;
		if (rowNum >= table->getNumRows()) {
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
		if (t->rows() >= t->maxRows())
			return ExecuteTableFull;

		Cursor *c = t->tableEnd();
		rowToInsert.serialize(c->value());
//		row->serialize(t->slot(t->rows()));
		t->incrementRowNum();
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
				continue;;
			}
		}

		Statement st;
		auto prepareResult = st.prepareStatement(input);
		switch(prepareResult) {
			case PrepareSuccess:
				break;
			case PrepareSyntaxError:
				std::cout << "Syntax error. Could not parse statement.\n";
				continue;;
			case PrepareStringTooLong:
				std::cout << "String too long\n";
				continue;;
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
			case ExecuteTableFull:
				break;
		}
	}

	return 0;
}