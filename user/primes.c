#include "kernel/types.h"
#include "user/user.h"

/* 
  The principle of prime solution with the sieve of Eratosthenes:  
  all composite numbers have the feature that it can be mutiplied 
  by primes smaller than it, which implys that a number is a prime 
  if only if it cannot be divisable by any prime smaller than it.
*/


#define REND 0
#define WEND 1

#define drop(prime, i) ((i) % (prime) == 0)

void re(int lpfds[2]) {
  int prime; 
  
  close(lpfds[WEND]);

  int flag = read(lpfds[REND], &prime, sizeof(prime)); // Only little-end

  // the last child
  if (flag == 0) {
    close(lpfds[REND]);
    return;
  }

  printf("prime %d\n", prime);

  int rpfds[2];
  pipe(rpfds); // right pipeline

  int pid = fork();

  if (pid != 0) {
    close(rpfds[REND]);

	int i;
    while (read(lpfds[REND], &i, sizeof(i))) // read() return 0 when the write-end of pipe is closed.
	  if (!drop(prime, i)) write(rpfds[WEND], &i, sizeof(i));
    
	close(rpfds[WEND]);
	close(lpfds[REND]);
	wait(0);
  } else {
    re(rpfds);
  } 

}



int main() {

  int rpfds[2];
  pipe(rpfds);

  int pid = fork();

  if (pid == 0) {
    re(rpfds);
  }
  else {
    close(rpfds[REND]);

    for (int i = 2; i <= 35; i++)
      write(rpfds[WEND], &i, sizeof(i)); // Only little-end

	close(rpfds[WEND]);
	wait(0);
  }

  exit(0);

}

