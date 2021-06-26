#!Makefile

#
# Makefile for centralized control of building the Hux system into a
# CDROM image and launching QEMU.
#


TARGET_BIN=hux.bin
TARGET_ISO=hux.iso
TARGET_SYM=hux.sym


C_SOURCES=$(shell find ./src/ -name "*.c")
C_OBJECTS=$(patsubst %.c, %.o, $(C_SOURCES))

INITPROC_SOURCE="./src/process/init.s"
INITPROC_OBJECT="./src/process/init.o"
INITPROC_LINKED="./src/process/init.out"
INITPROC_BINARY="./src/process/init"

S_SOURCES=$(filter-out $(INITPROC_SOURCE), $(shell find ./src/ -name "*.s"))
S_OBJECTS=$(patsubst %.s, %.o, $(S_SOURCES))


ASM=i686-elf-as
ASM_FLAGS=

CC=i686-elf-gcc
C_FLAGS=-c -Wall -Wextra -ffreestanding -O2 -std=gnu99 -Wno-tautological-compare \
        -g -fno-omit-frame-pointer -fstack-protector

LD=i686-elf-gcc
LD_FLAGS=-ffreestanding -O2 -nostdlib

OBJCOPY=i686-elf-objcopy
OBJDUMP=i686-elf-objdump


HUX_MSG="[--Hux->]"


#
# Targets for building.
#
all: $(S_OBJECTS) $(C_OBJECTS) initproc link verify update symfile

.s.o:
	@echo $(HUX_MSG) "Compiling assembly '$<'..."
	$(ASM) $(ASM_FLAGS) $< -o $@

.c.o:
	@echo $(HUX_MSG) "Compiling C code '$<'..."
	$(CC) $(C_FLAGS) $< -o $@

# Init process goes separately, links into an independent binary.
initproc:
	@echo $(HUX_MSG) "Handling 'init' process binary..."
	$(ASM) $(ASM_FLAGS) -o $(INITPROC_OBJECT) $(INITPROC_SOURCE)
	$(LD) $(LD_FLAGS) -N -e start -Ttext 0 -o $(INITPROC_LINKED) $(INITPROC_OBJECT)
	$(OBJCOPY) -S -O binary $(INITPROC_LINKED) $(INITPROC_BINARY)

# Remember to link 'libgcc'. Embeds the init process binary.
link:
	@echo $(HUX_MSG) "Linking..."
	$(LD) $(LD_FLAGS) -T scripts/kernel.ld -lgcc -o $(TARGET_BIN) -Wl,--oformat,elf32-i386 \
		$(S_OBJECTS) $(C_OBJECTS) -Wl,-b,binary,$(INITPROC_BINARY)


#
# Verify GRUB multiboot sanity.
#
.PHONY: verify
verify:
	@if grub-file --is-x86-multiboot $(TARGET_BIN); then	\
		echo $(HUX_MSG) "VERIFY MULTIBOOT: Confirmed ✓"; 	\
	else													\
		echo $(HUX_MSG) "VERIFY MULTIBOOT: FAILED ✗";		\
	fi


#
# Update CDROM image.
#
.PHONY: update
update:
	@echo $(HUX_MSG) "Writing to CDROM..."
	mkdir -p isodir/boot/grub
	cp $(TARGET_BIN) isodir/boot/$(TARGET_BIN)
	cp scripts/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(TARGET_ISO) isodir


#
# Stripping symbols out of the ELF.
#
.PHONY: symfile
symfile:
	@echo $(HUX_MSG) "Stripping symbols into '$(TARGET_SYM)'..."
	$(OBJCOPY) --only-keep-debug $(TARGET_BIN) $(TARGET_SYM)
	$(OBJCOPY) --strip-debug $(TARGET_BIN)


#
# Launching QEMU/debugging.
#
.PHONY: qemu
qemu:
	@echo $(HUX_MSG) "Launching QEMU..."
	qemu-system-i386 -cdrom $(TARGET_ISO)

.PHONY: qemu_debug
qemu_debug:
	@echo $(HUX_MSG) "Launching QEMU (debug mode)..."
	qemu-system-i386 -S -s -cdrom $(TARGET_ISO)

.PHONY: gdb
gdb:
	@echo $(HUX_MSG) "Launching GDB..."
	gdb -x scripts/gdb_init


#
# Clean the produced files.
#
.PHONY: clean
clean:
	rm -f $(S_OBJECTS) $(C_OBJECTS) $(INITPROC_OBJECT) $(INITPROC_LINKED) \
	      $(INITPROC_BINARY) $(TARGET_BIN) $(TARGET_ISO) $(TARGET_SYM)
