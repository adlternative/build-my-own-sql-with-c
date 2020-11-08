#ifndef STATE_H
#define STATE_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "row.h"
typedef enum {
  EXECUTE_SUCCESS,    /* 成功哦你 */
  EXECUTE_TABLE_FULL, /* 表满 */
  EXECUTE_DUPLICATE_KEY,/* 重复的键（主键:id） */
} ExecuteResult;      /* 执行的结果 */

typedef enum {
  META_COMMAND_SUCCESS,             /* 元数据成功 */
  META_COMMAND_UNRECOGNIZED_COMMAND /* 元数据错误命令 */
} MetaCommandResult;                /* .元数据处理 */

typedef enum {
  PREPARE_SUCCESS,                /* 表示预处理的命令成功 */
  PREPARE_NEGATIVE_ID,            /* 返回了负数的id */
  PREPARE_UNRECOGNIZED_STATEMENT, /* 表示预处理的命令不认识 */
  PREPARE_SYNTAX_ERROR,           /* 表示预处理有语法错误 */
  PREPARE_STRING_TOO_LONG,        /* 输入过长*/
} PrepareResult;                  /* sql语句预处理解析的结果 */

typedef enum {
  STATEMENT_INSERT, /* 状态：insert  */
  STATEMENT_SELECT, /*  状态：select*/
} StatementType;    /* sql语句预处理解析的状态 */

typedef struct {
  StatementType type; /* 一个含有sql语句类型的type */
  Row row_to_insert;  /* 只使用在insert语句的状态 */
} Statement;          /* 一个表明状态的结构体 */

#endif