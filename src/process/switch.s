/**
 * Context swtiching snippet from xv6,
 * see https://github.com/mit-pdos/xv6-public/blob/master/swtch.S.
 *
 * Call signature:
 *   void context_switch(process_context_t **old, process_context_t *new);
 *
 * Saves the current registers on the current stack, interpreting as
 * a context_t struct, and saves the address in `*old`. Then, switches
 * stack to `new` and pop previously-saved context registers.
 */


.global context_switch
.type context_switch, @function
context_switch:

    movl 4(%esp), %eax      /** old */
    movl 8(%esp), %edx      /** new */

    /** Save callee-saved registers of old (notice order). */
    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi

    /** Swtich stack to the new one. */
    movl %esp, (%eax)
    movl %edx, %esp

    /** Load callee-saved registers of new (notice order). */
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp

    /** Return to new's EIP (so not being popped). */
    ret
