#include "types.h"
#include "riscv.h"
#include "kernel.h"

extern int main(void);
extern void ex(void);
extern void printastring(char *);
extern void printhex(uint64);

extern pcbentry pcb[MAXPROCS];
extern uint64 current_pid;

#define NPROC 8 
#define PGSHIFT 12
#define PERMSHIFT 10
#define PERM_V 0
#define PERM_X 3
#define PERM_W 2
#define PERM_R 1
#define PERM_U 4

__attribute__ ((aligned (4096))) uint64 pt[NPROC][512*3];

uint64 init_pt(int proc) { 
  for (int i=0; i<512; i++) {
    if (i == 0) {
      pt[proc][i] = ((uint64)&pt[proc][512]) >> PGSHIFT << PERMSHIFT;
      pt[proc][i] |= (1 << PERM_V);
#if 0
printastring("PT0 = ");
printhex(pt[proc][i]);
printastring("\n");
#endif
    } else {
      pt[proc][i] = 0;
    }
  }
  for (int i=512; i<1024; i++) {
    if (i == 512) {
      pt[proc][i] = (0x80000000ULL + (proc+1) * 0x200000ULL) >> PGSHIFT << PERMSHIFT;
      pt[proc][i] |= 0x7f; // (1 << PERM_V) | (1 << PERM_R) | (1 << PERM_W) | (1 << PERM_X) | (1<<PERM_U) | (1 <<PERM_A) | (1<<PERM_D);
#if 0
printastring("PT1 = ");
printhex(pt[proc][i]);
printastring("\n");
#endif
    } else {
      pt[proc][i] = 0;
    }
  }
  return (uint64)&pt[proc][0];
}

void setup(void) {
  // set M Previous Privilege mode to User so mret returns to user mode.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_U;
  w_mstatus(x);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // enable software interrupts (ecall) in M mode.
  w_mie(r_mie() | MIE_MSIE);

  // set the machine-mode trap handler to jump to function "ex" when a trap occurs.
  w_mtvec((uint64)ex);

  // enable paging now!
  for (int i = 0; i < NPROC; i++) {
    pcb[i].pc = -4;
    pcb[i].sp = 0x80200000ULL + 0x200000 * i + 0x1ffff8 - 256; // 0xdeadc0de;
    pcb[i].physbase = 0x80200000ULL + 0x200000 * i;
    pcb[i].pagetablebase = init_pt(i);
    pcb[i].state = NONE;
  } 

  #define SATP_SV39 (8L << 60)
  #define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))
  w_satp(MAKE_SATP((uint64)&pt[0][0]));
  asm volatile("sfence.vma zero, zero");

  // configure Physical Memory Protection to give user mode access to all of physical memory.
  w_pmpaddr0(0x3fffffffffffffULL);
  w_pmpcfg0(0xf);

  // set M Exception Program Counter to main, for mret, requires gcc -mcmodel=medany
  w_mepc((uint64)0);

  current_pid = 0;
  pcb[0].state = RUNNING;
  pcb[1].state = READY;
  w_mscratch(pcb[0].physbase);

  // switch to user mode (configured in mstatus) and jump to address in mepc CSR -> main().
  asm volatile("mret");
}

