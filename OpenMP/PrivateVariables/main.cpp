#include <omp.h>

#include <cstdio>

int main() {
  printf("Start\n");

  int n = 12;
  int a[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  int b[12] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120};
  int c[12]{};

  size_t chunk_size;
  size_t i, i_start, i_end;

#pragma omp parallel shared(n, a, b, c) private(chunk_size, i, i_start, \
                                                    i_end) default(none)
  {
    chunk_size = n / omp_get_num_threads();

    i_start = chunk_size * omp_get_thread_num();
    i_end = chunk_size * (omp_get_thread_num() + 1);

    // Last thread
    if (i_end == chunk_size * omp_get_num_threads()) {
      i_end = n;
    }

    for (i = i_start; i < i_end; i++) {
      c[i] = a[i] + b[i];
    }
  }

  for (auto elem : c) {
    printf("%d\n", elem);
  }

  printf("End\n");
  return 0;
}
