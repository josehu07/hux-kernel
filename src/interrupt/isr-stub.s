/**
 * ISR handler common stub.
 *
 * We make 32 wrapper for all 32 trap gate ISRs. They all call the handler
 * stub function, which in turn calls our `isr_handler` function in `isr.c`.
 *
 * Notice that for ISRs where the CPU do not push a error code, we push a
 * dummy error code there, in order to maintain a unified function
 * signature.
 */


/**
 * Make 32 wrappers.
 * 
 * These are what get registered in the IDT. Trap gates will automatically
 * disable interrupts and restore interrupts when entering and leaving
 * ISR, so no need of `cli` and `sti` here.
 *
 * Traps # 8, 10, 11, 12, 13, 14, 17, or 21 will have a CPU-pushed error
 * code, and for others we push a dummy one.
 */
.global isr0
.type isr0, @function
isr0:
    pushl $0                /** Dummy error code. */
    pushl $0                /** Interrupt index code. */
    jmp isr_handler_stub    /** Jump to handler stub. */

.global isr1
.type isr1, @function
isr1:
    pushl $0                /** Dummy error code. */
    pushl $1                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr2
.type isr2, @function
isr2:
    pushl $0                /** Dummy error code. */
    pushl $2                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr3
.type isr3, @function
isr3:
    pushl $0                /** Dummy error code. */
    pushl $3                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr4
.type isr4, @function
isr4:
    pushl $0                /** Dummy error code. */
    pushl $4                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr5
.type isr5, @function
isr5:
    pushl $0                /** Dummy error code. */
    pushl $5                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr6
.type isr6, @function
isr6:
    pushl $0                /** Dummy error code. */
    pushl $6                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr7
.type isr7, @function
isr7:
    pushl $0                /** Dummy error code. */
    pushl $7                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr8
.type isr8, @function
isr8:
    pushl $8                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr9
.type isr9, @function
isr9:
    pushl $0                /** Dummy error code. */
    pushl $9                /** Interrupt index code. */
    jmp isr_handler_stub

.global isr10
.type isr10, @function
isr10:
    pushl $10               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr11
.type isr11, @function
isr11:
    pushl $11               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr12
.type isr12, @function
isr12:
    pushl $12               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr13
.type isr13, @function
isr13:
    pushl $13               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr14
.type isr14, @function
isr14:
    pushl $14               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr15
.type isr15, @function
isr15:
    pushl $0                /** Dummy error code. */
    pushl $15               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr16
.type isr16, @function
isr16:
    pushl $0                /** Dummy error code. */
    pushl $16               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr17
.type isr17, @function
isr17:
    pushl $17               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr18
.type isr18, @function
isr18:
    pushl $0                /** Dummy error code. */
    pushl $18               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr19
.type isr19, @function
isr19:
    pushl $0                /** Dummy error code. */
    pushl $19               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr20
.type isr20, @function
isr20:
    pushl $0                /** Dummy error code. */
    pushl $20               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr21
.type isr21, @function
isr21:
    pushl $21               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr22
.type isr22, @function
isr22:
    pushl $0                /** Dummy error code. */
    pushl $22               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr23
.type isr23, @function
isr23:
    pushl $0                /** Dummy error code. */
    pushl $23               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr24
.type isr24, @function
isr24:
    pushl $0                /** Dummy error code. */
    pushl $24               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr25
.type isr25, @function
isr25:
    pushl $0                /** Dummy error code. */
    pushl $25               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr26
.type isr26, @function
isr26:
    pushl $0                /** Dummy error code. */
    pushl $26               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr27
.type isr27, @function
isr27:
    pushl $0                /** Dummy error code. */
    pushl $27               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr28
.type isr28, @function
isr28:
    pushl $0                /** Dummy error code. */
    pushl $28               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr29
.type isr29, @function
isr29:
    pushl $0                /** Dummy error code. */
    pushl $29               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr30
.type isr30, @function
isr30:
    pushl $0                /** Dummy error code. */
    pushl $30               /** Interrupt index code. */
    jmp isr_handler_stub

.global isr31
.type isr31, @function
isr31:
    pushl $0                /** Dummy error code. */
    pushl $31               /** Interrupt index code. */
    jmp isr_handler_stub


/**
 * Handler stub. Saves processor state, load kernel data segment, pushes
 * interrupt number and error code as arguments and calls the `isr_handler`
 * function in `isr.c`. When the function returns, we restore the stack
 * frame.
 *
 * Be sure that kenrel code segment is at `0x10`.
 */
.extern isr_handler  /** extern `isr_handler` from C code. */

isr_handler_stub:
    /** Saves EDI, ESI, EBP, Current ESP, EBX, EDX, ECX, EAX. */
    pushal

    /** Saves DS (as lower 16 bits). */
    movw %ds, %ax
    pushl %eax

    /**
     * Push a pointer to the current stuff on stack, which are:
     *   - DS
     *   - EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX
     *   - Interrupt number, Error code
     *   - EIP, CS, EFLAGS, User's ESP, SS (these are previously pushed
     *     by the processor when entering this interrupt)
     *
     * This pointer is the argument of our `isr_handler` function.
     */
    movl %esp, %eax
    pushl %eax

    /** Load kernel mode data segment descriptor. */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    /** == Calls the ISR handler. == **/
    call isr_handler
    /** == ISR handler finishes.  == **/

    addl $4, %esp   /** Cleans up the pointer argument. */

    /** Restore previous segment descriptor. */
    popl %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    /** Restores EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX. */
    popal

    addl $8, %esp   /** Cleans up error code and ISR number. */

    iret            /** This pops EIP, CS, EFLAGS, User's ESP, SS. */
