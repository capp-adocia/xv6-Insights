// 内存布局

#define EXTMEM  0x100000            // 扩展内存起始地址
#define PHYSTOP 0xE000000           // 物理内存顶端
#define DEVSPACE 0xFE000000         // 其他设备位于高地址

// 地址空间布局的关键地址（布局见 vm.c 中的 kmap）
#define KERNBASE 0x80000000         // 第一个内核虚拟地址
#define KERNLINK (KERNBASE+EXTMEM)  // 内核链接地址

#define V2P(a) (((uint) (a)) - KERNBASE)
#define P2V(a) ((void *)(((char *) (a)) + KERNBASE))

#define V2P_WO(x) ((x) - KERNBASE)    // 与 V2P 相同，但没有类型转换
#define P2V_WO(x) ((x) + KERNBASE)    // 与 P2V 相同，但没有类型转换