/*  */
struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};

// inode 的内存副本(这是对于磁盘inode上的一份拷贝)
struct inode {
  uint dev;           // 设备号
  uint inum;          // inode 号
  int ref;            // 引用计数
  struct sleeplock lock; // 保护下面的所有内容
  int valid;          // inode 是否已从磁盘读取？

  short type;         // 磁盘 inode 的副本
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};

// 表格将主要设备号映射到设备功能
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
