#define O_RDONLY  0x000 // 2'b0000 0000 0000
#define O_WRONLY  0x001 // 2'b0000 0000 0001
#define O_RDWR    0x002 // 2'b0000 0000 0010
// to create the file if doesn't exit.
#define O_CREATE  0x200 // 2'b0010 0000 0000
// to truncate the file to zero length.
#define O_TRUNC   0x400 // 2'b0100 0000 0000
