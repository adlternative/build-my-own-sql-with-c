
#include "b-tree.h"
#include "page_table.h"
#include "state.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
typedef struct {
  char *buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;

InputBuffer *new_input_buffer() {
  /*仅仅是初始化了一个结构体，接着我们输入要用它  */
  InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;
  return input_buffer;
}

void close_input_buffer(InputBuffer *input_buffer) {
  /* 释放缓冲区资源 */
  free(input_buffer->buffer);
  free(input_buffer);
}

void print_prompt() {
  /* 打印了数据库提示 */
  printf("db > ");
}

void read_input(InputBuffer *input_buffer) {
  /* 读取终端输入 */
  /* longint */ ssize_t bytes_read =
      getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
  /* 如果读取到的数量<0，则出错且退出 */
  if (bytes_read < 0) {
    printf("Error reading input\n");
    exit(1);
  }
  /* 读到的数量为bytes_read(含有'\n')，
   *去除掉最后的'\n'变为'\0'
   */
  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = '\0';
}

PrepareResult prepare_insert(InputBuffer *input_buffer, Statement *statement) {
  /* 单独检验insert语句中各子语句的正确性，
   *如果过长，就返回错误。
   *如果非法如id<0，就返回错误。
   */
  statement->type = STATEMENT_INSERT;
  char *keyword = strtok(input_buffer->buffer, " ");
  char *id_string = strtok(NULL, " ");
  char *username = strtok(NULL, " ");
  char *email = strtok(NULL, " ");
  if (id_string == NULL || username == NULL || email == NULL) {
    return PREPARE_SYNTAX_ERROR;
  }
  int id = atoi(id_string);
  if (id < 0) {
    return PREPARE_NEGATIVE_ID;
  }
  if (strlen(username) > COLUMN_USERNAME_SIZE) {
    return PREPARE_STRING_TOO_LONG;
  }
  if (strlen(email) > COLUMN_EMAIL_SIZE) {

    return PREPARE_STRING_TOO_LONG;
  }
  statement->row_to_insert.id = id;
  strcpy(statement->row_to_insert.username, username);
  strcpy(statement->row_to_insert.email, email);
  return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer *input_buffer,
                                Statement *statement) {
  /*
   *判断预处理的状态(insert select)
   */

  /* 如果之前的状态是insert 那么statement的type 变为STATEMENT_INSERT
   *并返回PREPARE_SUCCESS
   */

  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    return prepare_insert(input_buffer, statement);

    /*
        statement->type = STATEMENT_INSERT;
        int args_assigned = sscanf(
            input_buffer->buffer, "insert %d %s %s",
       &(statement->row_to_insert.id), statement->row_to_insert.username,
       statement->row_to_insert.email); if (args_assigned < 3) { return
       PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS; */
  }
  /* 如果之前的状态是select 那么statement的type 变为STATEMENT_SELECT
   *并返回PREPARE_SUCCESS
   */
  if (strcmp(input_buffer->buffer, "select") == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }
  /*如都不是则之前的状态为不认识的指令  */
  return PREPARE_UNRECOGNIZED_STATEMENT;
}

void print_constants() {
  printf("ROW_SIZE: %ld\n", ROW_SIZE);
  printf("COMMON_NODE_HEADER_SIZE: %ld\n", COMMON_NODE_HEADER_SIZE);
  printf("LEAF_NODE_HEADER_SIZE: %ld\n", LEAF_NODE_HEADER_SIZE);
  printf("LEAF_NODE_CELL_SIZE: %ld\n", LEAF_NODE_CELL_SIZE);
  printf("LEAF_NODE_SPACE_FOR_CELLS: %ld\n", LEAF_NODE_SPACE_FOR_CELLS);
  printf("LEAF_NODE_MAX_CELLS: %ld\n", LEAF_NODE_MAX_CELLS);
}
/* insert 11 as as@qq.com */
MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table) {
  /* 元数据解析 */
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    /* 如果元数据命令为.exit,则退出 */
    db_close(table);
    // free_table(table);
    exit(0);
  } else if (strcmp(input_buffer->buffer, ".btree") == 0) {
    printf("Tree:\n");
    print_tree(table->pager, 0, 0);
    return META_COMMAND_SUCCESS;
  } else if (strcmp(input_buffer->buffer, ".constants") == 0) {
    printf("Constants:\n");
    print_constants();
    return META_COMMAND_SUCCESS;
  } else {
    /* 返回元数据命令不认识 */
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

ExecuteResult execute_insert(Statement *statement, Table *table) {
  /* 执行插入 */

  void *node = get_page(table->pager, table->root_page_num);
  /* 获取节点上的总单元数量 */
  uint32_t num_cells = *(leaf_node_num_cells(node));

  /* 获得statement中需要插入的row的信息 */
  Row *row_to_insert = &(statement->row_to_insert);

  uint32_t key_to_insert = row_to_insert->id;
  /*
  寻找我们需要插入的位置（叶子节点二分找key位置，内部节点二分找key位置且递归）
   */
  Cursor *cursor = table_find(table, key_to_insert);
  /* 如果我们需要插入的位置不是节点上的总单元数量(最后一个，肯定不会重复) */
  if (cursor->cell_num < num_cells) {
    uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
    /* 重复则不插入，报错 */
    if (key_at_index == key_to_insert) {
      return EXECUTE_DUPLICATE_KEY;
    }
  }
  /* 执行叶节点的插入 */
  leaf_node_insert(cursor, row_to_insert->id, row_to_insert);
  free(cursor);
  return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(Statement *statement, Table *table) {
  Cursor *cursor = table_start(table);
  Row row;

  while (!(cursor->end_of_table)) {
    /* 遍历所有叶节点 */
    deserialize_row(cursor_value(cursor), &row);
    print_row(&row);
    /*
     将游标向后移动1个cell
     暂时版本的游标移动只能在同级的叶节点间，因为暂时没有实现多层树结构下的线索b树，父节点还暂时无法分裂
     */
    cursor_advance(cursor);
  }
  free(cursor);
  return EXECUTE_SUCCESS;
}
ExecuteResult execute_statement(Statement *statement, Table *table) {
  /* 根据状态执行 */
  switch (statement->type) {
  /* 重构为执行函数 */
  case (STATEMENT_INSERT):
    return execute_insert(statement, table);
  case (STATEMENT_SELECT):
    return execute_select(statement, table);
  }
}

int main(int argc, char const *argv[]) {
  /* 信号屏蔽 */
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);

  /* 必须在命令行输入一个 [./main xxx.db]*/
  if (argc < 2) {
    printf("Usage:你只输入了程序名，没有输入想要操控的数据库文件。./main "
           "xxx.db.\n");
    exit(-1);
  }

  const char *filename = argv[1];
  Table *table = db_open(filename);

  InputBuffer *input_buffer = new_input_buffer();

  while (true) {
    print_prompt();           /* 打印db> */
    read_input(input_buffer); /* 读取整行输入 */

    if (input_buffer->buffer[0] == '.') { /* 如果是.exit这样的元数据的命令 */
      /* 解析命令 */
      switch (do_meta_command(input_buffer, table)) {
        /* 成功继续 */
      case (META_COMMAND_SUCCESS):
        continue;
        /* 不认识的 .预处理的命令 则输出不认识的命令,且不用后续判断状态了*/
      case (META_COMMAND_UNRECOGNIZED_COMMAND):
        printf("Unrecognized command: %s\n", input_buffer->buffer);
        continue;
      }
    }

    /* 如果不是元数据的命令了
     *检验input_buffer预处理的状态并存储到statement中 */
    Statement statement;
    switch (prepare_statement(input_buffer, &statement)) {
      /* 预处理解析成功 */
    case (PREPARE_SUCCESS):
      break;
    case (PREPARE_NEGATIVE_ID):
      printf("ID must be positive\n");
      continue;
    case (PREPARE_SYNTAX_ERROR):
      /* 语法错误 */
      printf("Syntax error.Could not parse statement.\n");
      continue;

    case (PREPARE_STRING_TOO_LONG):
      /* 子语句过长 如name>32 */
      printf("String is too long.\n");
      continue;

    case (PREPARE_UNRECOGNIZED_STATEMENT):
      /* 预处理的状态是不认识命令 */
      printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
      continue;
    }

      /* 根据状态执行相应的行为 */
    switch (execute_statement(&statement, table)) {
    case (EXECUTE_SUCCESS):
      printf("Executed.\n");
      break;
    case (EXECUTE_DUPLICATE_KEY):
      printf("Error:Duplicate key.\n");
      break;
    case (EXECUTE_TABLE_FULL):
      printf("Error:Table full.\n");
      break;
    }
  }
}
