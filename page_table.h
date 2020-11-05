#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include "row.h"

#define PAGE_SIZE 4096                           /* 页的大小 */
#define TABLE_MAX_PAGES 100                      /* 表的最大页数 */
#define ROWS_PER_PAGE ((PAGE_SIZE) / (ROW_SIZE)) /* 每页行数 4096/293 */
#define TABLE_MAX_ROWS                                                         \
  ((ROWS_PER_PAGE) *                                                           \
   (TABLE_MAX_PAGES)) /* 表上最大的行数=表的最大页数×每页行数 4096/293×100*/

typedef struct {
  int file_descriptor;          /* 文件描述符 */
  uint32_t file_length;         /* 文件大小 */
  void *pages[TABLE_MAX_PAGES]; /*  页表，最多100页 */
} Pager;

typedef struct {
  uint32_t num_rows; /* 页表总行数 */
  Pager *pager;      /* 页管理器 */
} Table;

Pager *pager_open(const char *filename);
Table *db_open(const char *filename);
void db_close(Table *table);
void *get_page(Pager *pager, uint32_t page_num);
void *row_slot(Table *table, uint32_t row_num);
void pager_flush(Pager *pager, uint32_t page_num, uint32_t size);
#endif