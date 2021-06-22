/**
 * AT&T syntax (GAS) x86 IA32 booting code, following the OSDev wiki on
 * https://wiki.osdev.org/Bare_Bones.
 */


/**
 * Declare the multiboot header. It should contain "magic", "flags", and
 * "checksum" in order. GRUB will search for this header in the first 8KiB of
 * the kernel file, aligned to 32-bit boundary.
 */
.set MAGIC,     0x1BADB002

.set ALIGN,     1<<0    /** Align modules on page boundaries. */
.set MEMINFO,   1<<1    /** Provide memory map. */
.set FLAGS,     ALIGN | MEMINFO

.set CHECKSUM,  -(MAGIC + FLAGS)    /** Add up to 0. */

/** Header section here. */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM


/**
 * The kernel must provide a temporary stack for code execution at booting,
 * when the entire virtual memory system and user mode execution has not been
 * set up yet. We make a 16KiB stack space here.
 * 
 * It will grow downwards from current 'stack_hi'. The stack must be
 * aligned to 16 Bytes according to the System V ABI standard.
 */
.section .bss
.align 16
stack_lo:
.skip 16384     /** 16 KiB. */
stack_hi:


/**
 * Our linker script will use '_start' as the entry point to the kernel. No
 * need to return from the _start function because the bootloader has gone
 * at this time point.
 *
 * We will be in 32-bit x86 protected mode. Interrupts disabled, paging
 * disabled, and processor state as defined in multiboot specification 3.2.
 * The kernel has full control over the machine.
 */
.section .text
.global _start
.type _start, @function
_start:

    /**
     * GRUB bootloader disables interrupts for us, so we can safely assume
     * that interrupts are disabled at this time and don't have to call
     * `cli` here.
     */

    /**
     * GRUB bootloader enables the A20 line for us, so we don't need
     * to worry about enabling it. It is recommmened to have a checking
     * function to ensure that it is indeed enabled,
     * see https://wiki.osdev.org/A20_Line.
     * We will skip this step since it is not necessary (our linker script
     * puts everything above 1MiB, so the A20 line must have been enabled
     * for code here to execute!).
     */

    /** Setup the kernel stack by setting ESP to our 'stack_hi' symbol. */
    movl $stack_hi, %esp

    /**
     * Other processor state modifications and runtime supports (such as
     * enabling paging) should go here. Make sure your ESP is still 16 Bytes
     * aligned.
     */
    
    /** Set EBP to NULL for stack tracing's use. */
    xor %ebp, %ebp

    /** Pass two arguments to 'kernel_main', last -> first. */
    pushl %ebx  /** Push pointer to multiboot info. */
    pushl %eax  /** Push bootloader magic number. */
    
    /**
     * Jump to the 'kernel_main' function. According to i386 calling
     * convention, 'kernel_main' will save EBP (== NULL) to the stack.
     * Args are:
     *   4-bytes bootloader_magic
     *   4-bytes multiboot_info_addr
     */
    call kernel_main

    /**
     * Put the computer into infinite loop if it has nothing more to do after
     * the main function. The following is such a trick.
     */
    cli         /** Disable interrupts. */
halt:
    hlt         /** Halt and wait for the next interrupt. */
    jmp halt    /** Jump to 'halt' if any non-maskable interrupt occurs. */


/**
 * Set the size of the '_start' symbol as the current location '.' minus
 * the starting point. Useful when later debugging or implementing call
 * stack tracing.
 */
.size _start, . - _start
