#include "kernel/types.h"
#include "user/user.h" 
#include "kernel/fcntl.h"

int
main() {
  char str[] = "Hello xv6!\n";
  int fd = open("output.txt", O_WRONLY | O_CREATE);
  write(fd, str, sizeof(str));
  exit(0);
}
