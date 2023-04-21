/*
 * @Author: qiujingbao qiujingbao@qq.com
 * @Date: 2023-04-11 11:26:44
 * @LastEditors: qiujingbao qiujingbao@qq.com
 * @LastEditTime: 2023-04-14 09:50:33
 * @FilePath: /xv6-labs-2020/kernel/kalloc.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#define MAX_NUM_PAGES 100 // 一次偷100页
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
struct spinlock kmemlocks[NCPU];
struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void kinit()
{
  char name[10];
  for (int i = 0; i < NCPU; i++)
  {

    snprintf(name, 10, "kmem-%d", i);
    initlock(&kmemlocks[i], name);
  }
  // initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}
// motify justice
void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  int id = 0;
  struct run *r;
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    memset(p, 1, PGSIZE);
    r = (struct run *)p;
    acquire(&kmem[id].lock);
    r->next = kmem[id].freelist;
    kmem[id].freelist = r;
    release(&kmem[id].lock);
    id = (id + 1) % NCPU;
  }
}
void *steal_pages(int cpu)
{
  int count = 0;
  struct run *start = 0;
  struct run *end = 0;
  for (int i = 0; i < NCPU; i++)
  {
    if (i == cpu)
      continue;

    acquire(&kmem[i].lock);

    start = kmem[i].freelist;
    end = kmem[i].freelist;
    if (!start)
    {
      release(&kmem[i].lock);
      continue;
    }
    while (end && count < MAX_NUM_PAGES)
    {
      end = end->next;
      count++;
    }
    if (end)
    {
      kmem[i].freelist = end->next; // 后面还有空闲内存，freelist接在后面
      end->next = 0;
    }
    else
      kmem[i].freelist = 0;
    release(&kmem[i].lock);

    acquire(&kmem[cpu].lock);
    kmem[cpu].freelist = start->next;
    release(&kmem[cpu].lock);
    break;
  }
  return (void *)start;
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;
  push_off(); // 关中断
  int id = cpuid();
  // 开中断

  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int id = cpuid();

  // 发生中断导致进程切换然后重新调度cpu核发生变化吗？

  acquire(&kmem[id].lock);

  r = kmem[id].freelist;
  if (r)
    kmem[id].freelist = r->next;
  release(&kmem[id].lock);
  pop_off();
  if (r == 0)
    r = steal_pages(id);
  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
