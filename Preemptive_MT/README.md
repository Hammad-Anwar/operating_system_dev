# VM, Multitasking and Timer interrupt/CLINT setup

This is the state of the code with working VM and multitasking.

The newest addition is the initialization of the CLINT timer 
interrupt and a handler for the async IRQs (which at the 
moment just prints "INT" and returns to the interrupted 
process).

The next step will be to implement task switching when a 
timer IRQ arrives.

