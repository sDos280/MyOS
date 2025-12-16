ENTRY (_start)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

STACK_SIZE = 0x200000;      /* 2 MiB  */
HEAP_SIZE  = 0x10000000;    /* 200 MiB  */

SECTIONS
{

    /*
     * The multiboot header and initial entry point must be in the identity-mapped
     * physical address space so the bootloader can find them and execute them
     * before paging is enabled.
     */
    __kernel_start = .;

    .multiboot.data :
    {
        KEEP(*(.multiboot.data))
    }

    /*
     * Start physical address (typical for bootloaders like GRUB).
     * This is where everything before 0xC0000000 virtual is loaded.
     */
    . = 0x00100000;

    /* Ensure the startup code is placed right after the header, physically. */
    .multiboot.text : AT(ADDR(.multiboot.text))
    {
        KEEP(*(.multiboot.text))
    }
    
    /*
     * Now we switch to the higher-half virtual address space for the rest
     * of the kernel sections. We define the virtual address relative to 
     * the physical load address using the 0xC0000000 offset.
     */
    . += 0xC0000000;

    /* Add a symbol that indicates the start address of the kernel in virtual memory. */

    .text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000)
    {
        *(.text)
    }
    .rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000)
    {
        *(.rodata)
    }
    .data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000)
    {
        *(.data)
    }
    .bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
    
    /* -------- Heap Section (1GiB) at end -------- */
    .heap ALIGN(4096) (NOLOAD) : 
    {
        heap_start = .;
        . = . + HEAP_SIZE;
        heap_end = .;
    }

    /* Add a symbol that indicates the end address of the kernel. */
    __kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame)
    }
}