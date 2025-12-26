#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
	
# 每个非引导 CPU（"AP"）是在引导 CPU 发送 STARTUP IPI 响应下启动的。
# 《多处理器规范》的 B.4.2 节指出，AP 将以实模式启动，
# CS:IP 设置为 XY00:0000，其中 XY 是随 STARTUP 一起发送的 8 位值。
# 因此，这段代码必须从 4096 字节对齐的地址开始。
#
# 因为这段代码将 DS 设置为零，它必须位于低 2^16 字节的地址中。
#
# Startothers（在 main.c 中）一次发送一个 STARTUP。
# 它将这段代码（start）复制到 0x7000。
# 它把新分配的每个核心的栈地址放在 start-4，
# 跳转目标的地址（mpenter）放在 start-8，
# 以及 entrypgdir 的物理地址放在 start-12。
#
# 这段代码结合了 bootasm.S 和 entry.S 的元素。

.code16           
.globl start
start:
  cli            

  # Zero data segment registers DS, ES, and SS.
  xorw    %ax,%ax
  movw    %ax,%ds
  movw    %ax,%es
  movw    %ax,%ss

  # Switch from real to protected mode.  Use a bootstrap GDT that makes
  # virtual addresses map directly to physical addresses so that the
  # effective memory map doesn't change during the transition.
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE, %eax
  movl    %eax, %cr0

  # 通过使用长跳转（long jmp）重新加载 %cs 和 %eip，完成向 32 位保护模式的过渡。
  # 段描述符已设置为无转换，因此映射仍然是直接映射。
  ljmpl    $(SEG_KCODE<<3), $(start32)

//PAGEBREAK!
.code32  # Tell assembler to generate 32-bit code now.
start32:
  # Set up the protected-mode data segment registers
  movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %ss                # -> SS: Stack Segment
  movw    $0, %ax                 # Zero segments not ready for use
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS

  # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4
  # Use entrypgdir as our initial page table
  movl    (start-12), %eax
  movl    %eax, %cr3
  # Turn on paging.
  movl    %cr0, %eax
  orl     $(CR0_PE|CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0

  # Switch to the stack allocated by startothers()
  movl    (start-4), %esp
  # Call mpenter()
  call	 *(start-8)

  movw    $0x8a00, %ax
  movw    %ax, %dx
  outw    %ax, %dx
  movw    $0x8ae0, %ax
  outw    %ax, %dx
spin:
  jmp     spin

.p2align 2
gdt:
  SEG_NULLASM
  SEG_ASM(STA_X|STA_R, 0, 0xffffffff)
  SEG_ASM(STA_W, 0, 0xffffffff)


gdtdesc:
  .word   (gdtdesc - gdt - 1)
  .long   gdt

