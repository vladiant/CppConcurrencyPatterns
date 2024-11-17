/*
https://wiki.osdev.org/Spinlock
*/

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

atomic_flag sl = ATOMIC_FLAG_INIT;

void acquire(atomic_flag* lock) {
  while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
    /* use whatever is appropriate for your target arch here */
    __builtin_ia32_pause();
  }
}

void release(atomic_flag* lock) {
  atomic_flag_clear_explicit(lock, memory_order_release);
}

void* inc(void* arg) {
  int64_t* val = (int64_t*)arg;
  for (int i = 0; i < 100000; i++) {
    acquire(&sl);
    *val = *val + 1;
    release(&sl);
  }

  return NULL;
}

int main() {
  int64_t val = 0;

  pthread_t t1, t2;

  pthread_create(&t1, NULL, inc, &val);
  pthread_create(&t2, NULL, inc, &val);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  printf("Sum: %ld\n", val);

  return 0;
}
