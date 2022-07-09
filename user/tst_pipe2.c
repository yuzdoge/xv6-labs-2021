

#include "kernel/types.h"
#include "user/user.h"

// pipe2.c: communication between two processes

int
main()
{
  int n, pid;
  int fds[2];
  char buf[100];
  
  // create a pipe, with two FDs in fds[0], fds[1].
  pipe(fds);

  pid = fork();
  if (pid == 0) {
    for (int i = 0; i < 10; i++)
      write(fds[1], "this is pipe2\n", 14);
  } else {
    close(fds[1]); // if not close write-end, the read will block.
    while ((n = read(fds[0], buf, sizeof(buf))) != 0)
      write(1, buf, n);
  }

  exit(0);
}
