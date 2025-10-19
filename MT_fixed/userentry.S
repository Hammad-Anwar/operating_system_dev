        .section .text
        .global _entry
_entry:
// TODO 3.1: add stack pointer initialization here
        la      sp, userstack
        li      a0, 4096
        add     sp, sp, a0

        jal     main
loop:
        j	loop

#if 0
goaway:
        li      a7, 42
        li      a0, 0
        ecall
#endif
