// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NINDIRECT (BSIZE / sizeof(uint))
#ifdef LAB_FS
#define NDIRECT 11
#define NINDIRECT2 (NINDIRECT * NINDIRECT) 
#define MAXFILE (NDIRECT + NINDIRECT + NINDIRECT2)
#else
#define NDIRECT 12
#define MAXFILE (NDIRECT + NINDIRECT)
#endif

// On-disk inode structure
struct dinode {
  short type;           // File type, see kernel/stat.h
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
#ifdef LAB_FS
  uint addrs[NDIRECT+1+1];   // Data block addresses
#else
  uint addrs[NDIRECT+1];   // Data block addresses
#endif
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block, the unit of BSIZE is byte, 
// it time 8 to convert to bit.

#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum; // inode number zero means free

  // the name is at most DIRSIZE characters, if shorter,
  // if shorter, it is terminated by a NULL.
  char name[DIRSIZ]; 
};

