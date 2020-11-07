#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H
#include "row.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PAGE_SIZE 4096      /* 页的大小 */
#define TABLE_MAX_PAGES 100 /* 表的最大页数 */
// #define ROWS_PER_PAGE ((PAGE_SIZE) / (ROW_SIZE)) /* 每页行数 4096/293 */
// #define TABLE_MAX_ROWS \
//   ((ROWS_PER_PAGE) * \
//    (TABLE_MAX_PAGES)) /* 表上最大的行数=表的最大页数×每页行数 4096/293×100*/

typedef struct {
  int file_descriptor;          /* 文件描述符 */
  uint32_t file_length;         /* 文件大小 */
  uint32_t num_pages;           /* 页管理器的页数 */
  void *pages[TABLE_MAX_PAGES]; /*  页表，最多100页 */
} Pager;

typedef struct {
  Pager *pager; /* 页管理器 */
  uint32_t root_page_num; /* 根节点页码 */
} Table;

/*
 *光标表示表中的一个位置。
 *当我们的表是一个简单的行数组时，
 *我们只需要给定行号就可以访问行。
 *现在它是一棵树，
 *我们通过节点的页码和节点中的单元号来标识位置。
 */
typedef struct {
  Table *table;
  uint32_t page_num;/* 节点的页码 */
  uint32_t cell_num;/* 节点中的单元号 */
  bool end_of_table;/* 是否到表底了 */
} Cursor;
Cursor *table_start(Table *table);
Cursor *table_end(Table *table);
void cursor_advance(Cursor *cursor);
void *cursor_value(Cursor *cursor);

Pager *pager_open(const char *filename);
Table *db_open(const char *filename);
void db_close(Table *table);
void *get_page(Pager *pager, uint32_t page_num);
void *row_slot(Table *table, uint32_t row_num);
void pager_flush(Pager *pager, uint32_t page_num);
#endif