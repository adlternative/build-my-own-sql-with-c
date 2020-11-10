#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
insert 18 user18 person18@example.com
insert 7 user7 person7@example.com
insert 10 user10 person10@example.com
insert 29 user29 person29@example.com
insert 23 user23 person23@example.com
insert 4 user4 person4@example.com
insert 14 user14 person14@example.com
insert 30 user30 person30@example.com
insert 15 user15 person15@example.com
insert 26 user26 person26@example.com
insert 22 user22 person22@example.com
insert 19 user19 person19@example.com
insert 2 user2 person2@example.com
insert 1 user1 person1@example.com
insert 21 user21 person21@example.com
insert 11 user11 person11@example.com
insert 6 user6 person6@example.com
insert 20 user20 person20@example.com
insert 5 user5 person5@example.com
insert 8 user8 person8@example.com
insert 9 user9 person9@example.com
insert 3 user3 person3@example.com
insert 12 user12 person12@example.com
insert 27 user27 person27@example.com
insert 17 user17 person17@example.com
insert 16 user16 person16@example.com
insert 13 user13 person13@example.com
insert 24 user24 person24@example.com
insert 25 user25 person25@example.com
insert 28 user28 person28@example.com
*/
int main(int argc, char *argv[]) {
  int arr[10] = {1, 2, 3, 4, 5, 6, 9, 10, 11, 12};
  //             0  1  2  3  4  5  6  7  8  9  10
  int one_past_max_index = 10;
  int min_index = 0;
  int key = 8;

  while (one_past_max_index !=
         min_index) { /* 结束条件：找到或min_index==one_past_max_index */

    int index = (min_index + one_past_max_index) / 2; // 5
    int key_at_index = arr[index];                    // 6
    printf("mid:%d\n", index);
    if (key_at_index == key) {
      printf("find %din %d\n", key, index);
      exit(0);
    }

    if (key <
        key_at_index) { /* arr[index]大>key 这么key的坐标在index前面或者不存在*/
      one_past_max_index =
          index; /* end缩到index,之后如果退出了,那么index=min_index,而且这个位置>key,我们找到的是个右边界，这也是我们想要插入的位置
                  */
      printf("max:%d\n", one_past_max_index);
      printf("!\n");
    } else {                 // 6<8
      min_index = index + 1; // min_index=6
      printf("min  %d\n", min_index);
      printf(">\n");
    }
  }
  printf("end in :%d\n", min_index);
  printf("not find!\n");
}