/**
 * Load the GDT.
 * 
 * The `gdt_load` function will receive a pointer to the `gdtr` value. We
 * will then load the `gdtr` value into GDTR register by the `lgdt`
 * instruction.
 *
 * Assumes flat memory model. The first argument is pointer to `gdtr`, the
 * second argument is the index of data segment selector and the third
 * argument is the index of code segment selector.
 */


.global gdt_load
.type gdt_load, @function
gdt_load:
    
    /** Get first two arguments. */
    mov 4(%esp), %edx
    mov 8(%esp), %eax

    /** Load the GDT. */
    lgdt (%edx)

    /** Load all data segment selectors. */
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs

    /** Load stack segment as well. */
    mov %eax, %ss

    
    /** Load code segment by doing a far jump. */
    pushl 12(%esp)  /** Push FAR pointer on stack. */
    push $.setcs    /** Push offset of FAR JMP. */
    ljmp *(%esp)    /** Perform FAR JMP. */

.setcs:
    add $8, %esp    /** Restore stack. */
    
    ret
