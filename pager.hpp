#include <stdint.h>
#include <string>

static constexpr uint32_t PAGE_SIZE = 4096;
static constexpr uint32_t MAX_PAGES = 100;

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
	uint32_t numOfPages;

public:
	Pager() noexcept;
	~Pager();

	inline uint32_t getNumOfPages() {
		return numOfPages;
	}

	void _open(std::string filename);
	char *getPage(uint32_t pageNum);
	void _flush(uint32_t pageNum);
	int _close();

	//return the file length
	inline uint32_t getFileLength() {
		return fileLength;
	}
};