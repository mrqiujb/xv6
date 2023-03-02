#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void)
{
  initlock(&tickslock, "time");
}
// 由用户空间转入kernel的时候 将stvec的值设为kernnel的stvec的值
//  set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

// 处理中断，异常和系统调用。由uservec调用 在trampoline.s
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
// 从用户空间发生的trap 可以是 syscall exception interrupt
void usertrap(void)
{
  int which_dev = 0;
  // 先检查一下此前的模式是否是用户模式
  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  // 将stvec寄存器的值设为kernelvec的值
  w_stvec((uint64)kernelvec);

  // 获得当前进程
  struct proc *p = myproc();

  // save user program counter.
  // 保存程序计数器通过这个trapframe 程序调度通过timer实现 也是一种trap
  p->trapframe->epc = r_sepc();

  // 检查一下scause寄存器 这个寄存器存放着引发trap的原因
  // 如果是8则说明是系统调用
  if (r_scause() == 8)
  {
    // system call

    if (p->killed)
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();

    syscall();
  }
  // devintr函数返回 2是timer 1是interrupt 如果是0 则是未识别 如果等于0 不会进入当前代码
  // 而是执行else的语句 报错退出
  else if ((which_dev = devintr()) != 0)
  {
    // ok
  }
  else
  {
    // 程序错误 用户程序错误则杀死该进程 内核错误系统崩溃
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if (p->killed)
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  // 如果是timer 则yield
  if (which_dev == 2)
  {
    if (p->tick_cycle > 0)
    {

      p->ticks++;
      if (p->ticks == p->tick_cycle)
      {
        if (p->timer_trapframe==0)
        {
          p->timer_trapframe=kalloc();
          memmove(p->timer_trapframe, p->trapframe, sizeof(struct trapframe));
          p->trapframe->epc = p->tick_handler;
        }
      }
    }
    yield();
  }
  // 最后调用此函数 注意此时trap的处理已经完成 属于中断返回的过程中了
  usertrapret();
}

//
// return to user space
// 返回到了用户空间之前所需要做的工作
void usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  // 将trap的目的地切换从kerneltrap切换到 usertrap，所以关中断直到返回用户空间，usertrap是正确的

  // 关中断
  //
  intr_off();
  // 将系统调用 中断 异常 发送到trampoline.S
  //  send syscalls, interrupts, and exceptions to trampoline.S
  //  重新设置stvec寄存器 指向uservec
  //  观察上述流程代码 stvec的值没有代码改动
  //  但是考虑多个中断的情况 stvec的值可能发生变化 所以重写一遍
  //  关了中断才敢这样写 不然就可能被改了
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // uservec需要设置trapframe的值 当进程下一次进入内核的时候
  // 此时去设置这个trapframe 那么下次再次进入的时候就可以在uservec中使用
  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack p->kstack是基地址 而sp在执行中总是高地址向低地址执行
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // 恢复sepc的值从trapframe中
  //  set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);
  // 获取用户页表，作为参数传入userret并恢复
  // 只能在trampline中恢复 因为只有哪里的代码在内核和用户中有相同的映射
  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // 调用函数userret
  // jump to trampoline.S at the top of memory, which
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64, uint64))fn)(TRAPFRAME, satp); // TRAPFRAME a0寄存器 satp a1 寄存器
}

// 处理两种 一个是设备中断 一个是异常exception
//  interrupts and exceptions from kernel code go here via kernelvec,
//  on whatever the current kernel stack is.
void kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if (intr_get() != 0)
    panic("kerneltrap: interrupts enabled");
  // 使用devintr函数处理dervice interrupt
  if ((which_dev = devintr()) == 0)
  { // 返回0代表是未识别也就是exception 需要panic
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }
  // which_dev由devintr赋值，如果是2代表是timer中断
  //  give up the CPU if this is a timer interrupt.
  // 如果是timer中断则就当前进程放弃cpu
  // 不能是调度器也就是scheduler
  if (which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int devintr()
{
  uint64 scause = r_scause();

  if ((scause & 0x8000000000000000L) &&
      (scause & 0xff) == 9)
  {
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if (irq == UART0_IRQ)
    {
      uartintr();
    }
    else if (irq == VIRTIO0_IRQ)
    {
      virtio_disk_intr();
    }
    else if (irq)
    {
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if (irq)
      plic_complete(irq);

    return 1;
  }
  else if (scause == 0x8000000000000001L)
  {
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if (cpuid() == 0)
    {
      clockintr();
    }

    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  }
  else
  {
    return 0;
  }
}
