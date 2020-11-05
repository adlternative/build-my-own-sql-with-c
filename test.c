#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

void func(int *ch) { free(ch); }

int main(int argc, char *argv[]){

  int *str=malloc(1024*sizeof(int));
  // free(str);
  func(str);
  return 0;
}