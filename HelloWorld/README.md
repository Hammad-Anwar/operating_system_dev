# Hello World!


The code implements a simple "Hello, World" program using direct UART access.

Files in this directory:

* boot.S: Startup code, sets up the stack and jumps to C code (setup)
* hello.c: Our code that prints using direct hardware access to the UART

The example kernel and user mode program can be compiled and linked with the build.sh shell script. This uses the hello.ld linker script.

