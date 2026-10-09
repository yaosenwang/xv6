// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
#ifdef LAB_LOCK
  int nts;
  int n;
#endif
};

#ifdef LAB_LOCK
// Reader-writer lock.
struct rwspinlock {
  // Replace this with your implementation.
  struct spinlock l;
  uint64 locked;
};

#define READ_ACTIVE_BITS    32 
#define WRITE_PENDING_BITS  31 
#define WRITE_ACTIVE_BITS   1
#define READ_ACTIVE_SHIFT   0
#define WRITE_PENDING_SHIFT READ_ACTIVE_BITS
#define WRITE_ACTIVE_SHIFT  (READ_ACTIVE_BITS + WRITE_PENDING_BITS)
#define READ_ACTIVE_MASK  (((1UL<<READ_ACTIVE_BITS)-1) << READ_ACTIVE_SHIFT)
#define READ_ACTIVE(val)  (((val)&READ_ACTIVE_MASK) >> READ_ACTIVE_SHIFT)
#define WRITE_PENDING_MASK  (((1UL<<WRITE_PENDING_BITS)-1) << WRITE_PENDING_SHIFT)
#define WRITE_PENDING(val)  (((val)&WRITE_PENDING_MASK) >> WRITE_PENDING_SHIFT)
#define WRITE_ACTIVE_MASK (((1UL<<WRITE_ACTIVE_BITS)-1) << WRITE_ACTIVE_SHIFT)
#define WRITE_ACTIVE(val) (((val)&WRITE_ACTIVE_MASK) >> WRITE_ACTIVE_SHIFT)
#endif
