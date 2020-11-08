#include "page_table.h"
#include "b-tree.h"

Cursor *table_start(Table *table) {
  /* 定位到table的root节点的0cell位置 */
  Cursor *cursor = malloc(sizeof(Cursor));
  cursor->table = table;
  cursor->page_num = table->root_page_num; /* 当前节点的页码是根节点的页码 */
  cursor->cell_num = 0;
  void *root_node = get_page(table->pager, table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(root_node); /* 获得根节点上的kv数，
      我觉得是root这里是保存整个树的cell数量对吗？   */
  cursor->end_of_table = (num_cells == 0);
  return cursor;
}

Cursor *table_end(Table *table) {
  /* 定位到table的root节点的最后一个cell位置 */
  Cursor *cursor = malloc(sizeof(Cursor));
  cursor->table = table;
  void *root_node = get_page(table->pager, table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(root_node); /* 获得根节点上的kv数 */
  cursor->cell_num = num_cells; /* 定位为第num_cells个cell/最后一个cell */
  cursor->end_of_table = true;
  return cursor;
}

Cursor *table_find(Table *table, uint32_t key) {
  uint32_t root_page_num = table->root_page_num;
  void *root_node = get_page(table->pager, root_page_num);
  if (get_node_type(root_node) == NODE_LEAF) {
    return leaf_node_find(table, root_page_num, key);
  } else {
    printf("Need to implement searching an internal leaf node.\n");
    exit(1);
  }
}
Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key) {
  void *node = get_page(table->pager, page_num);
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
Pager *pager_open(const char *filename) {
  int fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    printf("Unable to open file\n");
    exit(1);
  }
  off_t file_length = lseek(fd, 0, SEEK_END);
  Pager *pager = malloc(sizeof(Pager));
  /* 可以看出来，这里pager_open只是为pager分配文件信息，
   *页数，文件表述符，并没有进行读取
   */
  pager->file_descriptor = fd;
  pager->file_length = file_length;
  pager->num_pages = (file_length / PAGE_SIZE);
  if (file_length % PAGE_SIZE != 0) {
    printf("Db file is not a whole number of pages. Corrupt file.\n");
    exit(1);
  }
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }
  return pager;
}

void cursor_advance(Cursor *cursor) {
  /* 将游标向后移动1个cell */
  uint32_t page_num = cursor->page_num;
  void *node = get_page(cursor->table->pager, page_num);
  cursor->cell_num += 1;
  if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
    /*如果cell_num+1以后==当前叶节点最大cell_num,说明到了这个节点的末尾*/
    cursor->end_of_table = true;
  }
}

void *cursor_value(Cursor *cursor) {
  // uint32_t row_num = cursor->row_num;
  // uint32_t page_num = row_num / ROWS_PER_PAGE;
  uint32_t page_num = cursor->page_num;
  void *page =
      get_page(cursor->table->pager, page_num); /* 在页表中寻找第page_num页 */
  return leaf_node_value(
      page, cursor->cell_num); /* 从第page_num页返回第cell_num个cell上的value */
}
// void pager_flush(Pager *pager, uint32_t page_num, uint32_t size) {
//   /* 将第 page_num 个页写入到文件page_num * PAGE_SIZE的位置，写入size大小 */
//   if (pager->pages[page_num] == NULL) {
//     printf("Tried to flush null page\n");
//     exit(1);
//   }
//   off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE,
//   SEEK_SET); if (offset == -1) {
//     printf("Error seeking :%d\n", errno);
//     exit(1);
//   }
//   ssize_t bytes_written =
//       write(pager->file_descriptor, pager->pages[page_num], size);
//   if (bytes_written == -1) {
//     printf("Error writing:%d\n", errno);
//     exit(1);
//   }
// }

void pager_flush(Pager *pager, uint32_t page_num) {
  /* 将第 page_num 个页写入到文件page_num * PAGE_SIZE的位置，写入size大小 */
  if (pager->pages[page_num] == NULL) {
    printf("Tried to flush null page\n");
    exit(1);
  }
  off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
  if (offset == -1) {
    printf("Error seeking :%d\n", errno);
    exit(1);
  }
  ssize_t bytes_written =
      write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
  if (bytes_written == -1) {
    printf("Error writing:%d\n", errno);
    exit(1);
  }
}

Table *db_open(const char *filename) {
  /* Table *new_table()的重写 */
  Pager *pager = pager_open(filename);
  Table *table = malloc(sizeof(Table));
  table->pager = pager;
  table->root_page_num = 0; /* 根页码初始为0 */

  if (pager->num_pages == 0) {
    /*
     *如果读出来整个文件空的,
     *那么获得malloc的空白页面，
     *并将此页面的num_cells=0
     */

    void *root_node = get_page(pager, 0);
    initialize_leaf_node(root_node); /* num_cells=0 */
    set_node_root(root_node, true);
  }
  return table;
}

void db_close(Table *table) {
  Pager *pager = table->pager;
  /* 将pager中所有行找出 */
  for (uint32_t i = 0; i < pager->num_pages; i++) {
    /* 如果第i页是空的，continue */
    if (pager->pages[i] == NULL) {
      continue;
    }
    /* 否则将第i页写到磁盘 */
    pager_flush(pager, i);
    /* 释放第i页 */
    free(pager->pages[i]);
    pager->pages[i] = NULL;
  }
  // uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
  // if (num_additional_rows > 0) {
  //   /* 额外的行数算在最后一页 */
  //   uint32_t page_num = num_full_pages;
  //   if (pager->pages[page_num] != NULL) {
  //     pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
  //     free(pager->pages[page_num]);
  //     pager->pages[page_num] = NULL;
  //   }
  // }
  int result = close(pager->file_descriptor);
  if (result == -1) {
    printf("Error close db file.\n");
    exit(1);
  }
  /* 再次检查表上是否有页尚未释放，free */
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    void *page = pager->pages[i];
    if (page) {
      free(page);
      pager->pages[i] = NULL;
    }
  }
  free(pager);
  free(table);
}

void *get_page(Pager *pager, uint32_t page_num) {
  /* page_num:请求第几页 */
  if (page_num > TABLE_MAX_PAGES) {
    /* 如果页数大于表最大页数 */
    printf("Tried to fetch page number out of bound.%d>%d \n", page_num,
           TABLE_MAX_PAGES);
    exit(1);
  }
  /* 缓存未命中，分配内存 */
  if (pager->pages[page_num] == NULL) {
    void *page = malloc(PAGE_SIZE);
    /* 判断文件有几页 */
    uint32_t num_pages = pager->file_length / PAGE_SIZE;
    if (pager->file_length % PAGE_SIZE) {
      num_pages += 1;
    }
    /* 如果请求的页数<=文件页数 */
    if (page_num <= num_pages) {
      /* 定位到文件的相应位置 */
      lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
      /* 读取内容到页内 */
      ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
      if (bytes_read == -1) {
        printf("Error reading file:%d\n", errno);
        exit(1);
      }
    }
    /* 否则这个页是malloc出来的空页 */
    pager->pages[page_num] = page;
    /* 如果现在我们申请的第page_num页>页管理器中记录的页数，
     *将页管理器中记录的页数更新为page_num + 1
     */
    if (page_num >= pager->num_pages) {
      pager->num_pages = page_num + 1;
    }
  }
  return pager->pages[page_num];
}