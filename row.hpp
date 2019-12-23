#ifndef ROW_H
#define ROW_H

#include <stdint.h>
#include <cstring>

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

    void print();

	void serialize(char *dest);

	void deserialize(char *src);

	inline static constexpr uint32_t rowSize() {
		return ROW_SIZE;
	}
};

#endif