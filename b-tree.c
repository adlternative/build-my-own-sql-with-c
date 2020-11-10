#include "b-tree.h"
#include "page_table.h"

NodeType get_node_type(void *node) {
  uint8_t value = *((uint8_t *)(node + NODE_TYPE_OFFSET));
  return (NodeType)value;
}
void set_node_type(void *node, NodeType type) {
  /* 设置节点类型 */
  uint8_t value = type;
  *((uint8_t *)(node + NODE_TYPE_OFFSET)) = value;
}
bool is_node_root(void *node) {
  /* 判断是不是根节点 */
  uint8_t value = *((uint8_t *)(node + IS_ROOT_OFFSET));
  return (bool)value;
}
void set_node_root(void *node, bool is_root) {
  /* 设这节点是不是根节点 */
  uint8_t value = is_root;
  *((uint8_t *)(node + IS_ROOT_OFFSET)) = value;
}
uint32_t get_node_max_key(void *node) {
  /* 获得节点的最大key */
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
uint32_t *node_parent(void *node) { return node + PARENT_POINTER_OFFSET; }

uint32_t *leaf_node_next_leaf(void *node) {
  /* 获得叶子的兄弟节点的页码 */
  return node + LEAF_NODE_NEXT_LEAF_OFFSET;
}
uint32_t *leaf_node_num_cells(void *node) {
  /* 获得叶节点cells的数量 */
  return node + LEAF_NODE_NUM_CELLS_OFFSET;
}
void *leaf_node_cell(void *node, uint32_t cell_num) {
  /* 获得叶节点中第cell_num个cell */
  return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}
uint32_t *leaf_node_key(void *node, uint32_t cell_num) {
  /* 获得叶节点中第cell_num个cell的key
   *注意到和leaf_node_cell的返回值类型不同，
   *所以可以获取到key
   */
  return leaf_node_cell(node, cell_num);
}
void *leaf_node_value(void *node, uint32_t cell_num) {
  /* 获得叶节点第cell_num个单元的value */
  return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}
void print_leaf_node(void *node) {
  /* 打印叶节点信息单元数量，key */
  uint32_t num_cells = *leaf_node_num_cells(node);
  printf("leaf (size %d)\n", num_cells);
  for (uint32_t i = 0; i < num_cells; i++) {
    uint32_t key = *leaf_node_key(node, i);
    printf("  - %d : %d\n", i, key);
  }
}
void initialize_leaf_node(void *node) {
  /*
   * 初始化叶节点,
   * node_num_cells设置为0,
   * 设置节点类型以及是不是根节点
   */
  set_node_root(node, false);
  set_node_type(node, NODE_LEAF);
  *leaf_node_num_cells(node) = 0;
  *leaf_node_next_leaf(node) = 0;
}
void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
  /*
   *向叶子中插入一个单元数据
   *(其实这个key是value中的id)
   */

  /* 获得cursor所在页 */
  void *node = get_page(cursor->table->pager, cursor->page_num);
  /* 获得此页的单元（cell）数量 */
  uint32_t num_cells = *leaf_node_num_cells(node);
  if (num_cells >= LEAF_NODE_MAX_CELLS) {
    /* 如果满了 ，则分裂节点再插入*/
    leaf_node_split_and_insert(cursor, key, value);
    return;
  }
  /* 否则将最后一个单元--到要插入位置的单元向后移动*/
  if (cursor->cell_num < num_cells) {
    for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
      memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1),
             LEAF_NODE_CELL_SIZE);
    }
  }
  /* 这个页的单元数量++ */
  *(leaf_node_num_cells(node)) += 1;
  /* 将这个单元的key设置为key */
  *(leaf_node_key(node, cursor->cell_num)) = key;
  /* 将这个单元的value设置为value */
  serialize_row(value, leaf_node_value(node, cursor->cell_num));
}
void leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value) {
  /*
   *叶子节点分裂并插入新数据
   *
   *
   */

  void *old_node = get_page(cursor->table->pager, cursor->page_num);
  /* 获得旧叶子的maxkey */
  uint32_t old_max = get_node_max_key(old_node);

  /*  获得空闲页码*/
  uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
  /* 获取新节点，空白页 */
  void *new_node = get_page(cursor->table->pager, new_page_num);
  /* 初始化这个新的页为叶节点  */
  initialize_leaf_node(new_node);
  /* 新节点的父亲指针指向旧节点的父亲 */
  *node_parent(new_node) = *node_parent(old_node);
  /* 新节点的next指针指向旧节点的next指针指向的叶节点 */
  *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
  /* 旧节点的next指针指向新节点 */
  *leaf_node_next_leaf(old_node) = new_page_num;

  /* cell_cnt = 0从最后一个节点开始移动，
   *类似于数组插入元素，
   *插入点之后的节点后移
   */
  for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
    void *destination_node;
    /* 如果超过左叶可容纳总数，
     *那么这个cell就应该分到新的节点上
     */
    if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
      /* 过半去新 */
      destination_node = new_node;
    } else {
      /* 未半去旧 */
      destination_node = old_node;
    }
    /* 判断当前cell之后应该存在节点哪个坐标 */
    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    /* 获得目标节点上目标单元的内存位置 */
    void *destination = leaf_node_cell(destination_node, index_within_node);
    if (i == cursor->cell_num) {
      /* 中间执行，将我们想要插入的节点序列化到内存相应位置 */
      serialize_row(value,
                    leaf_node_value(destination_node, index_within_node));
      *leaf_node_key(destination_node, index_within_node) = key;
    } else if (i > cursor->cell_num) {
      /* 先去执行，插入点以后的需要向后偏移一个格子 */
      memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
    } else {
      /* 最后执行，搬家 */
      memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
    }
  }
  /* 更新俩节点的num_cells */
  *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
  *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;
  if (is_node_root(old_node)) {
    /*
     *如果old节点是根节
     *那么将创建内部节点作为新的root
     */
    return create_new_root(cursor->table, new_page_num);
  } else {
    /* 说明已经有父亲节点了 */
    uint32_t parent_page_num = *node_parent(old_node);
    /* 旧节点被分裂后的maxkey */
    uint32_t new_max = get_node_max_key(old_node);
    /* 获取父节点 */
    void *parent = get_page(cursor->table->pager, parent_page_num);
    /* 更新父节点中旧节点的key值 */
    update_internal_node_key(parent, old_max, new_max);
    /* 在父节点（内部节点）中插入新的key */
    internal_node_insert(cursor->table, parent_page_num, new_page_num);
    return;
  }
}
Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key) {
  /*
   *在叶节点找key
   *是二分找到要key"可插入"的位置的坐标，可以说若未找到则返回其右边界
   */

  /* 获得页 */
  void *node = get_page(table->pager, page_num);
  /* 获得单元总数 */
  uint32_t num_cells = *leaf_node_num_cells(node);
  Cursor *cursor = malloc(sizeof(Cursor));
  cursor->table = table;
  cursor->page_num = page_num;

  /* 二分搜索 */
  uint32_t min_index = 0;                  /* begin() */
  uint32_t one_past_max_index = num_cells; /* 一个过去的最大cell坐标,end() */

  while (one_past_max_index != min_index) {
    uint32_t index = (min_index + one_past_max_index) / 2;
    uint32_t key_at_index = *leaf_node_key(node, index);
    if (key_at_index == key) {
      cursor->cell_num = index;
      return cursor;
    }
    if (key < key_at_index) {
      one_past_max_index = index;
    } else {
      min_index = index + 1;
    }
  }
  cursor->cell_num = min_index;
  return cursor;
}

void initialize_internal_node(void *node) {
  /*
   * 初始化内部节点,
   * node_num_keys设置为0
   * 设置节点类型以及是不是根节点
   */
  set_node_root(node, false);
  set_node_type(node, NODE_INTERNAL);
  *internal_node_num_keys(node) = 0;
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
  /* 获得坐标是child_num的孩子节点页码 */

  /* 查看该内部节点的key数量 */
  uint32_t num_keys = *internal_node_num_keys(node);
  /* 如果请求的孩子节点数量>记录的key数量 ，出错 */
  if (child_num > num_keys) {
    printf("Tried to access child_num %d >num_keys %d\n", child_num, num_keys);
    exit(EXIT_FAILURE);
  } else if (child_num == num_keys) {
    /* 如果是请求的index=key的数量,返回最右节点的页码
     *思考（*,4,*,6,*8,*） 3个key,取出第三个key也就是取出最右节点
     */
    return internal_node_right_child(node);
  } else {
    /* 返回第child_num个节点的页码 */
    return internal_node_cell(node, child_num);
  }
}
uint32_t *internal_node_key(void *node, uint32_t key_num) {
  /* 返回内部节点第key_num个key的位置上的值
   *注意这个void*是为了在取地址的指针加法中加上
   *INTERNAL_NODE_CHILD_SIZE个字节大小
   *而不是sizeof(uint32_t)*INTERNAL_NODE_CHILD_SIZE　
   */
  return (void *)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}
uint32_t internal_node_find_child(void *node, uint32_t key) {
  /*
   *在内部节点找key
   *同样和leaf_node_find很像
   *是二分找到要key"可插入"的位置的坐标，可以说若未找到则返回其右边界
   */

  uint32_t num_keys = *internal_node_num_keys(node);
  uint32_t min_index = 0;
  uint32_t max_index = num_keys;

  /* 二分搜索key */
  while (min_index != max_index) {
    uint32_t index = (min_index + max_index) / 2;
    uint32_t key_to_right = *internal_node_key(node, index);
    if (key_to_right >= key) {
      max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  return min_index;
}
Cursor *internal_node_find(Table *table, uint32_t page_num, uint32_t key) {
  /*
   *内部节点递归子节点查找{key:value}
   *注意：内部节点必有子节点，so递归最终肯定落于叶节点
   */

  void *node = get_page(table->pager, page_num);
  uint32_t child_index = internal_node_find_child(node, key);

  /* 得到key子节点的坐标 */
  uint32_t child_num = *internal_node_child(node, child_index);
  void *child = get_page(table->pager, child_num);

  switch (get_node_type(child)) {
  case (NODE_LEAF):
    return leaf_node_find(table, child_num, key);
  case (NODE_INTERNAL):
    return internal_node_find(table, child_num, key);
  }
}

void update_internal_node_key(void *node, uint32_t old_key, uint32_t new_key) {
  /* 叶节点分裂以后,更新其父节点的key */
  /* 获取old_key对应的子节点坐标 */
  uint32_t old_child_index = internal_node_find_child(node, old_key);
  /* 将key更新 */
  *internal_node_key(node, old_child_index) = new_key;
}

void internal_node_insert(Table *table, uint32_t parent_page_num,
                          uint32_t child_page_num) {
  /* 叶节点分裂以后，在内部节点中插入一个新key */

  /* 获得父子节点 */
  void *parent = get_page(table->pager, parent_page_num);
  void *child = get_page(table->pager, child_page_num);
  /* 获得子的maxkey */
  uint32_t child_max_key = get_node_max_key(child);
  /* 获得子的maxkey在父中的坐标index */
  uint32_t index = internal_node_find_child(parent, child_max_key);
  /* 获得父节点key数量 */
  uint32_t original_num_keys = *internal_node_num_keys(parent);
  /* 父节点的keys数量++ */
  *internal_node_num_keys(parent) = original_num_keys + 1;
  /* 如果父节点的keys数量超标，需要去实现父节点的分裂 */
  if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
    printf("Need to impl splitting internal node.\n");
    exit(1);
  }
  /* 获取右孩子节点 */
  uint32_t right_child_page_num = *internal_node_right_child(parent);
  void *right_child = get_page(table->pager, right_child_page_num);

  /* 如果右孩子的maxkey要小于当前我们想要插入的key */
  if (child_max_key > get_node_max_key(right_child)) {
    /* 将右孩子复制给父节点最后一个后面的节点，包括页码和key */
    *internal_node_child(parent, original_num_keys) = right_child_page_num;
    *internal_node_key(parent, original_num_keys) =
        get_node_max_key(right_child);
    /* 将右节点的页码更新 */
    *internal_node_right_child(parent) = child_page_num;
  }
  /* 否则 */
  else {
    /* 后者右移 */
    for (uint32_t i = original_num_keys; i > index; i--) {
      void *destination = internal_node_cell(parent, i);
      void *source = internal_node_cell(parent, i - 1);
      memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
    }
    /* index 填入新节点页码和maxkey */
    *internal_node_child(parent, index) = child_page_num;
    *internal_node_key(parent, index) = child_max_key;
  }
}

void create_new_root(Table *table, uint32_t right_child_page_num) {
  /* 旧root分裂的时候创造新root */

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
  /* 它的第0孩子标记为lchild的坐标
   *值得注意的是，此时是因为第一次分裂，这个新根节点才是左右俩节点，
   *事实上内部节点上的key可以有很多个，暂时有个疑问，为什么需要单独将右节点弄出来？
   *疑问解决，因为　内部节点*3*5*6*的布局，右节点的maxkey不需要在父节点呈现，
   *但在我们为在父节点插入一个新的key的时候(internal_node_insert)能比较到最右边的节点的maxkey
   *(否则父节点不会去保存最右边的节点的maxkey)，这样插入新key就不好比较插入的位置
   */
  *internal_node_child(root, 0) = left_child_page_num;
  /* key=lchild最大的key */
  uint32_t left_child_max_key = get_node_max_key(left_child);
  *internal_node_key(root, 0) = left_child_max_key;
  /* 它的rchild标记rchild的坐标 */
  *internal_node_right_child(root) = right_child_page_num;
  /* 将分裂开的节点的parent 标记为父节点 */
  *node_parent(left_child) = table->root_page_num;
  *node_parent(right_child) = table->root_page_num;
}
void indent(uint32_t level) {
  /* 根据层数打印空格 */
  for (uint32_t i = 0; i < level; i++) {
    printf(" ");
  }
}
void print_tree(Pager *pager, uint32_t page_num, uint32_t indentation_level) {
  /* 递归打印树 */

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