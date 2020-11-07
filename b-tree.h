
#ifndef B_TREE_H
#define B_TREE_H
#include "page_table.h"
#include "stdint.h"
typedef enum { NODE_INTERVAL, NODE_LEFT } NodeType;

/*
 * Common Node Header Layout(头节点布局/头部说明信息)
 */
#define NODE_TYPE_SIZE                                                         \
  sizeof(uint8_t) /* 节点的类型，貌似是用了8个位，也就意味着最大存256种 */
#define NODE_TYPE_OFFSET 0
#define IS_ROOT_SIZE sizeof(uint8_t) /*节点的类型是不是root，另存  */
#define IS_ROOT_OFFSET NODE_TYPE_SIZE
#define PARENT_POINTER_SIZE sizeof(uint32_t) /* 存父节点指针 用int?深表怀疑 */
#define PARENT_POINTER_OFFSET ((IS_ROOT_OFFSET) + (IS_ROOT_SIZE))
#define COMMON_NODE_HEADER_SIZE                                                \
  ((NODE_TYPE_SIZE) + (IS_ROOT_SIZE) +                                         \
   (PARENT_POINTER_SIZE)) /* 节点头（头部说明信息）大小 */

/*
 * Leaf Node Header Layout(叶子节点额外的布局)
 */
#define LEAF_NODE_NUM_CELLS_SIZE                                               \
  sizeof(uint32_t) /* 它们包含的“单元格”{key:value}数量 */
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_HEADER_SIZE                                                  \
  ((COMMON_NODE_HEADER_SIZE) + (LEAF_NODE_NUM_CELLS_SIZE)) /*叶子节点头大小 */

/*
 * Leaf Node Body Layout
 */
#define LEAF_NODE_KEY_SIZE sizeof(uint32_t) /* 叶节点key的大小 */
#define LEAF_NODE_KEY_OFFSET 0
#define LEAF_NODE_VALUE_SIZE ROW_SIZE /* value的大小和row的大小相同 */
#define LEAF_NODE_VALUE_OFFSET ((LEAF_NODE_KEY_OFFSET) + (LEAF_NODE_KEY_SIZE))
#define LEAF_NODE_CELL_SIZE                                                    \
  ((LEAF_NODE_KEY_SIZE) + (LEAF_NODE_VALUE_SIZE)) /*“单元格”{key:value}大小 */
#define LEAF_NODE_SPACE_FOR_CELLS ((PAGE_SIZE) - (LEAF_NODE_HEADER_SIZE))
#define LEAF_NODE_MAX_CELLS                                                    \
  ((LEAF_NODE_SPACE_FOR_CELLS) / (LEAF_NODE_CELL_SIZE))

uint32_t *leaf_node_num_cells(void *node);
void *leaf_node_cell(void *node, uint32_t cell_num);
uint32_t *leaf_node_key(void *node, uint32_t cell_num);
void *leaf_node_value(void *node, uint32_t cell_num);
void initialize_leaf_node(void *node);
void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value);
void print_leaf_node(void *node);
#endif
