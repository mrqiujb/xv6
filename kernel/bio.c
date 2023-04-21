// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define HASH_TABLE_SIZE 13
#define hash_map(devno) devno % HASH_TABLE_SIZE
struct
{
  struct buf buf[NBUF];
  struct spinlock buket[HASH_TABLE_SIZE];
  // 13个头节点 更改双向链表的方式
  // 变为13个单链表并行的方式
  struct buf head[HASH_TABLE_SIZE];
  // 存储未使用的cache
  struct buf blank_head;
  struct spinlock block;

} bcache;
void
binit(void)
{
  struct buf *b;
  initlock(&(bcache.block),"blank_head_lock");
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    char name[10] = {'\0'};
    snprintf(name, 10, "bcache-%d", i);
    initlock(&(bcache.buket[i]),name);
  }

  bcache.blank_head.next=&bcache.buf[0];
  for (b = bcache.buf; b < bcache.buf+NBUF-1; b++) { //attion NBUF-1 可能导致下面的代码不能用 卡在了bget 
    b->next = b+1;
    initsleeplock(&b->lock, "buffer");
  }
  initsleeplock(&b->lock, "buffer");
}
//初始化cache
void init_cache(struct buf * b,uint dev,uint blockno)
{
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->time = ticks;
}
// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  int lock_num = hash_map(blockno);
  struct buf *b;

  acquire(&(bcache.buket[lock_num]));
  // Is the block already cached?
  // 如果在这返回说明命中
  for (b = bcache.head[lock_num].next; b; b = b->next) // 单链表 b不为空
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      // 更新最近使用时间
      b->time = ticks;
      // 更新引用计数
      b->refcnt++;
      // release(&bcache.lock);
      release(&bcache.buket[lock_num]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // 未命中 则需要新的未使用的cache快
  // 先去空闲cache管理的链表中找一个
  
  acquire(&(bcache.block));
  if (bcache.blank_head.next)
  {
    struct buf *take= bcache.blank_head.next;
    bcache.blank_head.next=take->next;

    //将当前cache 挂到对应的管理链表中
    struct buf *pre=bcache.head[lock_num].next;//头节点
    bcache.head[lock_num].next=take;
    take->next=pre;

    init_cache(take,dev,blockno);
    
    release(&(bcache.block));
    release(&bcache.buket[lock_num]);
    acquiresleep(&take->lock);
    return take;
  }

  // 未命中 且 没有空闲的cache 都在使用
  // 所以去所有的cache中找到最近未使用
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
 
  

  panic("bget: no buffers");
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int lock_num = hash_map(b->blockno);

  acquire(&bcache.buket[lock_num]);
  b->refcnt--;
  //printf("brelase block no %d\n",b->blockno);
  if (b->refcnt == 0)
  {
    acquire(&(bcache.block));
    struct buf *i;
    struct buf *pre=&bcache.head[lock_num];//头节点
    for (i = bcache.head[lock_num].next; b!=i; i = i->next)//如果空则头节点直接指向take
    {
      pre=i;
    }
    pre->next=i->next;
    //头插入到blank_head
    pre=bcache.blank_head.next;
    bcache.blank_head.next=b;
    b->next=pre;

    release(&(bcache.block));
  }
  release(&bcache.buket[lock_num]);
  // release(&bcache.lock);
}

// 表示b这个快被在外部引用 log.c中
/*
chat gpt answer

When a block device driver reads or writes data to a block device, it typically does so by transferring data between a buffer in memory and a block on the disk. The bpin function is used to indicate to the buffer cache system that a particular buffer needs to be pinned in memory, which means that it should not be evicted (or removed) from memory by the buffer cache system until it is explicitly unpinned by a call to bunpin.

The bpin function takes a single argument, which is a pointer to a buffer that is already in the buffer cache. It sets the B_BUSY and B_PINNED flags of the buffer's flags field to indicate that the buffer is currently in use and should be pinned in memory. The B_BUSY flag prevents other processes from using the buffer, while the B_PINNED flag prevents the buffer cache system from evicting the buffer from memory.
*/
void bpin(struct buf *b)
{
  int lock_num=hash_map(b->blockno);
  acquire(&bcache.buket[lock_num]);
  b->refcnt++;
  release(&bcache.buket[lock_num]);
}

void bunpin(struct buf *b)
{
  int lock_num=hash_map(b->blockno);
  acquire(&bcache.buket[lock_num]);
  b->refcnt--;
  release(&bcache.buket[lock_num]);
}
/*
  外部接口，应该不用修改
*/

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  // valid 表面此cache是否被缓存
  // 如果没有被缓存，则缓存他
  // bget分配一个cache给他
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}