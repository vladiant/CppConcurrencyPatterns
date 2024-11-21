/*
writers-preference: no writer, once added to the queue, shall be kept waiting
longer than absolutely necessary
*/

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

int readcount = 0, writecount = 0, sh_var = 5;
sem_t rcount_guard, wcount_guard, rsem, wsem, p_guard;
pthread_t r[5], w[3];

void *reader(void *i) {
  sem_wait(&p_guard);
  printf("-------------------------\n");
  printf("%ld reader-%ld is reading\n", pthread_self(), (long)i);
  sem_post(&p_guard);

  sem_wait(&rsem);

  sem_wait(&rcount_guard);
  readcount++;
  if (readcount == 1) sem_wait(&wsem);
  sem_post(&rcount_guard);

  sem_post(&rsem);

  sem_wait(&p_guard);
  printf("%ld updated value :%d\n", pthread_self(), sh_var);
  sem_post(&p_guard);

  sem_wait(&rcount_guard);
  readcount--;
  if (readcount == 0) sem_post(&wsem);
  sem_post(&rcount_guard);

  return NULL;
}

void *writer(void *i) {
  sem_wait(&p_guard);
  printf("\n%ld writer-%ld is writing\n", pthread_self(), (long)i);
  sem_post(&p_guard);

  sem_wait(&wcount_guard);
  writecount++;
  if (writecount == 1) sem_wait(&rsem);
  sem_post(&wcount_guard);

  sem_wait(&wsem);
  sh_var = sh_var + 5;
  sem_post(&wsem);

  sem_wait(&wcount_guard);
  writecount--;
  if (writecount == 0) sem_post(&rsem);
  sem_post(&wcount_guard);

  return NULL;
}

int main() {
  sem_init(&rcount_guard, 0, 1);
  sem_init(&wsem, 0, 1);
  sem_init(&wcount_guard, 0, 1);
  sem_init(&rsem, 0, 1);
  sem_init(&p_guard, 0, 1);

  pthread_create(&r[0], NULL, reader, (void *)0);
  pthread_create(&w[0], NULL, writer, (void *)0);
  pthread_create(&r[1], NULL, reader, (void *)1);
  pthread_create(&r[2], NULL, reader, (void *)2);
  pthread_create(&r[3], NULL, reader, (void *)3);
  pthread_create(&w[1], NULL, writer, (void *)1);
  pthread_create(&r[4], NULL, reader, (void *)4);

  pthread_join(r[0], NULL);
  pthread_join(w[0], NULL);
  pthread_join(r[1], NULL);
  pthread_join(r[2], NULL);
  pthread_join(r[3], NULL);
  pthread_join(w[1], NULL);
  pthread_join(r[4], NULL);

  sem_destroy(&rcount_guard);
  sem_destroy(&wsem);
  sem_destroy(&wcount_guard);
  sem_destroy(&rsem);
  sem_destroy(&p_guard);

  return 0;
}
