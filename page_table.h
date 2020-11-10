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

typedef struct {
  int file_descriptor;          /* 文件描述符 */
  uint32_t file_length;         /* 文件大小 */
  uint32_t num_pages;           /* 页管理器的页数 */
  void *pages[TABLE_MAX_PAGES]; /*  页表，最多100页 */
} Pager;                        /* 全局只有一个Pager页管理器 */

typedef struct {
  Pager *pager;           /* 页管理器 */
  uint32_t root_page_num; /* 根节点页码 */
} Table;                  /* 全局只有一个Table页表 */

/*
 *光标表示表中的一个位置。
 *当我们的表是一个简单的行数组时，
 *我们只需要给定行号就可以访问行。
 *现在它是一棵树，
 *我们通过节点的页码和节点中的单元号来标识位置。
 */
typedef struct {
  Table *table;      /* 页表 */
  uint32_t page_num; /* 节点的页码 */
  uint32_t cell_num; /* 节点中的单元号 */
  bool end_of_table; /* 是否到页表底了 */
} Cursor;/* 游标作为对于页数和单元数的索引 */

/* 页表的开始 */
Cursor *table_start(Table *table);
/* 页表的结束 */
Cursor *table_end(Table *table);
/* 在页表中查找key */
Cursor *table_find(Table *table, uint32_t key);
/* 游标++移动 */
void cursor_advance(Cursor *cursor);
/* 游标索引的节点的值 */
void *cursor_value(Cursor *cursor);
/* 初始化页管理器：打开文件，将文件大小，fd，交给页管理器 */
Pager *pager_open(const char *filename);
/* 初始化页表，内部初始化页管理器 */
Table *db_open(const char *filename);
/* 关闭数据库，释放资源 */
void db_close(Table *table);
/* 获得第page_num页的页内容，实现缓存未命中读取内存页 */
void *get_page(Pager *pager, uint32_t page_num);
/* 将内存刷到磁盘 */
void pager_flush(Pager *pager, uint32_t page_num);
  /* 获得页管理器最大页+1那个页码，必然没用过 */
uint32_t get_unused_page_num(Pager *pager);

#endif