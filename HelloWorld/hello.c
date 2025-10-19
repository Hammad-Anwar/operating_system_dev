typedef char uint8_t;

__attribute__ ((aligned (16))) char stack0[4096];

struct uart {
   union {
     uint8_t THR; // W = transmit hold register (offset 0)
     uint8_t RBR; // R = receive buffer register (also offset 0)
     uint8_t DLL; // R/W = divisor latch low (offset 0 when DLAB=1)
   };
   union {
     uint8_t IER; // R/W = interrupt enable register (offset 1)
     uint8_t DLH; // R/W = divisor latch high (offset 1 when DLAB=1)
   };
   union {
     uint8_t IIR; // R = interrupt identif. reg. (offset 2)
     uint8_t FCR; // W = FIFO control reg. (also offset 2)
   };
   uint8_t LCR; // R/W = line control register (offset 3)
   uint8_t MCR; // R/W = modem control register (offset 4)
   uint8_t LSR; // R   = line status register (offset 5)
};

volatile struct uart* uart0 = (volatile struct uart *)0x10000000;

void putachar(char c) {
    while ((uart0->LSR & 0x20) == 0);
    uart0->THR = c;
}

char getchar() {
    while ((uart0->LSR & 0x01) == 0);
    return uart0->RBR;
}

void readstring(char *buf, int max_len) {
    int i = 0;
    char c;
    while (i < max_len - 1) {
        c = getchar();
        putachar(c);
        if (c == '\n' || c == '\r') break;
        buf[i++] = c;
    }
    buf[i] = 0;
}


void printstring(char *s) {
    while (*s) {
	putachar(*s++);
    }
}

void touppercase(char *s) {
    while (*s) {
        if (*s >= 'a' && *s <= 'z') {
            *s -= 32;
        }
        s++;
    }
}


int main(void) {
    printstring("Hallo RISC-V!\n");
    char buffer[64];
    printstring("Enter text:\n");
    readstring(buffer, sizeof(buffer));
    touppercase(buffer);
    printstring("Uppercase version:\n");
    printstring(buffer);
    printstring("\n");
    return 0;
}

