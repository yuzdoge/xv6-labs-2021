// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

#ifdef LAB_LOCK
#define MAXKNAME 10
struct {
  struct spinlock lock;
  struct run *freelist;
  char name[MAXKNAME];
} kmems[NCPU];
#else
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
#endif

void
kinit()
{
#ifdef LAB_LOCK
  int id;
  id = cpuid();
  snprintf(kmems[id].name, 5, "kmem%d", id);
  initlock(&kmems[id].lock, kmems[id].name);
  if (id == 0)
    freerange(end, (void*)PHYSTOP);
#else
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
#endif
}

void
freerange(void *pa_start, void *pa_end)
{
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

#ifdef LAB_LOCK
  push_off();
  int id = cpuid();
  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  release(&kmems[id].lock);
  pop_off();
#else 
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
#endif
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

#ifdef LAB_LOCK
  push_off();
  int id = cpuid();
  acquire(&kmems[id].lock);
  r = kmems[id].freelist;

  if (r) {
    kmems[id].freelist = r->next; 
    release(&kmems[id].lock);
  } else {
    release(&kmems[id].lock);
    for (int i = 0; i < NCPU; i++) {     
      if (i == id) 
        continue;
      acquire(&kmems[i].lock);
      if (kmems[i].freelist) {
        r = kmems[i].freelist;
        kmems[i].freelist = r->next;
        release(&kmems[i].lock);
        break;
      }
      release(&kmems[i].lock);
    }
  }

  pop_off();
#else
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);
#endif

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
