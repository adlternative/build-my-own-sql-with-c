
#ifndef B_TREE_H
#define B_TREE_H
#include "page_table.h"
#include "stdint.h"
#include "string.h"
typedef enum {
  NODE_INTERNAL,
  NODE_LEAF,
} NodeType;

/*
 *                    internal
 *                /             \
 *              /                 \
 *        internal             internal
 *       /    |   \           /    |   \
 *     /     |    \           /     |    \
 *   leaf   leaf  leaf         leaf   leaf  leaf
 */

/*
 *普通节点头部信息布局：
 * common_node_header:[node_type  is_root  parent_pointer]
 *
 *叶节点头部信息布局：
 *leaf_node_header:[common_node_header,num_cells,next_leaf]
 *叶节点主体信息布局：
 *leaf_node_body:[{key,value},{key,value}...]
 *叶节点布局：leaf_node:[leaf_node_header,leaf_node_body]
 *
 *内部节点头部信息布局：
 *internal_node_header:[common_node_header,num_keys,right_child_page_num]
 *内部节点主体信息布局：
 *internale_node_body:[{key,child_page_num},{key,child_page_num}...]
 * */

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
 * Leaf Node Header Layout(叶子节点头部额外信息的布局)
 */
#define LEAF_NODE_NUM_CELLS_SIZE                                               \
  sizeof(uint32_t) /* 它们包含的“单元格”{key:value}数量 */
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_NEXT_LEAF_SIZE                                               \
  sizeof(uint32_t) /* 指向下一个叶节点（兄弟节点）的页码,     \
                    *个人觉得会像线索二叉树 */
#define LEAF_NODE_NEXT_LEAF_OFFSET                                             \
  ((LEAF_NODE_NUM_CELLS_OFFSET) + (LEAF_NODE_NUM_CELLS_SIZE))

#define LEAF_NODE_HEADER_SIZE                                                  \
  ((COMMON_NODE_HEADER_SIZE) + (LEAF_NODE_NUM_CELLS_SIZE) +                    \
   (LEAF_NODE_NEXT_LEAF_SIZE)) /*叶子节点头大小 */

/*
 * Leaf Node Body Layout(叶子节点主题信息的布局)
 */
#define LEAF_NODE_KEY_SIZE sizeof(uint32_t) /* 叶节点key的大小 */
#define LEAF_NODE_KEY_OFFSET 0
#define LEAF_NODE_VALUE_SIZE ROW_SIZE /* value的大小和row的大小相同 */
#define LEAF_NODE_VALUE_OFFSET ((LEAF_NODE_KEY_OFFSET) + (LEAF_NODE_KEY_SIZE))
#define LEAF_NODE_CELL_SIZE                                                    \
  ((LEAF_NODE_KEY_SIZE) +                                                      \
   (LEAF_NODE_VALUE_SIZE)) /*“单元格”{key:value}大小                  \
                              如本程序是{id:{id,username,email}}*/
#define LEAF_NODE_SPACE_FOR_CELLS ((PAGE_SIZE) - (LEAF_NODE_HEADER_SIZE))
#define LEAF_NODE_MAX_CELLS                                                    \
  ((LEAF_NODE_SPACE_FOR_CELLS) / (LEAF_NODE_CELL_SIZE))

#define LEAF_NODE_RIGHT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS + 1) / 2)
#define LEAF_NODE_LEFT_SPLIT_COUNT                                             \
  ((LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT)

/*
 * Internal Node Header Layout(内部节点头部额外信息的布局)
 */
#define INTERNAL_NODE_NUM_KEYS_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_NUM_KEYS_OFFSET COMMON_NODE_HEADER_SIZE
#define INTERNAL_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_RIGHT_CHILD_OFFSET                                       \
  ((INTERNAL_NODE_NUM_KEYS_OFFSET) + (INTERNAL_NODE_NUM_KEYS_SIZE))
#define INTERNAL_NODE_HEADER_SIZE                                              \
  ((COMMON_NODE_HEADER_SIZE) + (INTERNAL_NODE_NUM_KEYS_SIZE) +                 \
   (INTERNAL_NODE_RIGHT_CHILD_SIZE))

/*
 * Internal Node Body Layout(内部节点主题信息的布局)
 */
#define INTERNAL_NODE_KEY_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CELL_SIZE                                                \
  ((INTERNAL_NODE_CHILD_SIZE) + (INTERNAL_NODE_KEY_SIZE))
#define INTERNAL_NODE_MAX_CELLS 3

void *leaf_node_cell(void *node, uint32_t cell_num);
void *leaf_node_value(void *node, uint32_t cell_num);
void initialize_leaf_node(void *node);
void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value);
void print_leaf_node(void *node);
void set_node_root(void *node, bool is_root);
bool is_node_root(void *node);
uint32_t *node_parent(void *node);
void set_node_type(void *node, NodeType type);
NodeType get_node_type(void *node);
void create_new_root(Table *table, uint32_t right_child_page_num);
void leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value);

uint32_t *leaf_node_num_cells(void *node);
uint32_t *leaf_node_key(void *node, uint32_t cell_num);
uint32_t *internal_node_num_keys(void *node);
uint32_t *internal_node_right_child(void *node);
uint32_t *internal_node_child(void *node, uint32_t child_num);
uint32_t *internal_node_key(void *node, uint32_t key_num);
uint32_t get_node_max_key(void *node);
uint32_t *leaf_node_next_leaf(void *node);
Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key);
Cursor *internal_node_find(Table *table, uint32_t page_num, uint32_t key);
void indent(uint32_t level);
void print_tree(Pager *pager, uint32_t page_num, uint32_t indentation_level);
void update_internal_node_key(void *node, uint32_t old_key, uint32_t new_key);
void internal_node_insert(Table *table, uint32_t parent_page_num,
                          uint32_t child_page_num);
#endif
