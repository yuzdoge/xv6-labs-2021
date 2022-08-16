// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#ifdef LAB_COW
#define PGBASE PGROUNDUP(KERNBASE)
#define INDEX(pa) (((uint64)(pa) - PGBASE) / PGSIZE)
#endif
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

#ifdef LAB_COW
struct {
  struct spinlock lock;
  int cnt[(PHYSTOP - PGBASE) / PGSIZE];
}kmemref;
#endif

void
kinit()
{
#ifdef LAB_COW
  initlock(&kmemref.lock, "kmemref");
#endif
  initlock(&kmem.lock, "kmem");

  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
#ifdef LAB_COW
//printf("1: %d, 2: %d\n", (p - (char*)PGROUNDUP((uint64)pa_start)) / PGSIZE,  sizeof(kmem.refcnt) / sizeof(kmem.refcnt[0]));
  acquire(&kmemref.lock);
  memset(kmemref.cnt, 0, sizeof(kmemref.cnt));
  release(&kmemref.lock);
#endif
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);

}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
#ifdef LAB_COW
  int free = 0;

  acquire(&kmemref.lock);
  if (kmemref.cnt[INDEX(pa)] <= 1)
    free = 1;

  if (kmemref.cnt[INDEX(pa)] >= 1)
    kmemref.cnt[INDEX(pa)]--;

  release(&kmemref.lock);

  if (!free)
    return;
#endif
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
#ifdef LAB_COW
    //acquire(&kmemref.lock);
    kmemref.cnt[INDEX(r)] = 1;
    //release(&kmemref.lock);
#endif
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}

#ifdef LAB_COW
void
cntdec(uint64 pa)
{
  acquire(&kmemref.lock);
  if (kmemref.cnt[INDEX(pa)] > 0)
    kmemref.cnt[INDEX(pa)]--;
  release(&kmemref.lock);
}

void
cntinc(uint64 pa)
{
  acquire(&kmemref.lock);
  kmemref.cnt[INDEX(pa)]++;
  release(&kmemref.lock);
}

int
getcnt(uint64 pa)
{
  int cnt; 
  acquire(&kmemref.lock);
  cnt = kmemref.cnt[INDEX(pa)];
printf("here\n");
  release(&kmemref.lock);
  return cnt;
}

uint64
cowalloc(uint64 pa)
{
  uint64 ka;
  acquire(&kmemref.lock);
  if (kmemref.cnt[INDEX(pa)] > 1) {
    ka = (uint64)kalloc();
    if (ka) {
      kmemref.cnt[INDEX(pa)]--;
      memmove((char *)ka, (char *)pa, PGSIZE);
    }
  } else if (kmemref.cnt[INDEX(pa)] == 1) {
    ka = pa;
  } else {
    panic("cowalloc: should not be 0");
  }
  release(&kmemref.lock);
  return ka;
}
#endif
