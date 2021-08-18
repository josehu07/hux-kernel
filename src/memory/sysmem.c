/**
 * Syscalls related to user memory allocation.
 */


#include <stdint.h>

#include "sysmem.h"
#include "paging.h"

#include "../common/debug.h"
#include "../common/string.h"

#include "../interrupt/syscall.h"

#include "../process/scheduler.h"


/** int32_t setheap(uint32_t new_top); */
int32_t
syscall_setheap(void)
{
    process_t *proc = running_proc();
    uint32_t new_top;

    if (!sysarg_get_uint(0, &new_top))
        return SYS_FAIL_RC;
    if (new_top < proc->heap_high) {
        warn("setheap: does not support shrinking heap");
        return SYS_FAIL_RC;
    }
    if (new_top > proc->stack_low) {
        warn("setheap: heap meets stack, heap overflow");
        return SYS_FAIL_RC;
    }

    /**
     * Compare with current heap page allocation top. If exceeds the top
     * page, allocate new pages accordingly.
     */
    uint32_t heap_page_high = ADDR_PAGE_ROUND_UP(proc->heap_high);

    for (uint32_t vaddr = heap_page_high;
         vaddr < new_top;
         vaddr += PAGE_SIZE) {
        pte_t *pte = paging_walk_pgdir(proc->pgdir, vaddr, true);
        if (pte == NULL) {
            warn("setheap: cannot walk pgdir, out of kheap memory?");
            paging_unmap_range(proc->pgdir, heap_page_high, vaddr);
            return SYS_FAIL_RC;
        }
        uint32_t paddr = paging_map_upage(pte, true);
        if (paddr == 0) {
            warn("setheap: cannot map new page, out of memory?");
            paging_unmap_range(proc->pgdir, heap_page_high, vaddr);
            return SYS_FAIL_RC;
        }
        memset((char *) paddr, 0, PAGE_SIZE);
    }

    proc->heap_high = new_top;
    return 0;
}
