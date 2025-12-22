#include "mm/paging.h"
#include "multiboot.h"
#include "types.h"
#include "utils.h"

// paging breaks down a linear address: | Table Entry (10 bits) | Page Entry (10 bits) | Offset (12 bits) |
#define PAGE_OFFSET(addr)  (addr & 0xFFF)
#define PAGE_INDEX(addr)   ((addr >> 12) & 0x3FF)
#define TABLE_INDEX(addr)  ((addr >> 22) & 0x3FF)
#define PAGE_MASK(addr)    (addr & 0xFFFFF000) 

__attribute__((aligned(0x1000))) __attribute__ ((section(".boot.bss"))) page_directory_t kernel_temp_page_directory;
__attribute__((aligned(0x1000))) __attribute__ ((section(".boot.bss"))) page_table_t kernel_temp_page_tables[PAGING_ENTRIES_SIZE];

extern uint32_t __kernel_start;  // defined in the linker
extern uint32_t __kernel_end_no_heap;  // defined in the linker

static void inline __attribute__((always_inline)) memset_inline(void* dest, uint8_t val, uint32_t len);

void boot_ipl (multiboot_info_t* multiboot_info_structure, uint32_t multiboot_magic) __attribute__ ((section(".boot.text")));

static void inline __attribute__((always_inline)) memset_inline(void* dest, uint8_t val, uint32_t len) {
    /* this needs to be inline because else we would need to check for higher half checks */
    uint8_t* dst = (uint8_t*)dest;
    while(len-- > 0)
        *dst++ = (uint8_t)val;
}

void boot_ipl(multiboot_info_t* multiboot_info_structure, uint32_t multiboot_magic) {
    /* remember currently vm = pm */

    /* set the tables and directories to 0 */
    uint32_t len = sizeof(page_directory_t);
    uint8_t* dst = (uint8_t*)&kernel_temp_page_directory;

    while(len-- > 0)
        *dst++ = 0;
    
    len = sizeof(page_table_t) * PAGING_ENTRIES_SIZE;
    dst = (uint8_t*)&kernel_temp_page_tables;
    
    while(len-- > 0)
        *dst++ = 0;

    kernel_temp_page_directory.physical_addr = (uint32_t)&kernel_temp_page_directory;

    uint32_t kernel_start = (uint32_t)&__kernel_start;
    uint32_t kernel_end_no_heap_lower = (uint32_t)(&__kernel_end_no_heap) - 0xC0000000;
    int32_t  kernel_table_index = -1;  /* start with -1 so update the tables would update */

    /* setup lower half paging */
    for (uint32_t paddr = 0; paddr <= kernel_end_no_heap_lower; paddr += PAGE_SIZE) {
        if (kernel_table_index != (int32_t)TABLE_INDEX(paddr)) { /* a new table index have been encounter */
            kernel_table_index = TABLE_INDEX(paddr); /* update table index */
            /* update table address */
            kernel_temp_page_directory.tables[kernel_table_index] = &kernel_temp_page_tables[kernel_table_index];
            kernel_temp_page_directory.tables_physical[kernel_table_index] = 
                (uint32_t)&kernel_temp_page_tables[kernel_table_index] | PG_WRITABLE | PG_PRESENT;
        }

        page_table_t * table = kernel_temp_page_directory.tables[kernel_table_index];

        table->entries[PAGE_INDEX((uint32_t)paddr)].present = 1;
        table->entries[PAGE_INDEX((uint32_t)paddr)].rw = 1;
        table->entries[PAGE_INDEX((uint32_t)paddr)].user = 0;
        table->entries[PAGE_INDEX((uint32_t)paddr)].accessed = 0;
        table->entries[PAGE_INDEX((uint32_t)paddr)].dirty = 0;
        table->entries[PAGE_INDEX((uint32_t)paddr)].unused = 0;
        table->entries[PAGE_INDEX((uint32_t)paddr)].frame = (uint32_t)paddr >> 12;
    }

    /* switch directory */
    asm volatile("mov %0, %%cr3":: "r"(&(kernel_temp_page_directory.tables_physical)));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));

    /* temp instructions */
    for (uint32_t gg = 0; gg < 1000; gg += 2) {
        gg--;
    }
}