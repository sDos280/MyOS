ENTRY (_start)

SECTIONS
{
    . = 0x00100000;
    /* The kernel will live at 3GB + 1MB in the virtual address space, */
    /* which will be mapped to 1MB in the physical address space. */
    /* Note that we page-align the sections. */

    __kernel_start = .;

    .multiboot.data : {
        KEEP(*(.multiboot.data))
    }

    .multiboot.text : {
        KEEP(*(.multiboot.text))
    }

	.multiboot.bss : {
        KEEP(*(.multiboot.bss))
    }

	.boot.text : {
        KEEP(*(.boot.text))
    }

	.boot.bss : {
        KEEP(*(.boot.bss))
    }

    . += 0xC0000000;

	__kernel_start_v = .;

    /* Add a symbol that indicates the start address of the kernel. */
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
		*(.bootstrap_stack*)
    }

	__kernel_end_no_heap = .;

    /* -------- Heap Section (200Mbi) at end -------- */
    .heap :
    {
        __heap_start = .;
        . += 0x10000000;
        __heap_end = .;
        . = ALIGN(4096);
    }

    /* Add a symbol that indicates the end address of the kernel. */
    __kernel_end = .;
}