/*
 * @Author: qiujingbao qiujingbao@qq.com
 * @Date: 2023-04-07 21:58:11
 * @LastEditors: qiujingbao “qiujingbao@dlmu.edu.cn”
 * @LastEditTime: 2023-04-11 10:35:00
 * @FilePath: /xv6-labs-2020/notxv6/barrier.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;
static int arrive_thread = 0;
struct barrier
{
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread; // Number of threads that have reached this round of the barrier
  int round;   // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

static void
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  pthread_mutex_lock(&bstate.barrier_mutex);
  arrive_thread++;
  if (nthread != arrive_thread)
  {
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    pthread_mutex_unlock(&bstate.barrier_mutex);
  }
  else
  {
    bstate.round++;
    arrive_thread=0;
    pthread_mutex_unlock(&bstate.barrier_mutex);
    pthread_cond_broadcast(&bstate.barrier_cond);  
  }
}

static void *
thread(void *xa)
{
  long n = (long)xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++)
  {
    int t = bstate.round;
    assert(i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2)
  {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for (i = 0; i < nthread; i++)
  {
    assert(pthread_create(&tha[i], NULL, thread, (void *)i) == 0);
  }
  for (i = 0; i < nthread; i++)
  {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
