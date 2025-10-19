#!/bin/sh
qemu-system-riscv64 -M virt -bios none -kernel kernel -device loader,file=user1.bin,addr=0x80200000 -device loader,addr=0x80400000,file=user2.bin  -device loader,addr=0x80600000,file=user3.bin -nographic
