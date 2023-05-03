# Initial process that execs /init.
# This code runs in user space.

#include "syscall.h"

# exec(init, argv)
.globl start
start:
        la a0, init #将init放入a0
        la a1, argv #将argv放入a1
        li a7, SYS_exec #系统调用号放入a7
        #系统调用号匹配kernel/syscall.c的static uint64 (*syscalls[])(void) 函数

        ecall
        #ecall调用uservec -》 usertrap -》 syscall
        #syscall function 根据a7选择对应的函数执行
        #syscall 函数 将执行函数的返回值放入a0 负数代表失败，正数或0时成功
# for(;;) exit();
exit:
        li a7, SYS_exit
        ecall
        jal exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0
