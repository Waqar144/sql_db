#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include "row.hpp"
#include "pager.hpp"

/***************
 * NODE DATA
 * ************/

enum NodeType {
	NodeInternal,
	NodeLeaf
};

/*
 * Common Node Header Layout
 */
constexpr uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
constexpr uint32_t NODE_TYPE_OFFSET = 0;
constexpr uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
constexpr uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
constexpr uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
constexpr uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
constexpr uint8_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

NodeType get_node_type(char *node);
void set_node_type(char *node, NodeType type);

bool is_node_root(char *node);
void set_node_root(char *node, bool is_root);

/**
 * Leaf Node Header Layout
 */
constexpr uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

/*
 * Leaf Node Body Layout
 */
constexpr uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_KEY_OFFSET = 0;
constexpr uint32_t LEAF_NODE_VALUE_SIZE = Row::ROW_SIZE;
constexpr uint32_t LEAF_NODE_VALUE_OFFSET =
    LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
constexpr uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
constexpr uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
constexpr uint32_t LEAF_NODE_MAX_CELLS =
    LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

constexpr uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
constexpr uint32_t LEAF_NODE_LEFT_SPLIT_COUNT =
    (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

uint32_t* leaf_node_num_cells(char* node);

char* leaf_node_cell(char* node, uint32_t cell_num);

uint32_t* leaf_node_key(char* node, uint32_t cell_num);

char* leaf_node_value(char* node, uint32_t cell_num);

void initialize_leaf_node(char* node);

/************************
 * INTERNAL NODE
 ***********************/

//HEADER
constexpr uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
constexpr uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                           INTERNAL_NODE_NUM_KEYS_SIZE +
                                           INTERNAL_NODE_RIGHT_CHILD_SIZE;

//BODY
constexpr uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_CELL_SIZE =
    INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

void initialize_internal_node(char *node);
uint32_t *internal_node_num_keys(char *node);
uint32_t *internal_node_right_child(char *node);
uint32_t *internal_node_cell(char *node, uint32_t cell_num);
uint32_t *internal_node_child(char *node, uint32_t child_num);
uint32_t *internal_node_key(char *node, uint32_t key_num);
uint32_t get_node_max_key(char* node);
#endif