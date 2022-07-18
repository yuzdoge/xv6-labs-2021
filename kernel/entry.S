	# qemu -kernel loads the kernel at 0x80000000
        # and causes each CPU to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
	# set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        la sp, stack0
        li a0, 1024*4
	csrr a1, mhartid   # R[a1] = CSR[mhartid]
        addi a1, a1, 1 # R[a1] = R[a1] + 1 = CSR[mhartid] + 1
        mul a0, a0, a1 # R[a0] = R[a0] * R[a1] = 4096 * R[a1] = 4096 * (CSR[mhartid] + 1)
        add sp, sp, a0 # R[sp] = R[sp] + R[a0] = stack0 + mhartid * 4096 +  4096, point to top of the stack, because the stack on RISC-V grows down.
	# jump to start() in start.c
        call start
spin:
        j spin
