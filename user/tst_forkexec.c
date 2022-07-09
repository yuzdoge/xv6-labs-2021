#include "kernel/types.h"
#include "user/user.h"

int
main() {
  int pid, status;
  pid = fork();

  if (pid == 0) {
    char *argv[] = {"echo", "This", "is", "echo", 0};
	//exec("echo", argv); //it will not execute the next two line if successful. 
	exec("echo_error", argv);
	printf("exec failed!\n");
	exit(1);
  }
  else {
    printf("parent waiting\n");
    wait(&status);
    printf("the child exited with status %d\n", status); 
  }
  exit(0);
}
