#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// 简单日志记录，允许并发的文件系统（FS）系统调用。
//
// 一次日志事务包含多个 FS 系统调用的更新。日志系统只有在没有 FS 系统调用活动时才会提交。
// 因此，永远不需要担心提交会将未提交的系统调用更新写入磁盘。
//
// 系统调用应调用 begin_op()/end_op() 来标记其开始和结束。通常 begin_op() 只是增加正在进行的 FS 系统调用的计数并返回。
// 但是，如果它认为日志快满了，它会在最后一个未完成的 end_op() 提交之前进入休眠。
//
// 日志是一个包含磁盘块的物理重做日志。
// 磁盘上的日志格式：
//   头块，包含块 A、B、C 等的块号
//   块 A
//   块 B
//   块 C
//   ...
// 日志追加是同步的。

// 头块的内容，用于磁盘上的头块
// 并用于在内存中跟踪提交前记录的块号。
/*
日志区 = 日志头 + 日志数据区
| logheader:1 | logdata | logdata | ... |
*/
struct logheader {
  int n; // 当前事务修改的块数
  int block[LOGSIZE]; // 每个被修改块的实际磁盘扇区号
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int outstanding; // how many FS sys calls are executing.
  int committing;  // in commit(), please wait.
  int dev;
  struct logheader lh;
};
struct log log;

static void recover_from_log(void);
static void commit();

void initlog(int dev)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  struct superblock sb;
  initlock(&log.lock, "log");
  readsb(dev, &sb);
  log.start = sb.logstart;
  log.size = sb.nlog;
  log.dev = dev;
  recover_from_log();
}

// Copy committed blocks from log to their home location(主存)
static void
install_trans(void)
{
  int tail;
  // 从日志数据区读出数据后,写入到指定的数据区域.
  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    brelse(lbuf);
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header
static void
read_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// 将内存中的日志头写入磁盘。这是当前事务真正提交的时刻。
static void
write_head(void)
{
  struct buf *buf = bread(log.dev, log.start); // 从日志读出数据给buf
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n; // 设置块号
  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i]; // 将日志区拷贝到hd块
  }
  bwrite(buf); // 重新写入
  brelse(buf);
}

static void
recover_from_log(void)
{
  read_head();
  install_trans(); // if committed, copy from log to disk
  log.lh.n = 0;
  write_head(); // clear the log
}

// 在每个文件系统系统调用开始时调用。
void
begin_op(void)
{
  acquire(&log.lock);
  while(1){
    if(log.committing){
      sleep(&log, &log.lock); // 触发进程调度
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
      // this op might exhaust log space; wait for commit.
      sleep(&log, &log.lock);
    } else {
      log.outstanding += 1;
      release(&log.lock);
      break;
    }
  }
}

// 在每个文件系统系统调用结束时调用。
// 如果这是最后一个未完成的操作，则提交。
void end_op(void)
{
  int do_commit = 0;

  acquire(&log.lock);
  log.outstanding -= 1;
  if(log.committing)
    panic("log.committing");
  if(log.outstanding == 0){
    do_commit = 1;
    log.committing = 1;
  } else {
    // begin_op() 可能正在等待日志空间，而减少 log.outstanding 已经减少了保留空间的数量。
    wakeup(&log);
  }
  release(&log.lock);

  if(do_commit){
    // call commit w/o holding locks, since not allowed
    // to sleep with locks.
    commit();
    acquire(&log.lock);
    log.committing = 0;
    wakeup(&log);
    release(&log.lock);
  }
}

// 将修改过的块从缓存复制到日志。
static void
write_log(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++)
  {
    struct buf *to = bread(log.dev, log.start+tail+1); // log block 这里加1是为了越过日志区头
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // write the log
    brelse(from);
    brelse(to);
  }
}

static void
commit()
{
  if (log.lh.n > 0) {
    // 将修改写入日志区
    write_log();     // 将数据写入到log的数据区
    write_head();    // 修改log的头区域,当头区域n被标记大于0时意味着可以该事务已激活
    // 将修改写回实际位置
    install_trans(); // Now install writes to home locations
    // 清理日志
    log.lh.n = 0;
    write_head();    // Erase the transaction from the log
  }
}

// 调用者已修改 b->data 并完成对缓冲区的操作。
// 使用 B_DIRTY 将块号记录到缓存并固定。
// commit()/write_log() 会执行磁盘写入。
//
// log_write() 替代了 bwrite()；一个典型的使用方式是：
//   bp = bread(...)
//   修改 bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i;

  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction");
  if (log.outstanding < 1)
    panic("log_write outside of trans");

  acquire(&log.lock);
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // log absorbtion
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n)
    log.lh.n++;
  b->flags |= B_DIRTY; // prevent eviction
  release(&log.lock);
}

