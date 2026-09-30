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

struct {
  struct spinlock lock;
  struct spinlock superpg_lock;
  struct run *freelist;
  struct run *superpg_freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem.superpg_lock, "superpg_kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p, *superpg_start, *superpg_stop;

  // prepare super pages
  superpg_start = (char*)SUPERPGROUNDUP((uint64)pa_start);
  superpg_stop = superpg_start + SUPERPGNUM*SUPERPGSIZE;
  if (superpg_stop > (char*)pa_end)
    panic("unable to alloc enough super pages");
  for (; superpg_start + SUPERPGSIZE <= superpg_stop; superpg_start += SUPERPGSIZE)
    superpg_kfree(superpg_start);

  // prepare normal pages
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)SUPERPGROUNDUP((uint64)pa_start); p += PGSIZE)
    kfree(p);
  p = (char*)PGROUNDUP((uint64)superpg_stop);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

void *superpg_kalloc(void)
{
  struct run *r;

  acquire(&kmem.superpg_lock);
  r = kmem.superpg_freelist;
  if (r)
    kmem.superpg_freelist = kmem.superpg_freelist->next;
  release(&kmem.superpg_lock);

  return (void*)r;
}

void superpg_kfree(void *pa)
{
  struct run *r;

  // check pa
  if (((uint64)pa % SUPERPGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("superpg_kfree");

  r = (struct run*)pa;

  acquire(&kmem.superpg_lock);
  r->next = kmem.superpg_freelist;
  kmem.superpg_freelist = r;
  release(&kmem.superpg_lock);
}

// Free the page of physical memory pointed at by pa,
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

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
