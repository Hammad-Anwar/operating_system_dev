#include "types.h"
#include "syscalls.h"

__attribute__ ((aligned (16))) char userstack[4096];

uint64 syscall(uint64 nr, uint64 param) {
    uint64 retval;

    // TODO: add inline assembler code to copy the value in variable nr to register a7
    asm volatile("mv a7, %0" : : "r" (nr) : );
    asm volatile("mv a0, %0" : : "r" (param) : );

    // here's our ecall!
    asm volatile("ecall");

    // TODO:add inline assembler code to copy the return value in register a0 to variable retval

    // Here we return the return value...
    asm volatile("mv %0, a0" : "=r" (retval) : : );
    return retval;
}

// enum { PRINTASTRING = 1, PRINTACHAR, GETACHAR };

void printastring(char *s) {
    syscall(PRINTASTRING, (uint64)s);
}

void putachar(char c) {
    syscall(PUTACHAR, (uint64)c);
}

char getachar(void) {
    return syscall(GETACHAR, 0);
}

char yield(void) {
    syscall(YIELD, 0);
}

// ----

int main(void) {
    char c = 0;
    printastring("This is process 2!\n");
    while(1) {
      for (c='A'; c <= 'Z'; c++) {
        putachar(c);
        yield();
      }
    }
    printastring("This is the end!\n");
    return 0;
}

