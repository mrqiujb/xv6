#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  backtrace();
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
uint64
sys_sigalarm(void)
{
  int n;
  uint64 fun_handler;
  if (argint(0, &n) < 0)
    return -1;
  // 函数需要传入一个指针，但是这个指针必须指向一片分配好的空间
  // 一开始报错写法
  // uint64 *fun_handler=0;
  // argaddr(1,fun_handler)
  // 试图向0地址写入内容当然报错
  if (argaddr(1, &fun_handler) < 0)
    return -1;

  struct proc *p = myproc();
  //memmove(p->timer_trapframe, p->trapframe, sizeof(struct trapframe));
  p->tick_cycle = n;
  p->tick_handler=fun_handler;
  p->timer_trapframe=0;
  p->ticks=0;
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  if(p->timer_trapframe!=0)
  {
    memmove(p->trapframe, p->timer_trapframe, sizeof(struct trapframe));
    kfree(p->timer_trapframe);
    p->timer_trapframe=0;
  }
  p->ticks=0;
  return 0;
}