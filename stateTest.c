#include"state.h"
void serialize_row(Row *source, void *destination) {
  /* 将一个Row类型存储到void*的目的位置上，序列化 */
  /* 颇感疑惑：
  * 紧凑表示法 */
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
int main(int argc, char const *argv[])
{
  Row r,r2;
  r.id = 1;
  strcpy(r.username, "sad");
  strcpy(r.email, "123@1qq.com");
  char sss[1000];
  serialize_row(&r,sss);
  deserialize_row(sss,&r2 );
  printf("%s\n",r2.email);
  printf("%s\n",r2.username);
  printf("%d\n",r2.id);
  // printf("%lu\n", ROW_SIZE);
  // printf("%lu\n",sizeof(Row));
  return 0;
}
