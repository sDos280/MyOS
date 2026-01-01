#ifndef PAGING_H
#define PAGING_H

#include "kernel/description_tables.h"
#include "types.h"

//
// Page directory/table entry flags (x86 32-bit)
//
#define PG_PRESENT      (1 << 0)
#define PG_WRITABLE     (1 << 1)
#define PG_USER         (1 << 2)
#define PG_WRITE_THRU   (1 << 3)
#define PG_NO_CACHE     (1 << 4)
#define PG_ACCESSED     (1 << 5)
#define PG_DIRTY        (1 << 6)
#define PG_4MB          (1 << 7)   // huge page
#define PG_GLOBAL       (1 << 8)

#define PAGE_SIZE 0x1000

#define PAGING_ENTRIES_SIZE 1024

typedef struct page_entry_struct {
    uint32_t present    : 1;  // Page present in memory
    uint32_t rw         : 1;  // Read-only if clear, readwrite if set
    uint32_t user       : 1;  // Supervisor level only if clear
    uint32_t accessed   : 1;  // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;  // Has the page been written to since last refresh?
    uint32_t unused     : 7;  // Amalgamation of unused and reserved bits
    uint32_t frame      : 20; // Frame address (shifted right 12 bits)
} page_entry_t;

typedef struct __attribute__ ((aligned (0x1000))) page_table_struct {
    page_entry_t entries[PAGING_ENTRIES_SIZE];
} page_table_t;

typedef struct __attribute__ ((aligned (0x1000))) page_directory_struct {
    /**
       Array of pointers to page-tables.
    **/
    page_table_t *tables[PAGING_ENTRIES_SIZE];
    /**
       Array of pointers to the pagetables above, but gives their *physical*
       location, for loading into the CR3 register.
    **/
    uint32_t tables_physical[PAGING_ENTRIES_SIZE];

    /**
       The physical address of physical_addr. This comes into play
       when we get our kernel heap allocated and the directory
       may be in a different location in virtual memory.
    **/
    uint32_t physical_addr;
} page_directory_t;

//
// Initialization
//
void paging_init();
void switch_page_directory(page_directory_t * dir);  // switch to the new page directory
void page_fault_handler(cpu_status_t* regs);  // the page fault handler

void paging_map_page(void* vaddr, void* paddr, uint32_t page_flags);
void paging_unmap_page(void* vaddr);
void* paging_get_mapping(void* vaddr);

void* paging_alloc_page(uint64_t flags);
void  paging_free_page(void* vaddr);

void* paging_alloc_pages(size_t count, uint64_t flags);
void  paging_free_pages(void* vaddr, size_t count);

page_directory_t * paging_get_current_directory();

#endif // PAGING_H