# Build the kernel and user process binaries

CC=riscv64-unknown-elf-gcc
CFLAGS=-g -mcmodel=medany -mno-relax -I. -ffreestanding
OBJCOPY=riscv64-unknown-elf-objcopy

KERNELDEPS = hardware.h riscv.h types.h 
KERNELOBJS = boot.o kernel.o ex.o setup.o
USERDEPS = riscv.h types.h
USER1OBJS = user1.o userentry.o
USER2OBJS = user2.o userentry.o
USER3OBJS = user3.o userentry.o

%.o: %.c $(KERNELDEPS) $(USERDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.S $(KERNELDEPS) $(USERDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all:    user1.bin user2.bin user3.bin kernel

kernel: $(KERNELOBJS) $(KERNELDEPS)
	$(CC) -g -ffreestanding -fno-common -nostdlib -mno-relax \
	      -mcmodel=medany -Wl,-T kernel.ld $(KERNELOBJS) -o kernel
	$(OBJCOPY) -O binary kernel kernel.bin

user1.bin: $(USER1OBJS) $(USERDEPS)
	$(CC) $(CFLAGS) -c user1.c
	$(CC) $(CFLAGS) -c userentry.S
	$(CC) -g -ffreestanding -fno-common -nostdlib -mno-relax \
	      -mcmodel=medany   -Wl,-T user.ld userentry.o user1.o -o user1
	$(OBJCOPY) -O binary user1 user1.bin

user2.bin: $(USER2OBJS) $(USERDEPS)
	$(CC) $(CFLAGS) -c user2.c
	$(CC) $(CFLAGS) -c userentry.S
	$(CC) -g -ffreestanding -fno-common -nostdlib -mno-relax \
	      -mcmodel=medany   -Wl,-T user.ld userentry.o user2.o -o user2
	$(OBJCOPY) -O binary user2 user2.bin

user3.bin: $(USER3OBJS) $(USERDEPS)
	$(CC) $(CFLAGS) -c user3.c
	$(CC) $(CFLAGS) -c userentry.S
	$(CC) -g -ffreestanding -fno-common -nostdlib -mno-relax \
	      -mcmodel=medany   -Wl,-T user.ld userentry.o user3.o -o user3
	$(OBJCOPY) -O binary user3 user3.bin

run:	user1.bin user2.bin user3.bin kernel
	qemu-system-riscv64 -nographic -machine virt -smp 1 -bios none -kernel kernel -device loader,addr=0x80200000,file=user1.bin -device loader,addr=0x80400000,file=user2.bin -device loader,addr=0x80600000,file=user3.bin
	
clean:
	-@rm -f *.o *.bin kernel user1 user2 user3 userprogs1.h userprogs2.h

