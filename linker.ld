ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

STACK_SIZE = 0x200000;      /* 2 MiB  */
HEAP_SIZE  = 0x10000000;      /* 200 MiB  */

SECTIONS
{
    .multiboot 0x100000 :
    {
        *(.multiboot)
    }

    .text :
    {
        code = .; _code = .; __code = .;
        *(.text)
        . = ALIGN(4096);
    }

    .data :
    {
        data = .; _data = .; __data = .;
        *(.data)
        *(.rodata)
        . = ALIGN(4096);
    }

    .bss :
    {
        _bss = .;
        *(.bss)

        /* reserve 2MiB stack inside BSS at low VA */
        stack_start = .;
        . += STACK_SIZE;
        stack_end = .;

        . = ALIGN(4096);
    }

    /* -------- Heap Section (1GiB) at end -------- */
    .heap :
    {
        heap_start = .;
        . = . + HEAP_SIZE;
        heap_end = .;
        . = ALIGN(4096);
    }

    __kernel_end = .;
}