# VM and Multitasking

This code is supposed to implement cooperative multitasking on top
of our virtual memory implementation. In the current state, it suffers
from inconsistencies regarding the use of virtual or physical addresses
in the kernel - especially the question which address (virt or phys)
is stored in the PCB and when the respective representations need to
be converted.

