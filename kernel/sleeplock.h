// Long-term locks for processes
//
// sleeplock is based on spinlock, sleeplock and spinlock
// both has `locked` field, but the `locked` of former serves 
// as a condition variable. 
struct sleeplock {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
};
