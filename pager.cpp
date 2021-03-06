#include <iostream>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "pager.hpp"

Pager::Pager() noexcept {
    for (int i = 0; i < MAX_PAGES; ++i) {
        pages[i] = nullptr;
    }
    fileLength = 0;
}

Pager::~Pager() {
    for (int i = 0; i < MAX_PAGES; ++i) {
        if (pages[i]) {
            delete[] pages[i];
            pages[i] = nullptr;
        }
    }
}

void Pager::_open(std::string filename) {
	fileDescriptor = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fileDescriptor == -1) {
		std::cout << "Unable to open file\n";
	}

	fileLength = lseek(fileDescriptor, 0, SEEK_END);
	numOfPages = fileLength / PAGE_SIZE;

	if (fileLength % PAGE_SIZE != 0) {
		std::cout << "DB file is corrupt!\n";
	}
}

/**
 * @brief returns the page at pageNum
 */
char *Pager::getPage(uint32_t pageNum) {
	if (pageNum > MAX_PAGES) {
		std::cout << "This page number is out of bounds.\n";
		exit(EXIT_FAILURE);
	}

	if (pages[pageNum] == nullptr) {
		char *page = new char[PAGE_SIZE];
		uint32_t numOfPages_ = fileLength / PAGE_SIZE;

		//incomplete page
		if(fileLength % PAGE_SIZE) {
			numOfPages_ += 1;
		}

		if (pageNum <= numOfPages_) {
			lseek(fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);
			ssize_t numOfBytesRead = read(fileDescriptor, page, PAGE_SIZE);
			if (numOfBytesRead == -1) {
				std::cout << "Error reading file\n";
				exit(EXIT_FAILURE);
			}
		}
		pages[pageNum] = page;
		if (pageNum >= numOfPages) {
			this->numOfPages = pageNum + 1;
		}
	}
	return pages[pageNum];
}

uint32_t Pager::getUnusedPageNum() {
	return numOfPages;
}

void Pager::_flush(uint32_t pageNum) {
	if (pages[pageNum] == nullptr) {
		std::cout << "Tried to flush null page. Exiting..." << std::endl;
		exit(EXIT_FAILURE);
	}

	off_t offset = lseek(fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);
	if (offset == -1) {
		std::cout << "Error seeking file. Exiting... \n";
		exit(EXIT_FAILURE);
	}

	ssize_t numOfBytesWritten = write(fileDescriptor, pages[pageNum], PAGE_SIZE);
	if (numOfBytesWritten == -1) {
		std::cout << "Error writing to file. Exiting...\n";
		exit(EXIT_FAILURE);
	}
}

int Pager::_close() {
	return close(fileDescriptor);
}