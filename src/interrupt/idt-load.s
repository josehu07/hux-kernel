/**
 * Load the IDT.
 * 
 * The `idt_load` function will receive a pointer to the `idtr` value. We
 * will then load the `idtr` value into IDTR register by the `lidt`
 * instruction.
 */


.global idt_load
.type idt_load, @function
idt_load:
    
    /** Get the argument. */
    movl 4(%esp), %eax

    /** Load the IDT. */
    lidt (%eax)

    ret
