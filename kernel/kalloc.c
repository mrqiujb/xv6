// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
// 32*1024 * 8BIT=32KB 这个东西占32KB内存
// 给定一个pa 返回这个pa所在页的对应的refer的索引
// 应该是(128 * 1024 * 1024) / (4 * 1024)=32768
// 又因为kernel text kernel data占据一部分 end 所以是

#define NPAGE 32723
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

char reference[NPAGE];

int getrefindex(void *pa)
{
  int index = ((char *)pa - (char *)PGROUNDUP((uint64)end)) / PGSIZE;
  return index;
}

int getref(void *pa)
{
  return reference[getrefindex(pa)];
}

void addref(void *pa)
{
  reference[getrefindex(pa)]++;
}

void subref(void *pa)
{
  int index = getrefindex(pa);
  if (reference[index] == 0)
    return;
  reference[index]--;
}
void initref(void *pa)
{
  reference[getrefindex(pa)] = 0;
}
void kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    initref(p);
    kfree(p);
  }
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
  subref((void *)pa); // 因为是kfree 所以先减少，在判断是否用真实释放即可。
  // 如果先判断再减少，例如本来是1 判断通过不真实释放，然后减少至0，然后最终以0引用等待下一次调用kfree则永远不会有人调用，这块内存将永远不可用。
  if (getref(pa) == 0)
  {
    memset(pa, 1, PGSIZE);
    r = (struct run *)pa;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r)
  {
    memset((char *)r, 5, PGSIZE); // fill with junk
    int index = getrefindex((void *)r);
    reference[index] = 1;           // 分配的时候增加引用
  }
  return (void *)r;
}
