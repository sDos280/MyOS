.intel_syntax noprefix

// Declare constants for the multiboot header.
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    (ALIGN | MEMINFO)
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

// Multiboot header
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

// Lower half Bootstrap stack
.section .multiboot.bss, "aw", @nobits
lstack_bottom:
.skip 16384
lstack_top:

// Higher half Bootstrap stack
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384
stack_top:

// Paging structures
.section .bss, "aw", @nobits
.align 4096
boot_page_directory:
.skip 4096
boot_page_table1:
.skip 4096

// Kernel entry point
.section .multiboot.text, "ax"
.global _start
.type _start, @function
_start:
    // setup stack
    lea ebp, [lstack_top]
    lea esp, [lstack_top]
    
    push eax          // multiboot_magic
    push ebx          // multiboot_structure
    call boot_ipl

halt_loop:
    cli
infinite_halt:
    hlt
    jmp infinite_halt
