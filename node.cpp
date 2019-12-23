#include "node.hpp"
#include <iostream>

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

bool is_node_root(char *node) {
	uint8_t value = *((uint8_t *)(node + IS_ROOT_OFFSET));
	return (bool)value;
}

void set_node_root(char *node, bool is_root) {
	uint8_t value = is_root;
	*((uint8_t *)(node + IS_ROOT_OFFSET)) = value;
}

void initialize_leaf_node(char* node) {
	set_node_type(node, NodeType::NodeLeaf);
	set_node_root(node, false);
	*leaf_node_num_cells(node) = 0;
}

/**********************************************************************/

void initialize_internal_node(char *node) {
	set_node_type(node, NodeType::NodeInternal);
	set_node_root(node, false);
	*internal_node_num_keys(node) = 0;
}

uint32_t *internal_node_num_keys(char *node) {
    return reinterpret_cast<uint32_t*>(node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t *internal_node_right_child(char *node) {
    return reinterpret_cast<uint32_t*>(node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t *internal_node_cell(char *node, uint32_t cell_num) {
    return reinterpret_cast<uint32_t*>(node +
            INTERNAL_NODE_HEADER_SIZE +
            cell_num * INTERNAL_NODE_CELL_SIZE);
}

uint32_t *internal_node_child(char *node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys)
    {
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    }
    else if (child_num == num_keys)
    {
        return internal_node_right_child(node);
    }
    else
    {
        return internal_node_cell(node, child_num);
    }
}

uint32_t *internal_node_key(char *node, uint32_t key_num) {
    return internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

uint32_t get_node_max_key(char *node) {
	switch (get_node_type(node)) {
	case NodeType::NodeInternal:
		return *internal_node_key(node, *internal_node_num_keys(node) - 1);
	case NodeType::NodeLeaf:
		return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
  default:
    std::cout << "UNKNOWN - get_node_max_key";
    exit(EXIT_FAILURE);
	}
}