#include "kernel/types.h"
#include "user/user.h"

#define REND 0
#define WEND 1


/* pingpong.c: 'ping-pong' a byte between two processes over a pair of pipes, one for each direction. 
                The parent should send a byte to the child; the child should print "<pid>: received ping", 
				where <pid> is its process ID, write the byte on the pipe to the parent, and exit; 
				the parent should read the byte from the child, print "<pid>: received pong", and exit.
*/

int main() {
  int fds_p1[2];
  int fds_p2[2];
  char byte[1];

  /* child <----p1--- parent
     child ----p2----> parent
  */

  // succeed: 0, failed: -1
  if (pipe(fds_p1) < 0 || pipe(fds_p2) < 0) {
    fprintf(2, "error: pipe open failed.\n");
    exit(1);
  }

  int pid = fork();

  if (pid == 0) {
    close(fds_p1[WEND]); 
    close(fds_p2[REND]); 

	if (read(fds_p1[REND], byte, 1) == 1) {
	  printf("%d: received ping\n", getpid());

	  write(fds_p2[WEND], "x", 1);
	}

    close(fds_p1[REND]); 
    close(fds_p2[WEND]); 

  } else {
    close(fds_p1[REND]);
    close(fds_p2[WEND]);

	write(fds_p1[WEND], "x", 1);

	if (read(fds_p2[REND], byte, 1) == 1) {
	  printf("%d: received pong\n", getpid());
	}

    close(fds_p1[WEND]);
    close(fds_p2[REND]);
  }

  exit(0);
}
