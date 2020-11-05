#include "page_table.h"

Pager *pager_open(const char *filename) {
  int fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    printf("Unable to open file\n");
    exit(1);
  }
  off_t file_length = lseek(fd, 0, SEEK_END);
  Pager *pager = malloc(sizeof(Pager));
  pager->file_descriptor = fd;
  pager->file_length = file_length;
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }
  return pager;
}
void *row_slot(Table *table, uint32_t row_num) {
  /* 找到在内存中读/写特定行的位置 */
  uint32_t page_num =
      row_num / ROWS_PER_PAGE; /*第几页=想要寻找的行数/每页行数 */
  // void *page = table->pages[page_num];
  void *page = get_page(table->pager, page_num);
  /*   if (page == NULL) {
      page = table->pages[page_num] = malloc(PAGE_SIZE);
    } */
  uint32_t row_offset = row_num % ROWS_PER_PAGE; /* 此页第row_offset行 */
  uint32_t byte_offset = row_offset * ROW_SIZE;  /* 偏移多少字节 */
  return page + byte_offset; /* 寻找到需要寻址的位置了 */
}

void pager_flush(Pager *pager, uint32_t page_num, uint32_t size) {
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
      write(pager->file_descriptor, pager->pages[page_num], size);
  if (bytes_written == -1) {
    printf("Error writing:%d\n", errno);
    exit(1);
  }
}

Table *db_open(const char *filename) {
  /* Table *new_table()的重写 */
  Pager *pager = pager_open(filename);
  uint32_t num_rows = pager->file_length / ROW_SIZE;
  Table *table = malloc(sizeof(Table));
  table->pager = pager;
  table->num_rows = num_rows;
  return table;
}

void db_close(Table *table) {
  Pager *pager = table->pager;
  uint32_t num_full_pages =
      table->num_rows / ROWS_PER_PAGE; /*算出内存table中的总页数*/
  for (uint32_t i = 0; i < num_full_pages; i++) {
    /* 如果第i页是空的，continue */
    if (pager->pages[i] == NULL) {
      continue;
    }
    /* 否则将第i页写到磁盘 */
    pager_flush(pager, i, PAGE_SIZE);
    /* 释放第i页 */
    free(pager->pages[i]);
    pager->pages[i] = NULL;
  }
  uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
  if (num_additional_rows > 0) {
    /* 额外的行数算在最后一页 */
    uint32_t page_num = num_full_pages;
    if (pager->pages[page_num] != NULL) {
      pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
      free(pager->pages[page_num]);
      pager->pages[page_num] = NULL;
    }
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
  }
  return pager->pages[page_num];
}