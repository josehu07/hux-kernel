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

S_SOURCES=$(shell find ./src/ -name "*.s")
S_OBJECTS=$(patsubst %.s, %.o, $(S_SOURCES))

INIT_SOURCE=./user/init.c
INIT_OBJECT=./user/init.o
INIT_LINKED=./user/init.out
INIT_BINARY=./user/init

ULIB_C_SOURCES=$(shell find ./user/lib/ -name "*.c")
ULIB_C_OBJECTS=$(patsubst %.c, %.o, $(ULIB_C_SOURCES))

ULIB_S_SOURCES=$(shell find ./user/lib/ -name "*.s")
ULIB_S_OBJECTS=$(patsubst %.s, %.o, $(ULIB_S_SOURCES))

USER_SOURCES=$(filter-out $(INIT_SOURCE), $(shell find ./user/ -maxdepth 1 -name "*.c"))
USER_BINARYS=$(patsubst %.c, %.bin, $(USER_SOURCES))


ADDRSPACE_USER_BASE=0x20000000


ASM=i686-elf-as
ASM_FLAGS=

CC=i686-elf-gcc
C_FLAGS_USER=-c -Wall -Wextra -ffreestanding -O2 -std=gnu99 -Wno-tautological-compare \
			 -g -fno-omit-frame-pointer
C_FLAGS=$(C_FLAGS_USER) -fstack-protector

LD=i686-elf-gcc
LD_FLAGS=-ffreestanding -O2 -nostdlib

OBJCOPY=i686-elf-objcopy
OBJDUMP=i686-elf-objdump


HUX_MSG="[--Hux->]"


#
# Targets for building.
#
ALL_DEPS := $(S_OBJECTS) $(C_OBJECTS)
ALL_DEPS += $(ULIB_S_OBJECTS) $(ULIB_C_OBJECTS) $(USER_BINARYS) initproc
ALL_DEPS += link verify update symfile
all: $(ALL_DEPS)

$(S_OBJECTS): %.o: %.s
	@echo $(HUX_MSG) "Compiling kernel assembly '$<'..."
	$(ASM) $(ASM_FLAGS) -o $@ $<

$(C_OBJECTS): %.o: %.c
	@echo $(HUX_MSG) "Compiling kernel C code '$<'..."
	$(CC) $(C_FLAGS) -o $@ $<

# User programs use more specific rules to build into independent binary.
$(ULIB_S_OBJECTS): %.o: %.s
	@echo $(HUX_MSG) "Compiling user lib assembly '$<'..."
	$(ASM) $(ASM_FLAGS) -I ./user/lib/ -o $@ $<

$(ULIB_C_OBJECTS): %.o: %.c
	@echo $(HUX_MSG) "Compiling user lib C code '$<'..."
	$(CC) $(C_FLAGS_USER) -o $@ $<

$(USER_BINARYS): %.bin: %.c $(ULIB_S_OBJECTS) $(ULIB_C_OBJECTS)
	@echo $(HUX_MSG) "Compiling & linking user program '$<'..."
	$(CC) $(C_FLAGS_USER) -o $<.o $<
	$(LD) $(LD_FLAGS) -N -e main -Ttext $(ADDRSPACE_USER_BASE) -o $@ \
		$<.o $(ULIB_S_OBJECTS) $(ULIB_C_OBJECTS)

# Init process goes separately, to allow later embedding into kernel image.
initproc:
	@echo $(HUX_MSG) "Compiling & linking user 'init' program..."
	$(CC) $(C_FLAGS_USER) -o $(INIT_OBJECT) $(INIT_SOURCE)
	$(LD) $(LD_FLAGS) -N -e main -Ttext $(ADDRSPACE_USER_BASE) -o $(INIT_LINKED) \
		$(INIT_OBJECT) $(ULIB_S_OBJECTS) $(ULIB_C_OBJECTS)
	$(OBJCOPY) -S -O binary $(INIT_LINKED) $(INIT_BINARY)

# Remember to link 'libgcc'. Embeds the init process binary.
link: $(S_OBJECTS) $(C_OBJECTS) initproc
	@echo $(HUX_MSG) "Linking kernel image..."
	$(LD) $(LD_FLAGS) -T scripts/kernel.ld -lgcc -o $(TARGET_BIN) -Wl,--oformat,elf32-i386 \
		$(S_OBJECTS) $(C_OBJECTS) -Wl,-b,binary,$(INIT_BINARY)


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
	rm -f $(S_OBJECTS) $(C_OBJECTS) $(ULIB_S_OBJECTS) $(ULIB_C_OBJECTS) \
		$(INIT_OBJECT) $(INIT_LINKED) $(INIT_BINARY) $(USER_BINARYS) 	\
		$(TARGET_BIN) $(TARGET_ISO) $(TARGET_SYM)
