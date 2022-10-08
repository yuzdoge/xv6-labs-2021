struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;    // device number
  uint blockno; // block number

  // sleeplock only protects reads and writes of the data filed while 
  // the other fileds are protected by `bcache.lock`.
  struct sleeplock lock;  
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE]; // BSIZE is defined in kernel/fs.h
};

