#include <omp.h>

#include <cstdio>

int main() {
  printf("Start\n");

  size_t n = 12;
  int a[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  int min = 100;

#pragma omp parallel for shared(n, a) reduction(min : min) default(none)
  for (size_t i = 0; i < n; i++) {
    if (a[i] < min) min = a[i];
  }

  printf("min: %d\n", min);

  printf("End\n");
  return 0;
}
