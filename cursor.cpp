#include "cursor.hpp"
#include "node.hpp"
#include "table.hpp"

char* Cursor::value(){
	char *page = table->getPager()->getPage(pageNum);
	return leaf_node_value(page, cellNum);
}
void Cursor::advance() {
	char *node = table->getPager()->getPage(pageNum);
	cellNum++;
	if (cellNum >= (*leaf_node_num_cells(node))) {
		endOfTable = true;
	}
}