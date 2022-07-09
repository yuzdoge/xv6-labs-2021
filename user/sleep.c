#include "kernel/types.h"
#include "user/user.h"

// sleep.c: Pause for a user-specified number of ticks. 
//          A tick is a notion of time defined by the xv6 kernel, 
//          namely the time between two interrupts from the timer chip.

int main(int argc, char *argv[]) {
  int n = 0;
  if (argc < 2) {
    fprintf(2, "sleep: missing operand\n");
	exit(1);
  }

  for (int i = 1; i < argc; i++)
    n += atoi(argv[i]);

  /* system call: int sleep(int) 
     implemented with assembly(user/usys.S), pass system calls number into register a7,
	 then to execute `ecall` instruction (trap into kernel space), calling 
	 the kernel function `sys_sleep`(kernel/sysproc.c).
  */
  sleep(n);

  exit(0);
}
