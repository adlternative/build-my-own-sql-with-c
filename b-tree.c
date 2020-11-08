#include "b-tree.h"

uint32_t get_unused_page_num(Pager *pager) { return pager->num_pages; }

uint32_t *leaf_node_num_cells(void *node) {
  return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void *leaf_node_cell(void *node, uint32_t cell_num) {
  return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t *leaf_node_key(void *node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num);
}

void *leaf_node_value(void *node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void *node) {
  *leaf_node_num_cells(node) = 0; /* 也就是node_num_cells设置为0 */
  set_node_root(node, false);
  set_node_type(node, NODE_LEAF);
}
void initialize_internal_node(void *node) {
  set_node_root(node, false);
  set_node_type(node, NODE_INTERNAL);
  *internal_node_num_keys(node) = 0;
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
  /* 获得cursor所在页 */
  void *node = get_page(cursor->table->pager, cursor->page_num);
  /* 获得此页的单元（cell）数量 */
  uint32_t num_cells = *leaf_node_num_cells(node);
  if (num_cells >= LEAF_NODE_MAX_CELLS) {
    leaf_node_split_and_insert(cursor, key, value);
    return;
  }
  if (cursor->cell_num < num_cells) {
    /* 将最后一个单元--到要插入位置的单元向后移动*/
    for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
      memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1),
             LEAF_NODE_CELL_SIZE);
    }
  }
  *(leaf_node_num_cells(node)) += 1;
  *(leaf_node_key(node, cursor->cell_num)) = key;
  serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

void print_leaf_node(void *node) {
  uint32_t num_cells = *leaf_node_num_cells(node);
  printf("leaf (size %d)\n", num_cells);
  for (uint32_t i = 0; i < num_cells; i++) {
    uint32_t key = *leaf_node_key(node, i);
    printf("  - %d : %d\n", i, key);
  }
}

NodeType get_node_type(void *node) {
  uint8_t value = *((uint8_t *)(node + NODE_TYPE_OFFSET));
  return (NodeType)value;
}

void set_node_type(void *node, NodeType type) {
  uint8_t value = type;
  *((uint8_t *)(node + NODE_TYPE_OFFSET)) = value;
}

bool is_node_root(void *node) {
  uint8_t value = *((uint8_t *)(node + IS_ROOT_OFFSET));
  return (bool)value;
}

void set_node_root(void *node, bool is_root) {
  uint8_t value = is_root;
  *((uint8_t *)(node + IS_ROOT_OFFSET)) = value;
}

uint32_t *internal_node_num_keys(void *node) {
  /* 定位到内部节点的key数量的偏移量 */
  return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}
uint32_t *internal_node_right_child(void *node) {
  /* 定位到内部节点的rchild的偏移量 */
  return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}
uint32_t *internal_node_cell(void *node, uint32_t cell_num) {
  return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}

uint32_t *internal_node_child(void *node, uint32_t child_num) {
  /* 查看该内部节点的key数量 */
  uint32_t num_keys = *internal_node_num_keys(node);
  /* 如果请求的孩子节点数量>记录的key数量 ，出错 */
  if (child_num > num_keys) {
    printf("Tried to access child_num %d >num_keys %d\n", child_num, num_keys);
    exit(EXIT_FAILURE);
  } else if (child_num == num_keys) {
    /* 返回最右节点 */
    return internal_node_right_child(node);
  } else {
    /* 返回第child_num个节点 */
    return internal_node_cell(node, child_num);
  }
}
uint32_t *internal_node_key(void *node, uint32_t key_num) {
  /* 返回内部节点第key_num个key的位置上的值 */
  return internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}
uint32_t get_node_max_key(void *node) {
  switch (get_node_type(node)) {
  case (NODE_INTERNAL): {
    /* 获取第num_keys-1个key就是最后一个key的值 */
    return *internal_node_key(node, *internal_node_num_keys(node) - 1);
  }
  case (NODE_LEAF): {
    /* 获取第num_cells-1个就是最后一个cell上的key的值 */
    return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
  }
  }
}
void create_new_root(Table *table, uint32_t right_child_page_num) {
  /* 获得root,right_child的节点 */
  void *root = get_page(table->pager, table->root_page_num);
  void *right_child = get_page(table->pager, right_child_page_num);

  /* 获得未使用过的 left节点  */
  uint32_t left_child_page_num = get_unused_page_num(table->pager);
  void *left_child = get_page(table->pager, left_child_page_num);
  /* 将root中所有内容复制给lchild */
  memcpy(left_child, root, PAGE_SIZE);
  set_node_root(left_child, false);

  /* 初始化为内部节点 */
  initialize_internal_node(root);
  set_node_root(root, true);

  /* 这种分裂而成的内部节点暂时只有1个key */
  *internal_node_num_keys(root) = 1;
  /* 它的孩子0标记lchild的坐标 */
  *internal_node_child(root, 0) = left_child_page_num;
  /* key=lchild最大的key */
  uint32_t left_child_max_key = get_node_max_key(left_child);
  *internal_node_key(root, 0) = left_child_max_key;
  /* 它的rchild标记rchild的坐标 */
  *internal_node_right_child(root) = right_child_page_num;
}

void leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value) {

  void *old_node = get_page(cursor->table->pager, cursor->page_num);
  /*  获得空闲页码*/
  uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
  void *new_node = get_page(cursor->table->pager, new_page_num);
  initialize_leaf_node(new_node);
  /* 初始化此页cell_cnt = 0  */

  /* 从最后一个节点开始移动，
   *类似于数组插入元素，
   *插入点之后的节点后移
   */
  for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
    void *destination_node;
    /* 如果超过左叶可容纳总数，
     *那么这个cell就应该分到新的节点上
     */
    if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
      destination_node = new_node;
    } else {
      destination_node = old_node;
    }
    /* 判断当前cell之后应该存在节点哪个坐标 */
    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    /* 获得目标节点上目标单元的内存位置 */
    void *destination = leaf_node_cell(destination_node, index_within_node);
    if (i == cursor->cell_num) {
      /* 中间执行，将我们想要插入的节点序列化到内存相应位置 */
      serialize_row(value, destination);
    } else if (i > cursor->cell_num) {
      /* 先去执行，搬家 */
      memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
    } else {
      /* 最后执行，搬家 */
      memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
    }
  }
  /* 更新俩节点的num_cells */
  *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
  *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;
  /* 如果old节点是根节点 */
  if (is_node_root(old_node)) {
    return create_new_root(cursor->table, new_page_num);
  } else {
    printf("Need to impl updating parent after split.\n");
    exit(1);
  }
}

void indent(uint32_t level) {
  for (uint32_t i = 0; i < level; i++) {
    printf(" ");
  }
}

void print_tree(Pager *pager, uint32_t page_num, uint32_t indentation_level) {
  /* 获得第page_num页的节点 */
  void *node = get_page(pager, page_num);
  uint32_t num_keys, child;
  switch (get_node_type(node)) {
  case (NODE_LEAF): {
    /* 如果是个叶节点 */
    num_keys = *leaf_node_num_cells(node);
    indent(indentation_level);
    /* 打印本叶节点的单元数量 */
    printf("-leaf(size %d)\n", num_keys);
    /* 打印各单元的key */
    for (uint32_t i = 0; i < num_keys; i++) {
      indent(indentation_level + 1);
      printf("-%d\n", *leaf_node_key(node, i));
    }
    break;
  }
  case (NODE_INTERNAL): {
    num_keys = *internal_node_num_keys(node);
    indent(indentation_level);
    /* 打印本内部节点的key数量 */
    printf("-internal(size %d)\n", num_keys);
    for (uint32_t i = 0; i < num_keys; i++) {
      child = *internal_node_child(node, i);
      /* 递归打印每个子节点 */
      print_tree(pager, child, indentation_level + 1);

      indent(indentation_level + 1);
      printf("-key %d\n", *internal_node_key(node, i));
    }
    /* 递归打印最右节点 */
    child = *internal_node_right_child(node);
    print_tree(pager, child, indentation_level + 1);
    break;
  }
  }
}