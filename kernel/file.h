struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable; // if the file is readalbe
  char writable; // if the file is writable
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// in-memory copy of an inode
struct inode {
  // (dev, inum) indentifies a inode in memory uniquely
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count, if ref is 0, the inode in italbe can be evicted/replaced.

  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk(is the inode in itable valid)?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1]; // Direct blocks and a indirect block
};

// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
