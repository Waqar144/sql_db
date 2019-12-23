#include "node.hpp"

uint32_t* leaf_node_num_cells(char* node) {
  return (uint32_t*)(node + LEAF_NODE_NUM_CELLS_OFFSET);
}

char* leaf_node_cell(char* node, uint32_t cell_num) {
  return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(char* node, uint32_t cell_num) {
  return (uint32_t*)leaf_node_cell(node, cell_num);
}

char* leaf_node_value(char* node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

NodeType get_node_type(char *node) {
	uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
	return (NodeType)value;
}

void set_node_type(char *node, NodeType type) {
	uint8_t value = type;
	*((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

void initialize_leaf_node(char* node) {
	set_node_type(node, NodeType::NodeLeaf);
	*leaf_node_num_cells(node) = 0;
}