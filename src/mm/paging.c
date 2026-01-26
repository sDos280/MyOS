#include "kernel/print.h"
#include "kernel/panic.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "mm/kheap.h"
#include "utils.h"

// paging breaks down a linear address: | Table Entry (10 bits) | Page Entry (10 bits) | Offset (12 bits) |
#define PAGE_OFFSET(addr)  (addr & 0xFFF)
#define PAGE_INDEX(addr)   ((addr >> 12) & 0x3FF)
#define TABLE_INDEX(addr)  ((addr >> 22) & 0x3FF)
#define PAGE_MASK(addr)    (addr & 0xFFFFF000) 

__attribute__((aligned(0x1000))) page_directory_t kernel_page_directory;
__attribute__((aligned(0x1000))) page_table_t kernel_page_tables[PAGING_ENTRIES_SIZE];

static page_directory_t * current_directory;

/* Symbols provided by the linker */
extern uint32_t __kernel_end;
extern uint32_t __kernel_start_v;
extern uint32_t __kernel_end_v_no_heap;

void static print_page_directory(page_directory_t* dir) {
    print_clean_screen();

    printf("Page Directory Table:\n");
    printf("====================\n");

    for (uint32_t pd_idx = 0; pd_idx < PAGING_ENTRIES_SIZE; pd_idx++) {
        page_table_t* table = dir->tables[pd_idx];
        if (!table) continue; // skip empty tables

        printf("PD Entry %d: Table at %x\n", pd_idx, (uint32_t)table);

        // Print table header
        printf("  PT Index | Present | R/W | User | Frame\n");
        printf("  --------------------------------------\n");

        for (uint32_t pt_idx = 0; pt_idx < PAGING_ENTRIES_SIZE; pt_idx++) {
            page_entry_t* entry = &table->entries[pt_idx];

            if (!entry->present) continue; // skip not-present pages

            printf("  %d        | ", pt_idx);

            // Present
            printf("%d       | ", entry->present);

            // R/W
            printf("%d   | ", entry->rw);

            // User
            printf("%d    | ", entry->user);

            // Frame
            printf("%x\n", entry->frame);
        }

        printf("\n");
    }

    printf("====================\n");
    printf("End of page directory\n");
}

void paging_init() {
    /* register fault handle first, so in case of error will found the problem fast */
    register_interrupt_handler(14, page_fault_handler);

    /* there is a need to setup this before the use of paging_map_page */
    current_directory = &kernel_page_directory;
    kernel_page_directory.physical_addr = ((uint32_t)kernel_page_directory.tables_physical) - 0xC0000000;

    /* setup kernel tables */
    for (size_t t = 0; t < PAGING_ENTRIES_SIZE; t++) {
        kernel_page_directory.tables[t] = (page_table_t *)(&(kernel_page_tables[t]));  // pointer in linear address space
        kernel_page_directory.tables_physical[t] = (PAGE_MASK((uint32_t)(&(kernel_page_tables[t]))) - 0xC0000000) | PG_WRITABLE | PG_PRESENT;  // physical address for CR3 / PDEs
    }

    /* map kernel to higher memory and  */
    for (uint32_t vaddr = (uint32_t)&__kernel_start_v; vaddr < (uint32_t)&__kernel_end; vaddr += PAGE_SIZE) 
        paging_map_page((void *)vaddr, (void *)(vaddr - 0xC0000000), PG_PRESENT | PG_WRITABLE);
    
    /* map the last page to the text screen mmio */
    paging_map_page((void *)0xFFFFF000, (void *)0xB8000, PG_PRESENT | PG_WRITABLE); /* test screen mmio size = 2*80*25 = 4000 < 4096 */

    switch_page_directory(&kernel_page_directory);
}

void switch_page_directory(page_directory_t * dir) {
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(dir->physical_addr));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void page_fault_handler(cpu_status_t* regs) {
    
    // A page fault has occurred.
    // The faulting address is stored in the CR2 register.
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // The error code gives us details of what happened.
    int not_present   = !(regs->err_code & 0x1); // Page not present
    int rw = regs->err_code & 0x2;           // Write operation?
    int us = regs->err_code & 0x4;           // Processor was in user-mode?
    int reserved = regs->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = regs->err_code & 0x10;          // Caused by an instruction fetch?

    // Output an error message.
    printf("Page fault! ( ");
    if (not_present) {printf(" not_present ");}
    if (rw) {printf("read-only ");}
    if (us) {printf("user-mode ");}
    if (reserved) {printf("reserved ");}
    printf(") at %p. eip is %p\n", faulting_address, regs->eip);
    PANIC("Page fault");
}

void paging_map_page(void* vaddr, void* paddr, uint32_t page_flags) {
    uint32_t temp_addr;

    /* mark paddr if not marked already */
    pmm_alloc_frame_addr((void *)PAGE_MASK((uint32_t)paddr));

    /* check if there is a table for vaddr */
    page_table_t * table = current_directory->tables[TABLE_INDEX((uint32_t)vaddr)];
    
    if (table == NULL) PANIC("No table was found for this addr");

    page_entry_t * entry = &table->entries[PAGE_INDEX((uint32_t)vaddr)];

    entry->present  = page_flags & PG_PRESENT;
    entry->rw       = page_flags & PG_WRITABLE;
    entry->user     = page_flags & PG_USER;
    entry->accessed = page_flags & PG_ACCESSED;
    entry->dirty    = page_flags & PG_DIRTY;
    entry->unused   = (page_flags & 0xFE0) >> 5;
    entry->frame    = (uint32_t)paddr >> 12;
}

void paging_unmap_page(void* vaddr) {
    /* check if there is a table for vaddr */
    /* FIX: ther may be a memory leak, since we don't free the tables */
    page_table_t * t = current_directory->tables[TABLE_INDEX((uint32_t)vaddr)];

    if (t == NULL) return; /* no need to unmap */

    /* unmap the page */
    page_entry_t * p = &(current_directory->tables[TABLE_INDEX((uint32_t)vaddr)]->entries[PAGE_INDEX((uint32_t)vaddr)]);
    memset(p, 0, sizeof(page_entry_t));
}

page_directory_t * paging_get_current_directory() {
    return current_directory;
}