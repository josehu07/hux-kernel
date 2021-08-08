/**
 * Virtual address space layout of a user process.
 *
 * Each process has an address space of size 1GiB, so the valid virtual
 * addresses a process could issue range from `0x00000000` to `0x40000000`.
 * 
 *   - The kernel's virtual address space of size 512MiB is mapped to the
 *     bottom-most pages (`0x00000000` to `0x20000000`)
 *     
 *   - The ELF binary (`.text` + `.data` + `.bss` sections) starts from
 *     `0x20000000` (and takes the size of at most 1MiB)
 *     
 *   - The stack begins at the top-most page (`0x40000000`), grows downwards
 *   
 *   - The region in-between is usable by the process heap, growing upwards
 */


#ifndef LAYOUT_H
#define LAYOUT_H


/** Virtual address space size: 1GiB. */
#define USER_MAX 0x40000000

/**
 * The lower-half maps the kernel for simplicity (in contrast to
 * typical higher-half design). Application uses the higher-half
 * starting from this address.
 */
#define USER_BASE 0x20000000

/**
 * Hux allows user executable to take up at most 1MiB space, starting
 * at USER_BASE and ending no higher than HEAP_BASE.
 */
#define HEAP_BASE (USER_BASE + 0x00100000)

/** Max stack size limit is 4MiB. */
#define STACK_MIN (USER_MAX - 0x00400000)


#endif
