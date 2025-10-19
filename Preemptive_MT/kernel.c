#include "types.h"
#include "riscv.h"
#include "hardware.h"
#include "kernel.h"
#include "syscalls.h"

extern int main(void);
extern void ex(void);
extern void printstring(char *s);
extern uint64 pt[8][512*3];

#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

__attribute__ ((aligned (16))) char stack0[4096];

void printhex(uint64);

volatile struct uart* uart0 = (volatile struct uart *)0x10000000;

// Syscall 1: printstring. Takes a char *, prints the string to the UART, returns nothing
// Syscall 2: putachar.    Takes a char, prints the character to the UART, returns nothing
// Syscall 3: getachar.    Takes no parameter, reads a character from the UART (keyboard), returns the char
// Syscall 23: yield.       Takes no parameter, gives up the CPU
// Syscall 42: exit.        Takes no parameter, exits the process


uint64 ticks = 0;
pcbentry pcb[MAXPROCS];
uint64 current_pid;
int was_syscall = 0;

uint64 virt2phys(uint64 addr) {
  return pcb[current_pid].physbase + addr;
}

uint64 phys2virt(uint64 addr) {
  return addr - pcb[current_pid].physbase;
}

static void putachar(char c) {
  while ((uart0->LSR & (1<<5)) == 0)
    ; // polling!
  uart0->THR = c;
}

void printastring(char *s) {
  while (*s) {
    putachar(*s);
    s++;
  }
}

static char getachar(void) {
  char c;
  while ((uart0->LSR & (1<<0)) == 0)
    ; // polling!
  c = uart0->RBR;
  return c;
}

// This is a useful helper function (after you implemented putachar and printstring)
void printhex(uint64 x) {
  int i;
  char s;

  printastring("0x");
  for (i=60; i>=0; i-=4) {
    int d =  ((x >> i) & 0x0f);
    if (d < 10)
      s = d + '0';
    else
      s = d - 10 + 'a';
    putachar(s);
  }
}

// This is the C code part of the exception handler
// "exception" is called from the assembler function "ex" in ex.S with registers saved on the stack
uint64 exception(riscv_regs *regs) {
  uint64 nr;
  uint64 param;
  uint64 retval = 0;

  was_syscall = 1;

  nr = regs->a7;
  param = regs->a0;

  uint64 pc = r_mepc();
  uint64 mcause = r_mcause();
  uint64 mtval = r_mtval();

  pcb[current_pid].pc = pc;
  pcb[current_pid].sp = phys2virt((uint64)regs);
  pcb[current_pid].state = READY; // or blocked in the future...

#ifdef DEBUG
  printastring("mcause = ");
  printhex(mcause);
  printastring("\n");
#endif

  if (mcause & (1ULL<<63)) {
    // Interrupt - async
    was_syscall = 0;
    if ((mcause & ~(1ull<<63)) == 7) { // timer interrupt / CLINT
      int interval = 20000; // cycles; about 1/10th second in qemu.
      *(uint64*)CLINT_MTIMECMP(0) = *(uint64*)CLINT_MTIME + interval;

      ticks++;
#if 0
      printastring("INT ");
      printhex(ticks);
      printastring("\n");
#endif

      if ((ticks % 10) == 0) {
#if 0
               // anyone asleep?
               for (int i=0; i<MAXPROCS; i++) {
                 if (pcb[i].state == SLEEPING) {
                   if (ticks >= pcb[i].wakeuptime) {
                     pcb[i].state = READY;
                     pcb[i].wakeuptime = 0;
                   }
                 }
               }
#endif

               // change state of interrupted process to READY
               pcb[current_pid].state = READY;

               while (1) {
                 current_pid = (current_pid + 1) % MAXPROCS;
                 if (pcb[current_pid].state == READY) {
                   pcb[current_pid].state = RUNNING;
                 break;
                 }
               }
               w_mscratch(pcb[current_pid].physbase);

               pc = pcb[current_pid].pc;

               // set new process to RUNNING
               pcb[current_pid].state = RUNNING;
      }

    }
  } else {
    // all exceptions end up here
    if ((mcause & ~(1ULL<<63)) == 8) { // it's an ECALL!

#ifdef DEBUG
      printastring("SYSCALL ");
      printhex(nr);
      printastring(" PARAM ");
      printhex(param);
      printastring("\n");
#endif

      was_syscall = 1;

      switch(nr) {
      case PRINTASTRING:
        printastring((char *)virt2phys(param));
        break;
      case PUTACHAR:
        putachar((char)param);
        break;
      case GETACHAR:
        retval = (uint64)getachar();
        break;
      case EXIT:
        pcb[current_pid].state = NONE;
        while (1) {
          current_pid = (current_pid + 1) % MAXPROCS;
          if (pcb[current_pid].state != READY) continue;
          break;
        }
        pcb[current_pid].state = RUNNING;
        break;
      case YIELD:
        pcb[current_pid].state = READY;
        while (1) {
          current_pid = (current_pid + 1) % MAXPROCS;
          if (pcb[current_pid].state == READY) {
            pcb[current_pid].state = RUNNING;
            break;
          }
        }
        w_mscratch(pcb[current_pid].physbase);

        // switch page table!
#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))
        w_satp(MAKE_SATP((uint64)&pt[current_pid][0]));
        __asm__ __volatile__("sfence.vma");

        pc = pcb[current_pid].pc;
        break;
      default:
        printastring("*** INVALID SYSCALL NUMBER!!! ***\n");
        break;
      }
    } else { 
      printastring("EXC pid = ");
      printhex(current_pid);
      printastring(", mcause = ");
      printhex(mcause);
      printastring(", mepc = ");
      printhex(pc);
      printastring(", mtval = ");
      printhex(mtval);
      printastring("\n");
    }
  }

  // Here, we adjust return value - we want to return to the instruction _after_ the ecall! (at address mepc+4)

  w_satp(MAKE_SATP(pcb[current_pid].pagetablebase));
  asm volatile("sfence.vma zero, zero");

  w_mscratch(pcb[current_pid].physbase);

  // restore values for process we are going to switch to
  // adjust return value - we want to return to the instruction _after_ the ecall! (at address mepc+4)
  if (was_syscall) {
    w_mepc(pcb[current_pid].pc + 4);
    regs->a0 = retval; // return value of syscall
  } else {
    w_mepc(pcb[current_pid].pc);
  }


  // TODO: pass the return value back in a0
  regs = (riscv_regs*)virt2phys(pcb[current_pid].sp);

#ifdef DEBUG
  printastring("\nreturn sp = (V)"); printhex((uint64)pcb[current_pid].sp); printastring(" (P)"); printhex((uint64)regs);
  printastring("\nreturn pc "); printhex((uint64)r_mepc()); printastring("\n");
#endif

  regs->a0 = retval;
  regs->sp = (uint64)regs;

  // this function returns the new SP to ex.S
  return (uint64)regs;
}
