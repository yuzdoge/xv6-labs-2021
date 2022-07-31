#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"

/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable; // pagetable_t: typedef in kernel/riscv.h 

extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

// Make a direct-map page table for the kernel.
// It must execute with paging turned off, because the pagetable has been no existed currently.
pagetable_t
kvmmake(void)
{
  pagetable_t kpgtbl;

  // allocates a page of physical memory to hold the root page-table page. 
  kpgtbl = (pagetable_t) kalloc(); 
  memset(kpgtbl, 0, PGSIZE);

  // uart registers
  kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W); // pa(UART0) = UART0

  // virtio mmio disk interface
  kvmmap(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // PLIC, Platform Level Interrupt Controller
  kvmmap(kpgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W); // 0x400000 = 4M = 1024 * 4K 

  // map kernel text executable and read-only.
  kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X); // etext is defined in kernel/kernel.ld

  // map kernel data and the physical RAM we'll make use of.
  kvmmap(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X); // pa(TRAMPOLINE) = trampoline, trampoline is defined in kernel/trampoline.S

  // map kernel stacks
  proc_mapstacks(kpgtbl); // proc_mapstacks() is defined in kernel/proc.c

  
  return kpgtbl;
}

// Initialize the one kernel_pagetable
void
kvminit(void)
{
  kernel_pagetable = kvmmake();
}

// Switch h/w page table register to the kernel's page table,
// and enable paging.
void
kvminithart()
{ 
  // see kernel/riscv.h
  w_satp(MAKE_SATP(kernel_pagetable)); //csrw write satp.
  sfence_vma(); // flush all TLB entries.
}

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
//
//       64      39 38  30 29   21 20  12 11       0  
// VA:  |     0    |  L2  |   L1  |  L0  |  offset  | 
//       63        54 53   10 9   8 7 6 5 4 3 2 1 0 
// PTE: |  Reserved  |  PPN  | RSW |D|A|G|U|X|W|R|V|
//
// *Note.The meaning of comments above is that the addres walk return
// is physical address of PTE of L0, not a page or a page directory.
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA) // MAXVA is defined in kernel/riscv.h
    panic("walk");

  // Only walk L2 and L1, not L0, which means that
  // walk() will only create page directory if alloc == 1,
  // and never check the flags of pte of L0.
  // walk() will not revoke the page that allocated if current page allocation faild.
  for(int level = 2; level > 0; level--) {
    // PX means page table index extract.
	// PX(leval, va) is a Macro defined in kernel/riscv.h,
	// is used to extract page table indices from a virtual address. 
    pte_t *pte = &pagetable[PX(level, va)]; // pte_t is uint64.
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte); // pagetable_addr = | PPN | 0 |
    } else { // if PTE_V is set to zero. 
      if(!alloc || (pagetable = (pde_t*)kalloc()) == 0) // pde_t is uint64, defined in kernel/types.h
        return 0;
      memset(pagetable, 0, PGSIZE);
	  // The PTE point to the new page and set the PTE_V. 
	  // Note. the physical address that return from kalloc()
	  // is align to PGSIZE 
      *pte = PA2PTE(pagetable) | PTE_V;  
    }
  }
  return &pagetable[PX(0, va)]; // here pagetable is point to L0 directory.
}

// Look up a virtual address, return the physical (page) address,
// or 0 if not mapped.
// *Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return 0;

  pte = walk(pagetable, va, 0);
  if(pte == 0) // no L0 page table, it cannot merge with condition below, because of the null pointer.
    return 0;
  if((*pte & PTE_V) == 0) // page is not present. 
    return 0;
  if((*pte & PTE_U) == 0) // the page is inaccessible to user mode.
    return 0;
  pa = PTE2PA(*pte);
  return pa; // pa is the physical page address, not the physical address to which va refers.
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void
kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kpgtbl, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
// Note. mapages will allocate more actrual size than 
// specified by the argument `size`.
//
// e.g.
// ideal: SIZE = va + size - 1 - va + 1 = size
//    |   |     |   |
// va |   |     |   |  
//    |   | ... |   | va + size - 1
//    |   |     |   |
//    |   |     |   |
// actual: SIZE = page_i_end - a + 1
// a  |   |     |   | last
// va |   |     |   |  
//    |   | ... |   | va + size - 1
//    |   |     |   |
//    |   |     |   | page_i_end 
   
int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last;
  pte_t *pte;

  if(size == 0)
    panic("mappages: size");
  
  a = PGROUNDDOWN(va); // PGROUNDDOWN is defined in riscv.h
  last = PGROUNDDOWN(va + size - 1);
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0) // page table allocation failed 
      return -1;
    if(*pte & PTE_V)
      panic("mappages: remap");
    *pte = PA2PTE(pa) | perm | PTE_V; // PA2PTE() is a macro to extract PPNfrom physical address; perm is flag.
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}


/* the interface of uvm* used by user has no concept about pysical address */ 

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory with `do_free`.
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      panic("uvmunmap: walk");
    if((*pte & PTE_V) == 0)
      panic("uvmunmap: not mapped");
    if(PTE_FLAGS(*pte) == PTE_V) // according to the riscv isa
      panic("uvmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
  }
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t
uvmcreate()
{
  pagetable_t pagetable;
  pagetable = (pagetable_t) kalloc();
  if(pagetable == 0)
    return 0;
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// Load the user initcode into address 0 of pagetable,
// for the very first process.
// sz must be less than a page.
void
uvminit(pagetable_t pagetable, uchar *src, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
  memmove(mem, src, sz);
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
//
// e.g.
// before: 
//     assigend   unssigned
//       |   |     |   |
// oldsz |   |     |   |
//       |   | ... |   | newsz
//       |   |     |   |
//       |   |     |   |
// later: 
//       -------assigned-----------
//       |   |  |   | oldsz  |   |
//       |   |  |   |        |   |
//       |   |  |   |   ...  |   | newsz
//       |   |  |   |        |   |
//       |   |  |   |        |   |

uint64
uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  char *mem;
  uint64 a;

  if(newsz < oldsz)
    return oldsz;

  oldsz = PGROUNDUP(oldsz); // move to the begin of the next page
  for(a = oldsz; a < newsz; a += PGSIZE){
    mem = kalloc();
	// if memory not enough, undo assigned.
    if(mem == 0){
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
	// if fail to map, undo.
    if(mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_W|PTE_X|PTE_R|PTE_U) != 0){
      kfree(mem);
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
// i.e.
// before:
//      |  |     |  |  
// newsz|  | ... |  |
//      |  |     |  | oldsz
//      |  |     |  |
//
// later:
//      |  | 
// newsz|  | <--- remain this page, and return newsz
//      |  |
//      |  |
uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  if(newsz >= oldsz)
    return oldsz;

  // if newsz and oldsz is not within a same page
  if(PGROUNDUP(newsz) < PGROUNDUP(oldsz)){
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    // uvmunmap does not revoke the page table assigend, 
	// only the page referenced by leaf pte.
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);   
  }

  return newsz;
}

// Recursively free only page-table pages.It required that 
// all leaf mappings must already have been removed.
// Note. DFS
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
	  // leaf mapping has not been removed
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}

#ifdef LAB_PGTBL
#define BULLET " .."

static void 
pgtblsearch(pagetable_t pagetable, int n) 
{
  for (int i = 0; i < 512; i++) {
    pte_t pte = pagetable[i];
	if (pte & PTE_V) {
	  pagetable_t pa = (pagetable_t)PTE2PA(pte);

	  for (int j = 0; j <= n; j++) 
	    printf(BULLET); 
	  printf("%d: pte %p pa %p\n",i, pte, pa);

      // not leaf
	  if ((pte & (PTE_R | PTE_W | PTE_X)) == 0)
	    pgtblsearch(pa, n + 1); 
	}
  }
}

void
vmprint(pagetable_t pagetable)
{
  printf("page table %p\n", pagetable);
  pgtblsearch(pagetable, 0);
}


int
pgaccess(pagetable_t pagetable, uint64 va, int pgn, uint64 maskptr)
{
  bitmask_t mask = 0;
  pte_t *pte;

  if (va >= MAXVA)
    return -1;
 
  va = PGROUNDDOWN(va);
  for (int i = 0; i < pgn; i++, va += PGSIZE) {
    pte = walk(pagetable, va, 0);
    if (pte == 0)
      return -1;
    if (((*pte & PTE_V) == 0) || ((*pte & PTE_U) == 0))
	  return -1;

	if (*pte & PTE_A) {
	  mask = mask | (1 << i);
	  *pte &= ~PTE_A;
	}
  }

  if (copyout(pagetable, maskptr, (char *)&mask, sizeof(mask)) < 0)
    return -1; // maskptr is a invalid user address.

  // if maskptr within [PGROUNDDOWN(va), PGROUNDDOWN(va) + pgn), the PTE_A bit of 
  // the leaf PTE pointing to the page relevant to maskptr will be set again when
  // pgaccess return.

  return 0;
}
#endif

// Free user memory pages,
// then free page-table pages.
void
uvmfree(pagetable_t pagetable, uint64 sz)
{
  if(sz > 0)
    // unmap [start, start + npage)
    uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
  freewalk(pagetable);
}

// Given a parent process's page table, copy
// its memory into a child's page table.
// Copies both the page table and the
// physical memory.
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
      kfree(mem);
      goto err;
    }
  }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void
uvmclear(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  
  pte = walk(pagetable, va, 0);
  if(pte == 0)
    panic("uvmclear");
  *pte &= ~PTE_U;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva); // va0 = | VPN | 0 |
    pa0 = walkaddr(pagetable, va0); // pa0 = | PPN | 0 |  
    if(pa0 == 0) // not mapped or inaccessible to user
      return -1;  
	// Move bytes no more than PGSIZE at a time, 
	// because it must check each page.
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;

	// Take an advantadge of the mapping `pa(va) = va`, 
	// so it can work with MMU.
	// Load and Store from or to a virtual address is not 
	// dependent on `walk`(`walk` is only be used to create 
	// a new page table and help check), it will be done correctly 
	// by underlying hardware.
    memmove((void *)(pa0 + (dstva - va0)), src, n); 
    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  uint64 n, va0, pa0;
  int got_null = 0;

  while(got_null == 0 && max > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > max)
      n = max;

    char *p = (char *) (pa0 + (srcva - va0));
    while(n > 0){
      if(*p == '\0'){
        *dst = '\0';
        got_null = 1;
        break;
      } else {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;
  }
  if(got_null){
    return 0;
  } else {
    return -1;
  }
}
