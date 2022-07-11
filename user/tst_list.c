
#include "kernel/types.h"
#include "user/user.h"

// list.c: list file names in the current directory

// directory entry, also defined in kernel/fs.h
struct dirent {
  ushort inum;
  char name[14];
};

int
main()
{
  int fd;
  struct dirent e;

  fd = open(".", 0);
  // directory is a file containing a sequence of dirent structures.
  while(read(fd, &e, sizeof(e)) == sizeof(e)){
    if(e.name[0] != '\0'){
      printf("%s\n", e.name);
    }
  }
  exit(0);
}
