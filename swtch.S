# 上下文切换
#
#   void swtch(struct context **old, struct context *new);
#
# 将当前寄存器保存到栈上，创建一个 struct context，并将其地址保存在 *old。
# 切换栈到 new 并弹出之前保存的寄存器。

.globl swtch
swtch:
  movl 4(%esp), %eax
  movl 8(%esp), %edx

  # Save old callee-saved registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
  movl %esp, (%eax)
  movl %edx, %esp

  # Load new callee-saved registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret # 只有切换到新进程时才会跳转到forkret去执行，其余情况都是跳转到新进程的之前保存的eip
