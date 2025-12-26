# xv6 内核从这个文件开始执行。这个文件与内核 C 代码链接，因此可以引用内核符号，例如 main()。
# 启动块（bootasm.S 和 bootmain.c）会跳转到下面的 entry。

# Multiboot 头，用于像 GNU Grub 这样的 multiboot 启动加载程序。
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# 使用 GRUB 2，你可以通过复制 kernel 或 kernelmemfs 到 /boot
# 并添加以下菜单项，从 Linux 文件系统中的文件启动 xv6：
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"

# Multiboot header.  Data to direct multiboot loader.
.p2align 2
.text
.globl multiboot_header
multiboot_header:
  #define magic 0x1badb002
  #define flags 0
  .long magic
  .long flags
  .long (-magic-flags)

# 根据惯例，_start 符号指定 ELF 的入口点。
# 由于我们尚未设置虚拟内存，我们的入口点是
# 'entry' 的物理地址。
.globl _start
_start = V2P_WO(entry)

# 在启动处理器上进入 xv6，分页功能关闭。
.globl entry
entry:
  # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4
  # Set page directory
  movl    $(V2P_WO(entrypgdir)), %eax
  movl    %eax, %cr3
  # Turn on paging.
  movl    %cr0, %eax
  orl     $(CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0

  # Set up the stack pointer.
  movl $(stack + KSTACKSIZE), %esp

# 跳转到 main()，并切换到高地址执行。
# 需要间接调用，因为汇编器会为直接跳转生成相对 PC 的指令。
  mov $main, %eax
  jmp *%eax

.comm stack, KSTACKSIZE
