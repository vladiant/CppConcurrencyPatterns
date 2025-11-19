#include <omp.h>

#include <cstdio>
#include <thread>

int main() {
  printf("Start\n");

  unsigned int n = std::thread::hardware_concurrency();
  printf("%d concurrent threads are supported.\n", n);

#pragma omp parallel
  { printf("Thread %d\n", omp_get_thread_num()); }

  printf("End\n");
  return 0;
}
