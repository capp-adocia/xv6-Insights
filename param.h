#define NPROC        64  // 最大进程数
#define KSTACKSIZE 4096  // 每个进程内核栈的大小
#define NCPU          8  // 最大CPU数量
#define NOFILE       16  // 每个进程可打开的文件数
#define NFILE       100  // 系统可打开的文件数
#define NINODE       50  // 最大活动i节点数
#define NDEV         10  // 最大主设备号
#define ROOTDEV       1  // 文件系统根磁盘的设备号
#define MAXARG       32  // 最大执行参数数
#define MAXOPBLOCKS  10  // 文件系统操作可写的最大块数
#define LOGSIZE      (MAXOPBLOCKS*3)  // 磁盘日志中的最大数据块数
#define NBUF         (MAXOPBLOCKS*3)  // 磁盘块缓存大小
#define FSSIZE       1000  // 文件系统大小（块数）

