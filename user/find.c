#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"


/* find.c: Find all the files in a directory tree with a specific name. */

#define BUFSIZE 512 

void 
find(char* dirpath, char* tgfile) {
  char buf[BUFSIZE];
  struct dirent de; 
  struct stat st;

  int fd = open(dirpath, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open directory '%s'\n", dirpath);
	goto ERROR;
  } 

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", dirpath);
	goto ERROR;
  }

  if (strlen(dirpath) + 1 + DIRSIZ > BUFSIZE) {
    fprintf(2, "find: path too long\n");
	goto ERROR;
  }

  strcpy(buf, dirpath);
  char *cursor = buf + strlen(dirpath);
  *cursor++ = '/'; 
  
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0) continue;

    // de.name is a string
    memcpy(cursor, de.name, DIRSIZ);

	if (stat(buf, &st) < 0) {
	  fprintf(2, "find: cannot stat %s\n");
	  goto ERROR;
	}
	
    switch(st.type) {
      case T_FILE:
	    if (strcmp(de.name, tgfile) == 0) {
          printf("%s\n", buf);
		}
	    break;
	  case T_DIR:
	    if (strcmp(de.name, ".") && strcmp(de.name, "..")) {
		  // Do not search "." and ".."
		  find(buf, tgfile);
		}
		break;
    }  
  }

  close(fd);
  return;

ERROR:
  close(fd);
  return;

}

int 
main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(2, "find: there must be only two parameters. usage: find <dir path> <file name>\n");
    exit(1);
  }

  if (strlen(argv[2]) > DIRSIZ) {
    fprintf(2, "find: target filename too long\n");
	exit(1);
  }
  
  // argv[1] must be a directory.
  find(argv[1], argv[2]);
  exit(0);
  
}
