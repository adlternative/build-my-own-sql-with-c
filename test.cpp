#include <iostream>
#include <string>

template <typename T>
void func(T&& t){

}
void func(int &c) { int &a = c; }
int main(int argc, char const *argv[]) {
  int b = 1;
  int &a = b;
  int &c = a;
  func(std::string());
  return 0;
}
