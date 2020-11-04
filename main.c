#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  char *buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;

typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

/* sql编译器的状态 */
typedef enum { PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT } PrepareResult;
/*  */
typedef enum {STATEMENT_INSERT, STATEMENT_SELECT}StatementType;

typedef struct {
  StatementType type;
}Statement;


InputBuffer *
new_input_buffer() {
  /*仅仅是初始化了一个结构体  */
  InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;
  return input_buffer;
}

void close_input_buffer(InputBuffer *input_buffer) {
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

  printf("%p\n", input_buffer->buffer);
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

PrepareResult prepared_statement(InputBuffer *input_buffer,Statement*statement){
  if(strncmp(input_buffer->buffer,"insert",6)==0){
    statement->type = STATEMENT_INSERT;
    return PREPARE_SUCCESS;
  }
  if(strncmp(input_buffer->buffer,"select",)==0){
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}
MetaCommandResult do_meta_command(InputBuffer*input_buffer){
  if(strcmp(input_buffer->buffer,".exit")==0){
    exit(0);
  }else{
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

int main(int argc, char const *argv[]) {

  InputBuffer *input_buffer = new_input_buffer();
  while (true) {
    print_prompt();
    read_input(input_buffer);
    /* printf("%ld", input_buffer->input_length);
    printf("%ld", input_buffer->buffer_length); */
    if (input_buffer->buffer[0] == '.') {
      /* 解析命令 */
      switch (do_meta_command(input_buffer)) {
        /* 成功继续 */
      case (META_COMMAND_SUCCESS):
        continue;
        /* 不认识则输出 */
      case (META_COMMAND_UNRECOGNIZED_COMMAND):
        printf("Unrecognized command: %s\n", input_buffer->buffer);
        continue;
      }
    }
/* vscode://vscode.github-authentication/did-authenticate?windowid=1&code=4103db2fe87044f2e99e&state=bf6a07aa-0463-45fc-b744-097a721991dd */
    Statement statement;
    switch (prepared_statement(input_buffer, &statement)) {
      /* 之前的状态成功 */
    case (PREPARE_SUCCESS):
      break;
      /* 之前的状态是不认识命令 */
    case (PREPARE_UNRECOGNIZED_STATEMENT):
      printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
      continue;
    }
    execute_statement(statement);
    printf("Executed.\n");
  }
}
