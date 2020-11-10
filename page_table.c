#include "page_table.h"
#include "b-tree.h"


uint32_t get_unused_page_num(Pager *pager) {
  /* 获得页管理器最大页+1那个页码，必然没用过 */
  return pager->num_pages;
}

Cursor *table_start(Table *table) {
  /*在表中搜索key(id)=0的位置，即便找不到,返回的也是最小的那个那个id  */
  Cursor *cursor = table_find(table, 0);

  void *node = get_page(table->pager, cursor->page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);
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

  /* 遍历b树查找key */
  if (get_node_type(root_node) == NODE_LEAF) {
    return leaf_node_find(table, root_page_num, key);
  } else {
    return internal_node_find(table, root_page_num, key);
  }
}

Pager *pager_open(const char *filename) {
  /*
   *打开文件，读取文件大小，fd,初始化页管理器pager
   */
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
  pager->file_descriptor = fd;                  /* 文件描述符 */
  pager->file_length = file_length;             /* 文件大小 */
  pager->num_pages = (file_length / PAGE_SIZE); /* 文件页数 */
  /* 如果文件大小不是页数的整数倍数，
   *数据必然出现了错误，
   *当然有可能是页的大小发生了改变
   */
  if (file_length % PAGE_SIZE != 0) {
    printf("Db file is not a whole number of pages. Corrupt file.\n");
    exit(1);
  }
  /* 初始化页表的每一页为null */
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }
  return pager;
}

void cursor_advance(Cursor *cursor) {
  /*
   将游标向后移动1个cell
   暂时版本的游标移动只能在同级的叶节点间，因为暂时没有实现多层树结构下的线索b树，父节点还暂时无法分裂
   */
  uint32_t page_num = cursor->page_num;
  void *node = get_page(cursor->table->pager, page_num);
  cursor->cell_num += 1;

  if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
    /* 如果下一个位置的next=0,
     *那说明到了最右端,
     *正如我们在叶节点所初始化的那样
     */
    uint32_t next_page_num = *leaf_node_next_leaf(node);
    if (next_page_num == 0) {
      /* 到了同级节点最右端了 */
      cursor->end_of_table = true;
    } else {
      cursor->page_num = next_page_num;
      /* 到下一个叶节点 */
      cursor->cell_num = 0;
    }
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
  /*
   *初始化页表
   *可以发现这个函数并没有实际去取出文件中所有的内容
   *而是在get_page中会去读取页内容到动态分配的内存中
   */

  /* 初始化页管理器 */
  Pager *pager = pager_open(filename);
  Table *table = malloc(sizeof(Table));
  table->pager = pager;
  table->root_page_num = 0; /* 根页码初始为0 */

  if (pager->num_pages == 0) {
    /*
     *如果读出来整个文件空的,
     *那么将获得第0页malloc的空白页面，
     *并将此页面的单元数量设置为0，
     *设置该节点的根节点标志位为true
     */
    void *root_node = get_page(pager, 0);
    initialize_leaf_node(root_node); /* num_cells=0 */
    set_node_root(root_node, true);
  }
  return table;
}

void db_close(Table *table) {
  /* 释放资源 */

  Pager *pager = table->pager;
  /* 将pager中最大页数找出 */
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
  /* 请求超过表最大页数，退出 */
  if (page_num > TABLE_MAX_PAGES) {
    /* 如果请求页数大于表最大页数 */
    printf("Tried to fetch page number out of bound.%d>%d \n", page_num,
           TABLE_MAX_PAGES);
    exit(1);
  }
  /* 缓存未命中，分配内存并加载文件内容 */
  if (pager->pages[page_num] == NULL) {
    void *page = malloc(PAGE_SIZE);
    /* 判断文件有几页 */
    uint32_t num_pages = pager->file_length / PAGE_SIZE;
    if (pager->file_length % PAGE_SIZE) {
      /* 若文件末尾有“不完整叶”，添加１行去存储它 。
        文件大小不应该一直是页大小整数倍吗？读者觉得这里是不是作者忘记改了。
      */
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
    pager->pages[page_num] = page;
    /* 如果现在我们申请的第page_num页>页管理器中记录的文件内存页页数，
     *将页管理器中记录的页数更新为page_num页码 + 1（最大页码加１）
     */
    if (page_num >= pager->num_pages) {
      pager->num_pages = page_num + 1;
    }
  }
  return pager->pages[page_num];
}