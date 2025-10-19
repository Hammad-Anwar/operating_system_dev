#define MAXPROCS 8

typedef enum { NONE, READY, RUNNING, BLOCKED } procstate_t;

typedef struct {
  uint64 pc;
  uint64 sp;
  uint64 physbase;
  uint64 pagetablebase;
  procstate_t state;
} pcbentry;

