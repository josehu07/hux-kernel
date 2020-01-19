#!Makefile

C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))

S_SOURCES = $(shell find . -name "*.s")
S_OBJECTS = $(patsubst %.s, %.o, $(S_SOURCES))

CC = gcc
C_FLAGS = -c -Wall -m32 -ggdb -gstabs+ -nostdinc -fno-pic -fno-builtin -fno-stack-protector -I include

LD = ld
LD_FLAGS = -T scripts/kernel.ld -m elf_i386 -nostdlib

ASM = nasm
ASM_FLAGS = -f elf -g -F stabs


#
# Targets for building.
#

all: $(S_OBJECTS) $(C_OBJECTS) link update

.c.o:
	@echo Compiling C code $< ...
	$(CC) $(C_FLAGS) $< -o $@

.s.o:
	@echo Compiling asm code $< ...
	$(ASM) $(ASM_FLAGS) $<

link:
	@echo Linking ...
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o hux_kernel

.PHONY: clean
clean:
	rm -f $(S_OBJECTS) $(C_OBJECTS) hux_kernel


#
# Floppy image related.
#

.PHONY: update
update:
	sudo mount floppy_hux.img /mnt/floppy_hux
	sudo cp hux_kernel /mnt/floppy_hux/hux_kernel
	sleep 1
	sudo umount /mnt/floppy_hux

.PHONY: mount
mount:
	sudo mount floppy_hux.img /mnt/floppy_hux

.PHONY: umount
umount:
	sudo umount /mnt/floppy_hux


#
# Launching QEMU.
#

.PHONY: qemu
qemu:
	qemu -fda floppy_hux.img -boot a

.PHONY: debug
debug:
	qemu -S -s -fda floppy_hux.img -boot a &
	sleep 1
	cgdb -x tools/gdbinit
