// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#ifdef LAB_LOCK
extern struct spinlock tickslock;

#define MAXBUCKET 13
#define NEWNBUF (((NBUF / MAXBUCKET) + 1) * MAXBUCKET)

#define hash(i) (i % MAXBUCKET)

struct bucket {
  struct spinlock lock;
  struct buf head;
};

#endif

struct {
  struct spinlock lock;

#ifdef LAB_LOCK
  struct buf buf[NEWNBUF];
  struct bucket bucket[MAXBUCKET];
#else
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
#endif
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

#ifdef LAB_LOCK
  for (uint i = 0; i < MAXBUCKET; i++) {
    initlock(&bcache.bucket[i].lock, "bcache.bucket"); 
    bcache.bucket[i].head.prev = &bcache.bucket[i].head;
    bcache.bucket[i].head.next = &bcache.bucket[i].head;
    for (uint j = 0; j < (NEWNBUF / MAXBUCKET); j++) {
      b = &bcache.buf[i * (NEWNBUF / MAXBUCKET) + j];
      b->next = bcache.bucket[i].head.next;
      b->prev = &bcache.bucket[i].head;
      initsleeplock(&b->lock, "buffer");
      bcache.bucket[i].head.next->prev = b;
      bcache.bucket[i].head.next = b;
    }
  } 
#else
  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
#endif
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

#ifdef LAB_LOCK
  uint bkt = hash(blockno); 
  if (bkt >= MAXBUCKET)
    panic("bget: bkt >= MAXBUCKET");

  struct buf *lrubuf = 0;
  uint lr = -1; // max value

  acquire(&bcache.bucket[bkt].lock);

  for (b = bcache.bucket[bkt].head.next; b != &bcache.bucket[bkt].head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.bucket[bkt].lock);
      acquiresleep(&b->lock);
      return b;
    }
    if (b->refcnt == 0 && b->timestamp <= lr) {
      lrubuf = b;
      lr = b->timestamp;
    }
  }
  
  if (!lrubuf) {
    // It can not release(&bcache.bucket[bkt].lock) here, 
    // we must hold the lock for allocating a block atomically.
    // Because a disk bock on the buffer cache is unique.

    for (uint i = (bkt + 1) % MAXBUCKET, j = 1; j < MAXBUCKET; i = (i + 1) % MAXBUCKET, j++) {
      if (holding(&bcache.bucket[i].lock))
        continue;
      acquire(&bcache.bucket[i].lock);
      for (b = bcache.bucket[i].head.next; b != &bcache.bucket[i].head; b = b->next) {
        if (b->refcnt == 0 && b->timestamp <= lr) {
          lrubuf = b;
          lr = b->timestamp;
        }
      }
      if (lrubuf) {
        lrubuf->prev->next = lrubuf->next;
        lrubuf->next->prev = lrubuf->prev;

        lrubuf->next = bcache.bucket[bkt].head.next;
        lrubuf->prev = &bcache.bucket[bkt].head;
        bcache.bucket[bkt].head.next->prev = lrubuf;
        bcache.bucket[bkt].head.next = lrubuf;
        release(&bcache.bucket[i].lock);
        break;
      }
      release(&bcache.bucket[i].lock);
    }
  }

  if (lrubuf) {
    lrubuf->dev = dev;  
    lrubuf->blockno = blockno;
    lrubuf->valid = 0;
    lrubuf->refcnt = 1;
    release(&bcache.bucket[bkt].lock);
    acquiresleep(&lrubuf->lock);
    return lrubuf;
  }

#else
  acquire(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
#endif
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
#ifdef LAB_LOCK
  uint bkt = hash(b->blockno);
#endif
  releasesleep(&b->lock);

#ifdef LAB_LOCK
  acquire(&bcache.bucket[bkt].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    acquire(&tickslock);
    b->timestamp = ticks;
    release(&tickslock);
  } 
  release(&bcache.bucket[bkt].lock);
#else
  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  release(&bcache.lock);
#endif
}

void
bpin(struct buf *b) {
#ifdef LAB_LOCK
  uint bkt = hash(b->blockno);
  acquire(&bcache.bucket[bkt].lock);
  b->refcnt++;
  release(&bcache.bucket[bkt].lock);
#else
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
#endif
}

void
bunpin(struct buf *b) {
#ifdef LAB_LOCK
  uint bkt = hash(b->blockno);
  acquire(&bcache.bucket[bkt].lock);
  b->refcnt--;
  release(&bcache.bucket[bkt].lock);
#else
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
#endif
}
