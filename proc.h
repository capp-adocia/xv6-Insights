// 每个 CPU 的状态
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // 在这里使用 swtch() 进入调度器
  struct taskstate ts;         // x86 用于查找中断栈
  struct segdesc gdt[NSEGS];   // x86 全局描述符表
  volatile uint started;       // CPU 是否已启动？
  int ncli;                    // pushcli 嵌套的深度。
  int intena;                  // 在 pushcli 前中断是否已启用？
  struct proc *proc;           // 当前运行在此 CPU 上的进程或为空
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//分页: 17
// 为内核上下文切换保存的寄存器。
// 不需要保存所有的段寄存器（%cs 等），
// 因为它们在内核上下文中是固定的。
// 不需要保存 %eax、%ecx、%edx，因为
// x86 的约定是调用者已经保存了它们。
// 上下文存储在它们所描述的栈的底部；
// 栈指针就是该上下文的地址。
// 上下文的布局与 swtch.S 中“切换栈”注释处的栈布局相匹配。
// 切换并不会显式保存 eip，
// 但它在栈上，allocproc() 会操作它。
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 每个进程的状态
struct proc {
  uint sz;                     // 进程内存大小（字节）
  pde_t* pgdir;                // 页表
  char *kstack;                // 该进程内核栈的底部
  enum procstate state;        // 进程状态
  int pid;                     // 进程ID
  struct proc *parent;         // 父进程
  struct trapframe *tf;        // 当前系统调用的陷阱帧
  struct context *context;     // 在这里调用 swtch() 来运行进程
  void *chan;                  // 如果非零，则在 chan 上睡眠
  int killed;                  // 如果非零，表示已被杀死
  struct file *ofile[NOFILE];  // 打开的文件
  struct inode *cwd;           // 当前目录
  char name[16];               // 进程名称（调试用）
};

// 进程内存是连续分布的，低地址优先：
//   代码段
//   初始数据段和 bss 段
//   固定大小的栈
//   可扩展的堆