#include "boot/boot_ipl.h"
#include "boot/boot_screen.h"
#include "boot/boot_utils.h"

// paging breaks down a linear address: | Table Entry (10 bits) | Page Entry (10 bits) | Offset (12 bits) |
#define PAGE_OFFSET(addr)  (addr & 0xFFF)
#define PAGE_INDEX(addr)   ((addr >> 12) & 0x3FF)
#define TABLE_INDEX(addr)  ((addr >> 22) & 0x3FF)
#define PAGE_MASK(addr)    (addr & 0xFFFFF000) 

__attribute__((aligned(0x1000))) __attribute__ ((section(".boot.bss"))) page_directory_t kernel_temp_page_directory;
__attribute__((aligned(0x1000))) __attribute__ ((section(".boot.bss"))) page_table_t kernel_temp_page_tables[PAGING_ENTRIES_SIZE];


static __attribute__((section(".boot.data"))) const char boot_fmt[] = "vaddr: %p, paddr: %p\n";

extern uint32_t __kernel_start;  // defined in the linker
extern uint32_t __kernel_end_no_heap;  // defined in the linker
extern uint32_t __kernel_end;  // defined in the linker
extern uint32_t __kernel_start_v; // defined in the linker

uint8_t boot_map_page(page_directory_t* d, void* vaddr, void* paddr, uint32_t page_flags) {
    /* Note: the table must be allocated */
    page_table_t * table = d->tables[TABLE_INDEX((uint32_t)vaddr)];

    table->entries[PAGE_INDEX((uint32_t)vaddr)].present = page_flags & 0x1;
    table->entries[PAGE_INDEX((uint32_t)vaddr)].rw = page_flags & 0x2;
    table->entries[PAGE_INDEX((uint32_t)vaddr)].user = page_flags & 0x4;
    table->entries[PAGE_INDEX((uint32_t)vaddr)].accessed = page_flags & 0x8;
    table->entries[PAGE_INDEX((uint32_t)vaddr)].dirty = page_flags & 0x10;
    table->entries[PAGE_INDEX((uint32_t)vaddr)].unused = page_flags & 0xFE0;
    table->entries[PAGE_INDEX((uint32_t)vaddr)].frame = (uint32_t)paddr >> 12;

    return 0;
}

void boot_ipl(multiboot_info_t* multiboot_info_structure, uint32_t multiboot_magic) {
    /* remember currently vm = pm */
    
    /* set the tables and directories to 0 */
    boot_memset(&kernel_temp_page_directory, 0, sizeof(page_directory_t));
    boot_memset(&kernel_temp_page_tables, 0, sizeof(page_table_t) * PAGING_ENTRIES_SIZE);

    uint32_t kernel_start = (uint32_t)&__kernel_start;
    int32_t  kernel_table_index = -1;  /* start with -1 so update the tables would update */

    /* setup all kernel's table directory general data */
    kernel_temp_page_directory.physical_addr = (uint32_t)&kernel_temp_page_directory;
    for (size_t t = 0; t < PAGING_ENTRIES_SIZE; t++) {
        kernel_temp_page_directory.tables[t] = (page_table_t *)(&(kernel_temp_page_tables[t]));  // pointer in linear address space
        kernel_temp_page_directory.tables_physical[t] = (uint32_t)(&(kernel_temp_page_tables[t])) | PG_WRITABLE | PG_PRESENT;  // physical address for CR3 / PDEs
    }

    /* setup lower half identity map paging */
    uint32_t kernel_end_no_heap_lower = (uint32_t)(&__kernel_end_no_heap) - 0xC0000000;

    for (uint32_t paddr = 0; paddr < kernel_end_no_heap_lower; paddr += PAGE_SIZE) 
        boot_map_page(&kernel_temp_page_directory, (void *)paddr, (void *)paddr, PG_WRITABLE | PG_PRESENT);

    /* setup higher half paging */
    for (uint32_t vaddr = &__kernel_start_v; vaddr < (uint32_t)&__kernel_end; vaddr += PAGE_SIZE) {
        boot_map_page(&kernel_temp_page_directory, (void *)vaddr, (void *)(vaddr - 0xC0000000), PG_WRITABLE | PG_PRESENT);
    }

    /* switch directory */
    asm volatile("mov %0, %%cr3":: "r"(&(kernel_temp_page_directory.tables_physical)));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}