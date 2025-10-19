#define MAXPROCS 8

typedef enum { NONE, READY, RUNNING, BLOCKED, SLEEPING } procstate_t;

typedef struct {
  procstate_t state;
  uint64 pc;
  uint64 sp;
  uint64 physbase;
  uint64 pagetablebase;
  uint64 wakeuptime;
} pcbentry;

