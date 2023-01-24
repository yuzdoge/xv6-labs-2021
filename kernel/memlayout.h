// Physical memory layout

// qemu -machine virt is set up like this,
// based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0 
// 10001000 -- virtio disk 
// 80000000 -- boot ROM jumps here in machine mode
//             -kernel loads the kernel here
// unused RAM after 80000000.

// the kernel uses physical memory thus:
// 80000000 -- entry.S, then kernel text and data
// end -- start of kernel page allocation area
// PHYSTOP -- end RAM used by the kernel

// qemu puts UART registers here in physical memory.
#define UART0 0x10000000L
#define UART0_IRQ 10

// virtio mmio interface
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

#ifdef LAB_NET
#define E1000_IRQ 33
#endif

// core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

// qemu puts platform-level interrupt controller (PLIC) here.
#define PLIC 0x0c000000L
// Interrupt priority increases with increasing integer values.
// Priority value of 0 disables the interrupt.
// prior = {priority value, irq}
// prior1 > prior2 <=> (prior1[0] > prior2[0]) || (prior1[0] == prior2[0] && prior[1] < prior[1])
// 4 * 1024 = 0x1000
/* IRQ is also known as Interrupt ID, Interrupt source ID. 
   IRQ 0: 0x0(reserved)
   IRQ 1: 0x4
   ...
   IRQ 1023: 0xFFC
*/
#define PLIC_PRIORITY (PLIC + 0x0)

// 1024b/8 = 0x80B
/* 0x1000: IRQ 0 to IRQ 31 pending bits
   0x1004: IRQ 32 to IRQ 63 pending bits
   ...
   0x107C: IRQ 992 to 1023 pending bits
*/
// A pending bit in the PLIC core can be cleared after performing a claim, if the associated enable bit set.
#define PLIC_PENDING (PLIC + 0x1000)

// Note: M(S)ENABLE: M(S) mode enable
// 1024b/8 = 128B = 0x80B
/* context 0 hart0 M: 0x2000
   context 1 hart0 S: 0x2080 
   context 2 hart1 M: 0x2000 + 0x100
   context 3 hart1 S: 0x2080 + 0x100
   ...
  At most 15872 contexts.
*/
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)

/*
  offset 0x0: Priority threshold for context i 
  offset 0x4: Claim/comlet for context i 
  offset 0x8: Reserved
  ...
  offset 0xFFC; Reserved
*/
// Priority threshold
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)

// Claim/complete
/* 
*Claim:
  The PLIC can perform an interrupt claim by reading the claim/complete 
  register, which returns the ID of the highest priority pending interrupt 
  or zero if there is no pending interrupt. A successful claim will also 
  atomically clear the corresponding pending bit on the interrupt source.

*Completion:
  The PLIC signals it has completed executing an interrupt handler by writing
  the interrupt ID it received from the claim to the claim/complete register.
  The PLIC does not check whether the completion ID is the same as the last 
  claim ID for that target. If the completion ID does not match an interrupt 
  source that is currently enabled for the target, the completion is silently ignored.
*/
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128*1024*1024)

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - (p)*2*PGSIZE - 3*PGSIZE)

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   USYSCALL (shared with kernel)
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)
#ifdef LAB_PGTBL
#define USYSCALL (TRAPFRAME - PGSIZE)

struct usyscall {
  int pid;  // Process ID
};
#endif
