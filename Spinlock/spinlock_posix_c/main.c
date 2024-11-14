/*
https://coffeebeforearch.github.io/2020/11/07/spinlocks-7.html
*/

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

pthread_spinlock_t sl;

void* inc(void* arg) {
  int64_t* val = (int64_t*)arg;
  for (int i = 0; i < 100000; i++) {
    pthread_spin_lock(&sl);
    *val = *val + 1;
    pthread_spin_unlock(&sl);
  }

  return NULL;
}

int main() {
  pthread_spin_init(&sl, PTHREAD_PROCESS_PRIVATE);

  int64_t val = 0;

  pthread_t t1, t2;

  pthread_create(&t1, NULL, inc, &val);
  pthread_create(&t2, NULL, inc, &val);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  printf("Sum: %ld\n", val);

  return 0;
}
