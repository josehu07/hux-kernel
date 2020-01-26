#!Makefile

# Makefile for centralized contorlling of building the Hux system into a
# CDROM image and launching QEMU.


TARGET_BIN=hux.bin
TARGET_ISO=hux.iso

C_SOURCES=$(shell find . -name "*.c")
C_OBJECTS=$(patsubst %.c, %.o, $(C_SOURCES))

S_SOURCES=$(shell find . -name "*.s")
S_OBJECTS=$(patsubst %.s, %.o, $(S_SOURCES))

ASM=/usr/local/cross/bin/i686-elf-as
ASM_FLAGS=

CC=/usr/local/cross/bin/i686-elf-gcc
C_FLAGS=-c -Wall -Wextra -ffreestanding -O2 -std=gnu99 -Wno-tautological-compare

LD=/usr/local/cross/bin/i686-elf-gcc -T scripts/kernel.ld
LD_FLAGS=-ffreestanding -O2 -nostdlib

HUX_MSG="[--Hux->]"


#
# Targets for building.
#
all: $(S_OBJECTS) $(C_OBJECTS) link verify update

.s.o:
	@echo $(HUX_MSG) "Compiling assembly '$<'..."
	$(ASM) $(ASM_FLAGS) $< -o $@

.c.o:
	@echo $(HUX_MSG) "Compiling C code '$<'..."
	$(CC) $(C_FLAGS) $< -o $@

link:
	@echo $(HUX_MSG) "Linking..."	# Remember to link 'libgcc'.
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -lgcc -o $(TARGET_BIN)

# Verify GRUB multiboot sanity.
.PHONY: verify
verify:
	@if grub-file --is-x86-multiboot $(TARGET_BIN); then	\
		echo $(HUX_MSG) "VERIFY MULTIBOOT: Confirmed ✓"; 	\
	else													\
		echo $(HUX_MSG) "VERIFY MULTIBOOT: FAILED ✗";		\
	fi

# Clean the produced files.
.PHONY: clean
clean:
	rm -f $(S_OBJECTS) $(C_OBJECTS) $(TARGET_BIN) $(TARGET_ISO)


#
# CDROM image related.
#
.PHONY: update
update:
	@echo $(HUX_MSG) "Writing to CDROM..."
	mkdir -p isodir/boot/grub
	cp $(TARGET_BIN) isodir/boot/$(TARGET_BIN)
	cp scripts/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(TARGET_ISO) isodir


#
# Launching QEMU.
#
.PHONY: qemu
qemu:
	@echo $(HUX_MSG) "Launching QEMU..."
	qemu-system-i386 -cdrom $(TARGET_ISO)

.PHONY: debug
debug:
	qemu-system-i386 -S -s -cdrom $(TARGET_ISO) &
	sleep 1
	cgdb -x tools/gdbinit
