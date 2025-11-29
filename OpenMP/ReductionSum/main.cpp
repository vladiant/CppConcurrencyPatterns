#include <omp.h>

#include <cstdio>

int main() {
  printf("Start\n");

  size_t n = 12;
  int a[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  int sum = 100;

// Available reductions: + - * & | ^ && || min max
#pragma omp parallel for shared(n, a) reduction(+ : sum) default(none)
  for (size_t i = 0; i < n; i++) {
    sum += a[i];
  }

  printf("sum: %d\n", sum);

  printf("End\n");
  return 0;
}
