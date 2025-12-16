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

extern uint32_t __kernel_end;  // defined in the linker

void static print_page_directory(page_directory_t* dir) {
    print_clean_screen();

    print_const_string("Page Directory Table:\n");
    print_const_string("====================\n");

    for (uint32_t pd_idx = 0; pd_idx < PAGING_ENTRIES_SIZE; pd_idx++) {
        page_table_t* table = dir->tables[pd_idx];
        if (!table) continue; // skip empty tables

        printf("PD Entry %d: Table at %x\n", pd_idx, (uint32_t)table);

        // Print table header
        print_const_string("  PT Index | Present | R/W | User | Frame\n");
        print_const_string("  --------------------------------------\n");

        for (uint32_t pt_idx = 0; pt_idx < PAGING_ENTRIES_SIZE; pt_idx++) {
            page_t* entry = &table->entries[pt_idx];

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

        print_const_string("\n");
    }

    print_const_string("====================\n");
    print_const_string("End of page directory\n");
}

void paging_init() {
    /* remember currently vm = pm */
    memset(&kernel_page_directory, 0, sizeof(page_directory_t));
    memset(&kernel_page_tables, 0, sizeof(page_table_t) * PAGING_ENTRIES_SIZE);

    /* there is a need to setup this before the use of paging_map_page */
    current_directory = &kernel_page_directory;

    /* link kernel page directory to her tables */
    size_t table_count = TABLE_INDEX(__kernel_end) + 1; // +1, index to count
    printf("Kernel's Page directory table count: %d\n", table_count);
    
    for (size_t t = 0; t < PAGING_ENTRIES_SIZE; t++) {
        kernel_page_directory.tables[t] = (page_table_t *)(&(kernel_page_tables[t]));  // pointer in linear address space
        kernel_page_directory.tables_physical[t] = (uint32_t)(&(kernel_page_tables[t])) | PG_WRITABLE | PG_PRESENT;  // physical address for CR3 / PDEs
    }

    /* identity map */
    for (size_t addr = 0; addr <= __kernel_end; addr += PAGE_SIZE)
        paging_map_page((void *)addr, (void *)addr, PG_WRITABLE | PG_PRESENT, PG_WRITABLE | PG_PRESENT);
    
    
    register_interrupt_handler(14, page_fault_handler);
    switch_page_directory(&kernel_page_directory);
}

void switch_page_directory(page_directory_t * dir) {
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(&(dir->tables_physical)));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void page_fault_handler(registers_t* regs) {
    
    // A page fault has occurred.
    // The faulting address is stored in the CR2 register.
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // The error code gives us details of what happened.
    int present   = !(regs->err_code & 0x1); // Page not present
    int rw = regs->err_code & 0x2;           // Write operation?
    int us = regs->err_code & 0x4;           // Processor was in user-mode?
    int reserved = regs->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = regs->err_code & 0x10;          // Caused by an instruction fetch?

    // Output an error message.
    printf("Page fault! ( ");
    if (present) {printf("present ");}
    if (rw) {printf("read-only ");}
    if (us) {printf("user-mode ");}
    if (reserved) {printf("reserved ");}
    printf(") at %p\n", faulting_address);
    PANIC("Page fault");
}

uint8_t paging_map_page(void* vaddr, void* paddr, uint32_t table_flags, uint32_t page_flags) {
    uint32_t temp_addr;

    /* mark paddr if not marked already */
    pmm_alloc_frame_addr((void *)PAGE_MASK((uint32_t)paddr));

    /* check if there is a table for vaddr */
    page_table_t * table = current_directory->tables[TABLE_INDEX((uint32_t)vaddr)];

    if (table == NULL)  { /* we need to allocate a new table for this vaddr */
        /* NOTE: the vheap and the kheap is mapped to the same place in memory, a more general approch is needed */
        uint32_t vpaddr_table;

        vpaddr_table = (uint32_t)kalloc(PAGE_SIZE * 2); /* kalloc may return an non page aligned address, so double memory is needed */

        if (kalloc == NULL) return 1;  /* no memory in heap, error */

        /* PAGE_MASK(vpaddr_table) + PAGE_SIZE is needed because we want to round to the upper page */
        current_directory->tables[TABLE_INDEX((uint32_t)vaddr)] = (void *)(PAGE_MASK(vpaddr_table) + PAGE_SIZE);
        current_directory->tables_physical[TABLE_INDEX((uint32_t)vaddr)] = (PAGE_MASK(vpaddr_table) + PAGE_SIZE) | table_flags;
    }

    table->entries[PAGE_INDEX((uint32_t)paddr)].present = page_flags & 0x1;
    table->entries[PAGE_INDEX((uint32_t)paddr)].rw = page_flags & 0x2;
    table->entries[PAGE_INDEX((uint32_t)paddr)].user = page_flags & 0x4;
    table->entries[PAGE_INDEX((uint32_t)paddr)].accessed = page_flags & 0x8;
    table->entries[PAGE_INDEX((uint32_t)paddr)].dirty = page_flags & 0x10;
    table->entries[PAGE_INDEX((uint32_t)paddr)].unused = page_flags & 0xFE0;
    table->entries[PAGE_INDEX((uint32_t)paddr)].frame = (uint32_t)paddr >> 12;

    return 0;
}

void paging_unmap_page(void* vaddr) {
    /* check if there is a table for vaddr */
    /* FIX: ther may be a memory leak, since we don't free the tables */
    page_table_t * t = current_directory->tables[TABLE_INDEX((uint32_t)vaddr)];

    if (t == NULL) return; /* no need to unmap */

    /* unmap the page */
    page_t * p = &(current_directory->tables[TABLE_INDEX((uint32_t)vaddr)]->entries[PAGE_INDEX((uint32_t)vaddr)]);
    memset(p, 0, sizeof(page_t));
}