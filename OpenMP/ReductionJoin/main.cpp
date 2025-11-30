#include <omp.h>

#include <cstdio>

int main() {
  printf("Start\n");

  size_t n = 12;
  int a[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  int sum = 0;

#pragma omp parallel shared(n, a, sum) default(none)
  {
    int sum_priv = 0;
#pragma omp for
    for (size_t i = 0; i < n; i++) {
      sum_priv += a[i];
    }
#pragma omp atomic
    sum += sum_priv;
  }

  printf("sum: %d\n", sum);

  printf("End\n");
  return 0;
}
