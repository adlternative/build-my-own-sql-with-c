// #include "state.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include"state.h"
#include"page_table.h"

typedef struct {
  char *buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;

InputBuffer *new_input_buffer() {
  /*仅仅是初始化了一个结构体  */
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
  /* if (input_buffer->buffer[bytes_read - 1] == '\n') {
    printf("yes\n");
  } */

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
  /* 单独检验insert语句中各子语句的长度，
   *如果过长，就返回错误。
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
    {
      return PREPARE_STRING_TOO_LONG;
    }
    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);
    return PREPARE_SUCCESS;
  }
}
PrepareResult prepare_statement(InputBuffer *input_buffer,
                                Statement *statement) {
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

MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table) {
  /* 元数据解析 */
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    /* 如果元数据命令为.exit,则退出 */
    db_close(table);
    // free_table(table);
    exit(0);
  } else {
    /* 返回元数据命令不认识 */
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

ExecuteResult execute_insert(Statement *statement, Table *table) {

  /* 如果行数超标，就意味着内存页写满了，那么insert失败 */
  if (table->num_rows >= TABLE_MAX_ROWS) {
    return EXECUTE_TABLE_FULL;
  }
  Row *row_to_insert = &(statement->row_to_insert);
  // printf("test:%d%s%s\n",row_to_insert->id,row_to_insert->username,row_to_insert->email);
  serialize_row(row_to_insert, row_slot(table, table->num_rows));
  table->num_rows += 1;
  return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(Statement *statement, Table *table) {
  Row row;
  for (uint32_t i = 0; i < table->num_rows; i++) {
    deserialize_row(row_slot(table, i), &row);
    print_row(&row);
  }
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

  // Table *table = new_table();
  if (argc < 2) {
    printf("Usage:Must supply a datebase filename.\n");
    exit(-1);
  }
  const char *filename = argv[1];
  Table *table = db_open(filename);

  InputBuffer *input_buffer = new_input_buffer();

  while (true) {
    print_prompt();           /* 打印db> */
    read_input(input_buffer); /* 读取整行输入 */

    if (input_buffer->buffer[0] == '.') { /* 如果是.预处理的命令 */
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

    /* 如果不是.预处理的命令了
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

    switch (execute_statement(&statement, table)) { /* 根据状态执行相应的行为 */
    case (EXECUTE_SUCCESS):
      printf("Executed.\n");
      break;
    case (EXECUTE_TABLE_FULL):
      printf("Error:Table full.\n");
      break;
    }
  }
}
