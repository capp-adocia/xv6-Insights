set architecture i386

target remote localhost:26000

file kernel
break ideinit
break scheduler

set disassembly-flavor intel
# TUI 布局
tui enable
layout src
layout regs
focus src

continue
