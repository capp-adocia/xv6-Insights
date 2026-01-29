// 缓冲区缓存。
//
// 缓冲区缓存是一个 buf 结构的链表，保存了磁盘块内容的缓存副本。将磁盘块缓存到内存中可以减少磁盘读取次数，同时为多个进程使用的磁盘块提供同步点。
//
// 接口：
// * 要获取特定磁盘块的缓冲区，请调用 bread。
// * 在修改缓冲区数据后，调用 bwrite 将其写入磁盘。
// * 使用完缓冲区后，调用 brelse。
// * 调用 brelse 后不要再使用该缓冲区。
// * 每次只有一个进程可以使用缓冲区，
//     因此不要长时间占用缓冲区。
//
// 实现中内部使用两个状态标志：
// * B_VALID：缓冲区数据已从磁盘读取。
// * B_DIRTY：缓冲区数据已被修改
//     需要写入磁盘。

#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // 所有缓冲区的链表，通过 prev/next 链接。
  // head.next 是最近使用的缓冲区。
  struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

//PAGEBREAK!
  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// 在缓冲缓存中查找设备 dev 上的块。
// 如果未找到，则分配一个缓冲区。
// 无论哪种情况，都返回锁定的缓冲区。
static struct buf* bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 未缓存；回收未使用的缓冲区。
  // 即使 refcnt==0，B_DIRTY 也表示缓冲区正在使用中
  // 因为 log.c 已修改它但尚未提交。
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->flags = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if((b->flags & B_VALID) == 0) {
    iderw(b);
  }
  return b;
}

// 将 b 的内容写入磁盘。必须上锁。
void bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  b->flags |= B_DIRTY;
  iderw(b); // 这里是真正的写入磁盘
}

// 释放已锁定的缓冲区。
// 移动到 MRU 列表的头部。
void brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
}
//PAGEBREAK!
// Blank page.

