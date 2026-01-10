#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

struct stat {
  short type;  // 文件类型
  int dev;     // 文件系统的磁盘设备
  uint ino;    // i节点编号
  short nlink; // 文件的链接数量
  uint size;   // 文件大小（字节）
};
