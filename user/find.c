#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"


/* find.c: Find all the files in a directory tree with a specific name. */

#define BUFSIZE 512 
char dirpath[BUFSIZE];

void 
find(char *subpath, char *tgfile) {

  if (strlen(dirpath) + 1 + DIRSIZ > BUFSIZE) {
    fprintf(2, "find: path too long\n");
	return;
  }

  struct dirent de; 
  struct stat st;

  // Concatenate two strings.
  int cursor = strlen(dirpath);
  strcpy(&dirpath[cursor], subpath);
  cursor += strlen(subpath);
  dirpath[cursor] = '\0';

  int fd = open(dirpath, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open directory '%s'\n", dirpath);
	goto ENDFUNC;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", dirpath);
	goto ENDFUNC;
  }

  if (st.type != T_DIR) {
    if (strcmp(subpath, tgfile) == 0)
      printf("%s\n", dirpath);
	goto ENDFUNC;
  }

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
	if (de.inum && strcmp(de.name, ".") && strcmp(de.name, "..")) {
      dirpath[cursor] = '/'; 
      dirpath[cursor + 1] = '\0';
      find(de.name, tgfile);
	}
  }

ENDFUNC:
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
  
  find(argv[1], argv[2]);
  exit(0);
  
}
