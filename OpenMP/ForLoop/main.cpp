#include <omp.h>

#include <cstdio>

int main() {
  printf("Start\n");

  size_t n = 12;
  int a[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  int b[12] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120};
  int c[12]{};

#pragma omp parallel shared(n, a, b, c) default(none)
  {
// Should be only for loop
// Number of iterations should be known before loop start
// No early exits
// Only random access iterators
#pragma omp for
    for (size_t i = 0; i < n; i++) {
      c[i] = a[i] + b[i];
    }
  }

  for (auto elem : c) {
    printf("%d\n", elem);
  }

  printf("End\n");
  return 0;
}
