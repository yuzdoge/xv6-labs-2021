# Initial process that execs /init.
# This code runs in user space.

#include "syscall.h"

# exec(init, argv)
.globl start
start:
        la a0, init // R[a0] = init, address of "init\0".
		la a1, argv // R[a1] = argv, address of argv[]
        li a7, SYS_exec // R[a7] = SYS_exec
        ecall // trap into kenerl

# for(;;) exit(); never exit.
exit:
        li a7, SYS_exit
        ecall
        jal exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2 // align to 2 bytes, .p2align: align to power of 2. 
argv:
  .long init // .long: 32-bit comma separated words. Use 4 bytes here, even if it is RV64. 
  .long 0
