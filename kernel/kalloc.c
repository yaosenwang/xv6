// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#if 0
void freerange(void *pa_start, void *pa_end, int cpu_id);
#endif
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

#if 0
struct page {
  int cpuid;
};

struct {
  struct spinlock locks[NCPU];
  struct run *freelists[NCPU];
  struct page pages[(PHYSTOP-KERNBASE)/PGSIZE];
} kmem;
#endif
struct {
  struct spinlock locks[NCPU];
  struct run *freelists[NCPU];
} kmem;

void
kinit()
{
  #if 0
  int i;
  char *phy_mem_start = end;
  uint64 per_cpu_phy_mem_size = (PHYSTOP-(uint64)end) / NCPU;
  printf("per_cpu_phy_mem_size: %ld\n", per_cpu_phy_mem_size);

  for (i = 0; i < NCPU; i++) {
    initlock(&kmem.locks[i], "kmem");
    kmem.freelists[i] = 0;
    freerange(phy_mem_start, phy_mem_start + per_cpu_phy_mem_size, i);
    phy_mem_start += per_cpu_phy_mem_size;
  }
  #endif

  int i;
  for (i = 0; i < NCPU; i++) {
    initlock(&kmem.locks[i], "kmem");
    kmem.freelists[i] = 0;
  }
  freerange(end, (void*)PHYSTOP);
}

#if 0
void
freerange(void *pa_start, void *pa_end, int cpu_id)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    kmem.pages[((uint64)p-KERNBASE)/PGSIZE].cpuid = cpu_id;
    kfree(p);
  }
}
#endif
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int cpu_id;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

#if 0
  cpu_id = kmem.pages[((uint64)pa-KERNBASE)/PGSIZE].cpuid;
#endif
  r = (struct run*)pa;

  push_off();
  cpu_id = cpuid();
  acquire(&kmem.locks[cpu_id]);
  r->next = kmem.freelists[cpu_id];
  kmem.freelists[cpu_id] = r;
  release(&kmem.locks[cpu_id]);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int cpu_id, i;

  push_off();
  cpu_id = cpuid();
  for (i = cpu_id; i < cpu_id+NCPU; i++)
  {
    acquire(&kmem.locks[i%NCPU]);
    r = kmem.freelists[i%NCPU];
    if(r)
      kmem.freelists[i%NCPU] = r->next;
    release(&kmem.locks[i%NCPU]);
    if (r)
      break;
  }
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
