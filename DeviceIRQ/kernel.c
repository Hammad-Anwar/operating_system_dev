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

#define BUFFER_SIZE 32
char ringbuffer[BUFFER_SIZE];
int  head, tail, full_flag = 0, nelem = 0;

void printhex(uint64);

volatile struct uart* uart0 = (volatile struct uart *)0x10000000;

// Syscall 1: printstring. Takes a char *, prints the string to the UART, returns nothing
// Syscall 2: putachar.    Takes a char, prints the character to the UART, returns nothing
// Syscall 3: getachar.    Takes no parameter, reads a character from the UART (keyboard), returns the char
// Syscall 4: sleep.       Takes a uint64, suspends the process for the given number of timer ticks
// Syscall 23: yield.       Takes no parameter, gives up the CPU
// Syscall 42: exit.        Takes no parameter, exits the process


uint64 ticks = 0;
pcbentry pcb[MAXPROCS];
uint64 current_pid;
uint64 waiting_pid;
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

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{    
  int irq = *(uint32*)PLIC_MCLAIM;
  return irq;         
}       

// tell the PLIC we've served this IRQ.
void    
plic_complete(int irq)
{       
  *(uint32*)PLIC_MCLAIM = irq;
}       

int buffer_is_full(void) {
  return (full_flag == 1);
}

int buffer_is_empty(void) {
  return (nelem == 0);
}

int rb_write(char c) {
  int retval;

  // wait(&mutex);
  if (buffer_is_full()) {
     retval = -1;
  } else {
    ringbuffer[head] = c;
    head = (head+1) % BUFFER_SIZE;
    nelem++;
    if (head == tail)
      full_flag = 1;
    retval = 0;
  }
  //signal(&mutex);
  
  return retval;
}

int rb_read(char *c) {
  int retval;

//  wait(&mutex);
  if (buffer_is_empty()) {
    retval = -1;
  } else {
    *c = ringbuffer[tail];
    tail = (tail+1) % BUFFER_SIZE;
    nelem--;
    full_flag = 0;
    retval = 0;
  }
//  signal(&mutex);
  return retval;
}

unsigned char readachar(void) {
    char c;
    int retval;
    retval = rb_read(&c);
    if (retval == 0) {
#ifdef DEBUG
      putachar('>'); putachar(c); putachar('\n');
#endif
      return c;
    } else {
      return 0;
    }
}

void schedule() {
  while (1) {
    current_pid = (current_pid + 1) % MAXPROCS;
#ifdef DEBUG
    printastring("> Trying "); printhex(current_pid); printastring(": "); printhex(pcb[current_pid].state); printastring("\n");
#endif
    if (pcb[current_pid].state == READY) {
      pcb[current_pid].state = RUNNING;
    break;
    }
  }
  w_mscratch(pcb[current_pid].physbase);

  // set new process to RUNNING
  pcb[current_pid].state = RUNNING;
#ifdef DEBUG
  printastring("> Switch to "); printhex(current_pid); printastring("\n");
#endif
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

// #define DEBUG
#ifdef DEBUG
      printastring("EXC pid = ");
      printhex(current_pid);
      printastring(", mcause = ");
      printhex(mcause);
      printastring(", mepc = ");
      printhex(pc);
      printastring(", mtval = ");
      printhex(mtval);
      printastring("\n");
#endif

  if (mcause & (1ULL<<63)) {
    // Interrupt - async
    was_syscall = 0;
    if ((mcause & ~(1ull<<63)) == MTI) { // timer interrupt / CLINT
      int interval = 2000; // cycles; about 1/10th second in qemu.
      *(uint64*)CLINT_MTIMECMP(0) = *(uint64*)CLINT_MTIME + interval;

      ticks++;

      if ((ticks % 10) == 0) {
        // anyone asleep?
        for (int i=0; i<MAXPROCS; i++) {
          if (pcb[i].state == SLEEPING) {
            if (ticks >= pcb[i].wakeuptime) {
              pcb[i].state = READY;
              pcb[i].wakeuptime = 0;
            }
          }
        }
        schedule();
      }
    } else if ((mcause & ~(1ull<<63)) == MEI) { // external interrupt / PLIC
      int irq = plic_claim();
      if (irq == UART0_IRQ) {
	char c = uart0->RBR;
        rb_write(c); 
        if (full_flag) putachar('*');
      }
      plic_complete(irq);
      pcb[waiting_pid].state = READY; // make blocked process runnable again...
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
      case SLEEP:
        if (param > 0) {
          pcb[current_pid].state = SLEEPING;
          pcb[current_pid].wakeuptime = param;
        }
        schedule();
        break;
      case PRINTASTRING:
        printastring((char *)virt2phys(param));
        break;
      case PUTACHAR:
        putachar((char)param);
        break;
      case GETACHAR:
        retval = readachar();
        if (retval == 0) {
          was_syscall = 0;
#ifdef DEBUG
          printastring("BLOCK "); printhex(current_pid); printastring("\n");
#endif
          pcb[current_pid].state = BLOCKED;
          waiting_pid = current_pid;
          schedule();
        } else {
          was_syscall = 1;
        }
        break;
      case EXIT:
        pcb[current_pid].state = NONE;
        schedule(); 
        break;
      case YIELD:
        pcb[current_pid].state = READY;
        schedule();
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

#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))
  w_satp(MAKE_SATP(pcb[current_pid].pagetablebase));
  __asm__ volatile("sfence.vma zero, zero");

  // printastring("\n->Process "); printhex(current_pid); printastring(" pagetable @ "); printhex((uint64)pcb[current_pid].pagetablebase); printastring("\n");
  w_mscratch(pcb[current_pid].physbase);

  // restore values for process we are going to switch to
  // adjust return value - we want to return to the instruction _after_ the ecall! (at address mepc+4)
  if (was_syscall) {
    w_mepc(pcb[current_pid].pc + 4);
    regs->a0 = retval; // return value of syscall
  } else {
    w_mepc(pcb[current_pid].pc);
  }

  regs = (riscv_regs*)virt2phys(pcb[current_pid].sp);

#ifdef DEBUG
  printastring("\nreturn sp = (V)"); printhex((uint64)pcb[current_pid].sp); printastring(" (P)"); printhex((uint64)regs);
  printastring("\nreturn pc "); printhex((uint64)r_mepc()); printastring("\n");
#endif

  // pass the return value back in a0
  regs->a0 = retval;
  regs->sp = (uint64)regs;

  // this function returns the new SP to ex.S
  return (uint64)regs;
}
