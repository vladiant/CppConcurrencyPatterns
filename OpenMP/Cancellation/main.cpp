#include <omp.h>

#include <cstdio>

int main() {
  printf("Start\n");

  size_t n = 12;
  int a[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  int val = 9;
  int idx = -1;

#pragma omp parallel for shared(n, a, val, idx) default(none)
  for (size_t i = 0; i < n; i++) {
    if (a[i] == val) {
// export OMP_CANCELLATION=true
#pragma omp critical
      idx = val;
#pragma omp cancel for
    }
#pragma omp cancellation point for
  }

  printf("idx: %d\n", idx);

  printf("End\n");
  return 0;
}
