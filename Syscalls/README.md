# System Calls

The code implements system call handling in our example OS.

Files in this directory:

* boot.S: Startup code, sets up the stack and jumps to C code (setup)
* ex.S: Assembler function called when Interrupts/Exceptions occur
* hardware.h: Definiton of hardware-related structures (UART registers)
* kernel.c: Interrupt handling and "device driver" for UART
* riscv.h: Constants and functions to access RISC-V CSRs
* setup.c: Initialize the machine (Interrupts, PMP) and jump to main in U mode
* syscalls.h: Definitions of constants for system calls
* types.h: Definitons of standard types
* user.c: Our user mode program, includes the stub functions for system calls (these would commonly belong in libc)

The example kernel and user mode program can be compiled and linked with the build.sh shell script. This uses the kernel.ld linker script. Currently, all code and data are linked into the same address range, there is no (address) distincting between user and kernel code and no memory protection at all.

