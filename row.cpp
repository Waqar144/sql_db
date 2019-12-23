#include <iostream>
#include "row.hpp"

void Row::print() {
	std::cout << "(" << id << ", "<< username << ", " << email << ")\n";
}

void Row::serialize(char *dest) {
	memcpy(dest + ID_OFFSET, &(id), ID_SIZE);
	memcpy(dest + USERNAME_OFFSET, &(username), USERNAME_SIZE);
	memcpy(dest + EMAIL_OFFSET, &(email), EMAIL_SIZE);
}

void Row::deserialize(char *src) {
	memcpy(&(id), src + ID_OFFSET, ID_SIZE);
	memcpy(&(username), src + USERNAME_OFFSET, USERNAME_SIZE);
	memcpy(&(email), src + EMAIL_OFFSET, EMAIL_SIZE);
}
