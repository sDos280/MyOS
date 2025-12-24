#include "boot/boot_ipl.h"
#include "boot/boot_utils.h"

/*
 * 32-bit paging address layout:
 * | Page Directory Index (10) | Page Table Index (10) | Offset (12) |
 */
#define PAGE_OFFSET(addr)   ((addr) & 0xFFF)
#define PAGE_TABLE_INDEX(a) (((a) >> 12) & 0x3FF)
#define PAGE_DIR_INDEX(a)   (((a) >> 22) & 0x3FF)
#define PAGE_ALIGN(addr)   ((addr) & 0xFFFFF000)

/*
 * Temporary paging structures used during early boot.
 * They live in .boot.bss so they are identity-mapped and zero-initialized.
 */
__attribute__((aligned(0x1000), section(".boot.bss")))
page_directory_t boot_page_directory;

__attribute__((aligned(0x1000), section(".boot.bss")))
page_table_t boot_page_tables[PAGING_ENTRIES_SIZE];

/* Symbols provided by the linker */
extern uint32_t __kernel_start;
extern uint32_t __kernel_start_v;
extern uint32_t __kernel_end_no_heap;
extern uint32_t __kernel_end;

/*
 * Maps a single 4KB page.
 * Assumes the page table already exists.
 */
static uint8_t
boot_map_page(page_directory_t *pd, void *vaddr, void *paddr, uint32_t flags)
{
    uint32_t va = (uint32_t)vaddr;
    uint32_t pa = (uint32_t)paddr;

    page_table_t *pt = pd->tables[PAGE_DIR_INDEX(va)];
    page_entry_t * entry = &pt->entries[PAGE_TABLE_INDEX(va)];

    entry->present  = flags & PG_PRESENT;
    entry->rw       = flags & PG_WRITABLE;
    entry->user     = flags & PG_USER;
    entry->accessed = flags & PG_ACCESSED;
    entry->dirty    = flags & PG_DIRTY;
    entry->unused   = (flags & 0xFE0) >> 5;
    entry->frame    = pa >> 12;

    return 0;
}

/*
 * Initial Paging Loader (IPL)
 * Sets up identity mapping + higher-half kernel mapping,
 * then enables paging.
 */
void boot_ipl(multiboot_info_t *mb_info, uint32_t mb_magic) {
    // At this point: virtual == physical
    
    /* Clear paging structures */
    boot_memset(&boot_page_directory, 0, sizeof(page_directory_t));
    boot_memset(boot_page_tables, 0,
                sizeof(page_table_t) * PAGING_ENTRIES_SIZE);

    /*
     * Initialize page directory entries.
     * Linear pointers are used before paging,
     * physical addresses are written to PDEs.
     */
    boot_page_directory.physical_addr = (uint32_t)&boot_page_directory;

    for (size_t i = 0; i < PAGING_ENTRIES_SIZE; i++) {
        boot_page_directory.tables[i] =  &boot_page_tables[i];

        boot_page_directory.tables_physical[i] = 
            (uint32_t)&boot_page_tables[i] | PG_PRESENT | PG_WRITABLE;
    }

    /*
     * Identity map the lower memory region.
     */
    uint32_t kernel_end_phys = (uint32_t)&__kernel_end_no_heap - 0xC0000000;

    for (uint32_t addr = 0; addr < kernel_end_phys; addr += PAGE_SIZE) 
        boot_map_page(&boot_page_directory, (void *)addr, (void *)addr, PG_PRESENT | PG_WRITABLE);
    

    /*
     * Map kernel into the higher half (0xC0000000+)
     */
    for (uint32_t vaddr = (uint32_t)&__kernel_start_v; vaddr < (uint32_t)&__kernel_end; vaddr += PAGE_SIZE) 
            boot_map_page(&boot_page_directory, (void *)vaddr, (void *)(vaddr - 0xC0000000), PG_PRESENT | PG_WRITABLE);
    
    /*
     * Load page directory and enable paging
     */
    asm volatile("mov %0, %%cr3"
                 :
                 : "r"(boot_page_directory.tables_physical)
                 : "memory");

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  /* PG bit */
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}
