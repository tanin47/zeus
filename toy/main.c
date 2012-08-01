#include <stdio.h>
#include <stdlib.h>

int main() {
  int *a = NULL;
  printf("%d %d\n", a, &a);

  int b = 5;
  *(&a) = &b;

  printf("%d %d\n", *a, b);

  return 0;
}