#include "paging.h"
#include "utils.h"
#include "panic.h"
#include "kheap.h"
#include "screen.h"

// paging breaks down a linear address: | Table Entry (10 bits) | Page Entry (10 bits) | Offset (12 bits) |
#define OFFSET(addr) (addr & 0b111111111111)
#define PAGE(addr)   ((addr >> 12) & 0b1111111111)
#define TABLE(addr)  ((addr >> 22) & 0b1111111111)

extern uint32_t end; // end is defined in the linker script
uint32_t last_address;

extern uint32_t placement_address; // defined in kheap.c

page_directory_t kernel_directory;
page_directory_t * current_directory;

void static print_page_directory(page_directory_t* dir) {
    clear_screen();

    print("Page Directory Table:\n");
    print("====================\n");

    for (uint32_t pd_idx = 0; pd_idx < 1024; pd_idx++) {
        page_table_t* table = dir->tables[pd_idx];
        if (!table) continue; // skip empty tables

        print("PD Entry ");
        print_int(pd_idx);
        print(": Table at ");
        print_hex((uint32_t)table);
        print("\n");

        // Print table header
        print("  PT Index | Present | R/W | User | Frame\n");
        print("  --------------------------------------\n");

        for (uint32_t pt_idx = 0; pt_idx < 1024; pt_idx++) {
            page_t* entry = &table->entries[pt_idx];

            if (!entry->present) continue; // skip not-present pages

            print("  ");
            print_int(pt_idx);
            print("        | ");

            // Present
            print_int(entry->present);
            print("       | ");

            // R/W
            print_int(entry->rw);
            print("   | ");

            // User
            print_int(entry->user);
            print("    | ");

            // Frame
            print_hex(entry->frame);
            print("\n");
        }

        print("\n");
    }

    print("====================\n");
    print("End of page directory\n");
}

void initialize_paging() {
    last_address = (uint32_t)&end; // ceil to the next page

    register_interrupt_handler(14, page_fault);
}

void switch_page_directory(page_directory_t * dir) {
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void identity_map_kernal() {
    memset(&kernel_directory, 0, sizeof(page_directory_t));
    
    kernel_directory.physicalAddr = (uint32_t)&kernel_directory;
    current_directory = &kernel_directory;

    // allocate all the tables needed for the kernal in the start
    size_t table_count = TABLE(last_address) + 1; // +1, index to count
    for (size_t t = 0; t < table_count; t++) {
        // allocate a page for a page table (return is physical address)
        uint32_t phys = alloc_unfreable_phys(0x1000, 1);

        // Clear the physical page so entries start as not-present
        memset((void *)phys, 0, 0x1000);

        // store the physical address and a usable virtual pointer (before paging these are identity)
        kernel_directory.tables[t] = (page_table_t *)phys;        // pointer in linear address space
        kernel_directory.tablesPhysical[t] = phys | 3;                // physical address for CR3 / PDEs
    }

    // allocate all the tables needed for the kernal heap
    table_count = TABLE(KHEAP_INITIAL_SIZE) + 1; // +1, index to count
    for (size_t t = TABLE(KHEAP_START); t < TABLE(KHEAP_START) + table_count; t++) {
        // allocate a page for a page table (return is physical address)
        uint32_t phys = alloc_unfreable_phys(0x1000, 1);

        // Clear the physical page so entries start as not-present
        memset((void *)phys, 0, 0x1000);

        // store the physical address and a usable virtual pointer (before paging these are identity)
        kernel_directory.tables[t] = (page_table_t *)phys;        // pointer in linear address space
        kernel_directory.tablesPhysical[t] = phys | 3;                // physical address for CR3 / PDEs
    }

    // create kernel identity map
    for(uint32_t addr = 0; addr < placement_address; addr += PAGE_SIZE) {
        page_table_t * table = kernel_directory.tables[TABLE(addr)];
        table->entries[PAGE(addr)].present = 1;
        table->entries[PAGE(addr)].rw = 1;
        table->entries[PAGE(addr)].user = 0;
        table->entries[PAGE(addr)].frame = addr >> 12; 
    }

    // create kernel heap identity map
    for(uint32_t addr = KHEAP_START; addr < KHEAP_START + KHEAP_INITIAL_SIZE; addr += PAGE_SIZE) {
        page_table_t * table = kernel_directory.tables[TABLE(addr)];
        table->entries[PAGE(addr)].present = 1;
        table->entries[PAGE(addr)].rw = 1;
        table->entries[PAGE(addr)].user = 0;
        table->entries[PAGE(addr)].frame = addr >> 12; 
    }

    print("KHEAP_START table pointer: ");
    print_hex((uint32_t)kernel_directory.tables[TABLE(KHEAP_START)]);
    print("\n");

    switch_page_directory(&kernel_directory);
}

void page_fault(registers_t* regs) {
    
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
    print("Page fault! ( ");
    if (present) {print("present ");}
    if (rw) {print("read-only ");}
    if (us) {print("user-mode ");}
    if (reserved) {print("reserved ");}
    print(") at ");
    print_hex(faulting_address);
    print("\n");
    PANIC("Page fault");
}

