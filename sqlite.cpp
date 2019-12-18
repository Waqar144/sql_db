#include <iostream>
#include <string>
#include <cstring>

// class InputBuffer {
// 	std::string buffer;
// 	size_t bufferLength;
// 	ssize_t inputLength;

// public:
// 	InputBuffer() : buffer(""), bufferLength(0), inputLength(0)
// 	{
// 	}

// 	operator >>
// };
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
	PrepareSyntaxError
};

enum ExecuteResult {
	ExecuteSucess,
	ExecuteTableFull
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

	void serialize(void *dest) {
		memcpy(dest + ID_OFFSET, &(id), ID_SIZE);
		memcpy(dest + USERNAME_OFFSET, &(username), USERNAME_SIZE);
		memcpy(dest + EMAIL_OFFSET, &(email), EMAIL_SIZE);
	}

	void deserialize(void *src) {
		memcpy(&(id), src + ID_OFFSET, ID_SIZE);
		memcpy(&(username), src + USERNAME_OFFSET, USERNAME_SIZE);
		memcpy(&(email), src + EMAIL_OFFSET, EMAIL_SIZE);
	}

	static constexpr uint32_t rowSize() {
		return ROW_SIZE;
	}
};

class Table {
	static constexpr uint32_t PAGE_SIZE = 4096;
	static constexpr uint32_t MAX_PAGES = 100;
	static constexpr uint32_t ROWS_PER_PAGE = PAGE_SIZE / Row::rowSize();
	uint32_t num_rows;
	void *pages[PAGE_SIZE];

public:
	Table() {
		num_rows = 0;
		for (int i = 0; i < MAX_PAGES; ++i)
			pages[i] = nullptr;
	}
	~Table() {
		for (int i = 0; i < MAX_PAGES; ++i) {
			if (pages[i])
				free(pages[i]);
		}
	}

	void *slot(const uint32_t row_num) {
		uint32_t page_num = row_num / ROWS_PER_PAGE;
		void *page = pages[page_num];
		if (page == nullptr) {
			page = pages[page_num] = malloc(PAGE_SIZE);
		}
		uint32_t rowOffset = row_num % ROWS_PER_PAGE;
		uint32_t byteOffset = rowOffset * Row::rowSize();
		return page + byteOffset;
	}

	constexpr uint32_t rows() const {
		return num_rows;
	}

	void incrementRowNum() {
		num_rows += 1;
	}

	static constexpr uint32_t maxRows() {
		return ROWS_PER_PAGE * MAX_PAGES;
	}
};

inline static void printPrompt() {
	std::cout << "db > ";
}

MetaCommandResult runCommand(std::string input, Table *t) {
	if (input == ".exit") {
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
	PrepareResult prepareStatement(std::string st) {
		if (strncmp(st.c_str(), "insert", 6) == 0) {
			type = Insert;
			int args = sscanf(st.c_str(), "insert %d %s %s", &rowToInsert.id, rowToInsert.username, rowToInsert.email);
			if (args < 3) return PrepareSyntaxError;
			return PrepareSuccess;
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

		Row *row = &rowToInsert;
		row->serialize(t->slot(t->rows()));
		t->incrementRowNum();

		return ExecuteSucess;
	}

	ExecuteResult executeSelect(Table *t) {
		Row r;
		for (int i = 0; i < t->rows(); ++i) {
			r.deserialize(t->slot(i));
			r.print();
		}
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
	std::string input;
	Table *table = new Table;

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
				break;
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