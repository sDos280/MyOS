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
.align 16
lstack_bottom:
.skip 16384
lstack_top:

// Higher half Bootstrap stack
.section .bootstrap_stack, "aw", @nobits
.align 16
stack_bottom:
.skip 16384
stack_top:


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

    pop ebx
    pop eax

    lea ecx, jump_to_higher_memory
    jmp ecx

.section .text
jump_to_higher_memory:
    // Reload crc3 to force a TLB flush so the changes to take effect.
    mov ecx, cr3
    mov cr3, ecx

    // Set up the stack.
    lea ebp, [stack_top]
    lea esp, [stack_top]

    push eax
    push ebx
    
    call kernel_main

halt_loop:
    cli
infinite_halt:
    hlt
    jmp infinite_halt