#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"


#define BUFSIZE 128 

int readline(char *buf) {
  int i;
  int eof = 0;
  for (i = 0; i < BUFSIZE - 1; i++) {
    int n = read(0, &buf[i], 1);

	if (n < 1) {
	  if (i == 0) eof = 1;
	  break;
	}
	if (buf[i] == '\n' || buf[i] == '\r')
	  break;
  } 
  buf[i] = '\0';
  return eof;

}

int main(int argc, char *argv[]) {
   
  if (argc == 1) {
    // Print standinput to standoutput
    exit(0);
  }

  int exargc;
  char *exargv[MAXARG];
  char buf[BUFSIZE];

  memset(exargv, 0, MAXARG);
  
  for (exargc = 0; exargc < argc - 1; exargc++)
    exargv[exargc] = argv[exargc + 1];

  exargv[exargc] = buf;

  int eof, pid; 
  for (; ;) {
    eof = readline(buf);

	wait(0); // if no child, return -1, no error.
	
	if (eof) exit(0);

	pid = fork();

    // Child process
	if (pid == 0) {
	  exec(argv[1], exargv);
	  fprintf(2, "xargs: exec failed.\n");
	  exit(1);
	}
  }
}
