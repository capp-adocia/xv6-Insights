// 互斥锁
struct spinlock {
  uint locked;       // 锁是否已被持有？

  // 用于调试：
  char *name;        // 锁的名称。
  struct cpu *cpu;   // 持有锁的CPU。
  uint pcs[10];      // 调用栈（一个程序计数器数组）
                     // 锁定该锁时的调用顺序。
};