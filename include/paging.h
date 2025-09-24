#ifndef PAGING_H
#define PAGING_H

#include "types.h"
#include "description_tables.h"

#define PAGE_SIZE 0x1000           // Size of a page/frame in bytes (4096)
#define PAGE_MASK 0xFFFFF000

#define PAGE_PRESENT    0b00001
#define PAGE_READWRITE  0b00010
#define PAGE_READ       0b00000
#define PAGE_USER       0b00100
#define PAGE_SUPERVISOR 0b00000

typedef struct page_struct {
    uint32_t present    : 1;  // Page present in memory
    uint32_t rw         : 1;  // Read-only if clear, readwrite if set
    uint32_t user       : 1;  // Supervisor level only if clear
    uint32_t accessed   : 1;  // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;  // Has the page been written to since last refresh?
    uint32_t unused     : 7;  // Amalgamation of unused and reserved bits
    uint32_t frame      : 20; // Frame address (shifted right 12 bits)
} page_t;

typedef struct __attribute__ ((aligned (0x1000))) page_table_struct {
    page_t entries[1024];
} page_table_t;

typedef struct __attribute__ ((aligned (0x1000))) page_directory_struct {
    /**
       Array of pointers to pagetables.
    **/
    page_table_t *tables[1024];
    /**
       Array of pointers to the pagetables above, but gives their *physical*
       location, for loading into the CR3 register.
    **/
    uint32_t tablesPhysical[1024];

    /**
       The physical address of tablesPhysical. This comes into play
       when we get our kernel heap allocated and the directory
       may be in a different location in virtual memory.
    **/
    uint32_t physicalAddr;
} page_directory_t;

void initialize();
void switch_page_directory(page_directory_t * dir);  // switch to the new page directory
void identity_map_kernal(); // identity maps the kernal, apply the changes to the kernal page directory
void page_fault(registers_t* regs);  // the page fault handler

#endif // PAGING_H