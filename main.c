#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void startothers(void);
static void mpmain(void)  __attribute__((noreturn));
extern pde_t *kpgdir;
extern char end[]; // first address after kernel loaded from ELF file

// 引导处理器从这里开始运行 C 代码。
// 首先分配一个真实的栈并切换到它，
// 同时进行内存分配器运行所需的一些设置。
int main(void)
{
  kinit1(end, P2V(4*1024*1024)); // 物理页分配器
  kvmalloc();      // 内核页表
  mpinit();        // 检测其他处理器
  lapicinit();     // 中断控制器
  seginit();       // 段描述符
  picinit();       // 禁用PIC
  ioapicinit();    // 另一个中断控制器
  consoleinit();   // 控制台硬件
  uartinit();      // 串口
  pinit();         // 进程表
  tvinit();        // 陷阱向量
  binit();         // 缓冲区缓存
  fileinit();      // 文件表
  ideinit();       // 磁盘
  startothers();   // 启动其他处理器
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // 必须在startothers()之后调用
  userinit();      // 第一个用户进程
  mpmain();        // 完成本处理器的设置
}

// 其他 CPU 从 entryother.S 跳转到这里。
static void
mpenter(void)
{
  switchkvm();
  seginit();
  lapicinit();
  mpmain();
}

// 常见的 CPU 初始化代码。
static void mpmain(void)
{
  cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
  idtinit();       // load idt register
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
  scheduler();     // start running processes
}

pde_t entrypgdir[];  // For entry.S

// 启动非引导（AP）处理器。
static void startothers(void)
{
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  uchar *code;
  struct cpu *c;
  char *stack;

  // 将入口代码写入未使用的内存地址 0x7000。
  // 链接器已将 entryother.S 的镜像放置在 _binary_entryother_start。
  code = P2V(0x7000);
  memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);

  for(c = cpus; c < cpus+ncpu; c++){
    if(c == mycpu())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = kalloc();
    *(void**)(code-4) = stack + KSTACKSIZE;
    *(void(**)(void))(code-8) = mpenter;
    *(int**)(code-12) = (void *) V2P(entrypgdir);

    lapicstartap(c->apicid, V2P(code));

    // wait for cpu to finish mpmain()
    while(c->started == 0)
      ;
  }
}

// boot 页面表在 entry.S 和 entryother.S 中使用。
// 页目录（和页表）必须从页面边界开始，
// 因此使用了 __aligned__ 属性。
// 页目录项中的 PTE_PS 启用 4M 字节页面。

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // 将 VA 的 [0, 4MB) 映射到 PA 的 [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.

