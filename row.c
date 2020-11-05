#include "row.h"

void print_row(Row *row) {
 printf("(%d,%s,%s)\n", row->id, row->username, row->email);
}


void serialize_row(Row *source, void *destination) {
  /* 将一个Row类型存储到void*的目的位置上，序列化 */
  /* 颇感疑惑 */
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void *source, Row *destination) {
  /*反序列化  */
  memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}
