#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/* Possible states of a thread: */
// 可能用到的状态 free时unused runing是执行 runable 可执行
// 想象成没有内核 用户独占所有时间空间，中断不存在
// 函数间的返回仅仅依靠调用
#define FREE 0x0
#define RUNNING 0x1
#define RUNNABLE 0x2

#define STACK_SIZE 8192
#define MAX_THREAD 4
struct context
{
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

struct thread
{
  char stack[STACK_SIZE]; /* the thread's stack */
  int state;              /* FREE, RUNNING, RUNNABLE */
  char *name;
  struct context context;
};
// 所有进程
struct thread all_thread[MAX_THREAD];
// 当前执行的进程
struct thread *current_thread;
// 进程切换函数
extern void thread_switch(uint64, uint64);

// 进程初始化
void thread_init(void)
{
  // main() is thread 0, which will make the first invocation to
  // thread_schedule().
  // it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  // 第一个线程是main
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
  current_thread->name="init";
}

void thread_schedule(void)
{
  struct thread *t, *next_thread;

  /* Find another runnable thread. */
  next_thread = 0;
  t = current_thread + 1;
  for (int i = 0; i < MAX_THREAD; i++)
  {
    // 如果当前指针越界，那么就选择初始main线程
    // 所以此调度规则是一种fifo式的调度
    if (t >= all_thread + MAX_THREAD)
      t = all_thread;
    // 如果能找到 那么久选择
    if (t->state == RUNNABLE)
    {
      next_thread = t;
      break;
    }
    t = t + 1;
  }
  // 找不到可执行的进程 报错结束
  if (next_thread == 0)
  {
    printf("thread_schedule: no runnable threads\n");
    exit(-1);
  }
  // 进程切换
  if (current_thread != next_thread)
  { /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    
    thread_switch((uint64)(&(t->context)), (uint64)(&(current_thread->context)));
  }
  else
    next_thread = 0;
}

void thread_create(void (*func)(),char * name)
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++)
  {
    if (t->state == FREE)
      break;
  }
  t->name=name;
  t->state = RUNNABLE;
  // YOUR CODE HERE
  // 当前函数栈
  t->context.sp = (uint64)(t->stack+STACK_SIZE);
  t->context.ra = (uint64)func;
}

void thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule();
}

volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void thread_a(void)
{
  int i;
  printf("thread_a started\n");
  a_started = 1;
  while (b_started == 0 || c_started == 0)
    thread_yield();

  for (i = 0; i < 100; i++)
  {
    printf("thread_a %d\n", i);
    a_n += 1;
    thread_yield();
  }
  printf("thread_a: exit after %d\n", a_n);

  current_thread->state = FREE;
  thread_schedule();
}

void thread_b(void)
{
  int i;
  printf("thread_b started\n");
  b_started = 1;
  while (a_started == 0 || c_started == 0)
    thread_yield();

  for (i = 0; i < 100; i++)
  {
    printf("thread_b %d\n", i);
    b_n += 1;
    thread_yield();
  }
  printf("thread_b: exit after %d\n", b_n);

  current_thread->state = FREE;
  thread_schedule();
}

void thread_c(void)
{
  int i;
  printf("thread_c started\n");
  c_started = 1;
  while (a_started == 0 || b_started == 0)
    thread_yield();

  for (i = 0; i < 100; i++)
  {
    printf("thread_c %d\n", i);
    c_n += 1;
    thread_yield();
  }
  printf("thread_c: exit after %d\n", c_n);

  current_thread->state = FREE;
  thread_schedule();
}

int main(int argc, char *argv[])
{
  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a,"a");
  thread_create(thread_b,"b");
  thread_create(thread_c,"c");
  thread_schedule();
  exit(0);
}
