#include "kernel/panic.h"
#include "kernel/syscall.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "errno.h"

void syscall_init() {
    register_interrupt_handler(0x80, syscall_handler);
}

void * mmap_irq_handler(void *addr, size_t length, uint32_t flags) {
    uint32_t pflags = 0;

    if (flags | PPROT_WRITE)
        pflags |= PG_WRITABLE; 
    
    if (flags | PPROT_USER)
        pflags |= PG_USER; 

    pflags |= PG_PRESENT;

    void * paddr = pmm_alloc_frames((length + PAGE_SIZE) / PAGE_SIZE);

    paging_map_page(addr, paddr, pflags);

    /* should probably return something better */
    return addr;
}

uint32_t syscall_handler(cpu_status_t * regs) {
    uint32_t out = 0;

    switch (regs->eax) {
        case SYSC_MMAP:
            out = mmap_irq_handler(regs->ebx, regs->ecx, regs->edx);
            break;
        
        default:
            printf("Invalid syscall number");
            return -EGENERAL;
    }

    /* setup the return value */
    regs->eax = out;

    /* there are a lot of problems with the return types */
    return out;
}

void *mmap(void *addr, size_t length, uint32_t flags) {
    void *ret;

    asm volatile(
        "int $0x80"
        : "=a"(ret)                 // return value comes in EAX
        : "a"(SYSC_MMAP),           // syscall number in EAX
          "b"(addr),                // 1st argument -> EBX
          "c"(length),              // 2nd argument -> ECX
          "d"(flags)                // 3rd argument -> EDX
        : "memory"
    );

    return ret;
}