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

// Bootstrap stack
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
    // edi = physical address of boot_page_table1
    mov edi, boot_page_table1 - 0xC0000000

    // esi = first physical address to map
    mov esi, 0

    // ecx = number of pages
    mov ecx, 1023

page_mapping_loop:
    cmp esi, __kernel_start
    jl skip_kernel_mapping
    cmp esi, __kernel_end - 0xC0000000
    jge skip_kernel_mapping

    // Map page (present | writable)
    mov edx, esi
    or  edx, 0x003
    mov [edi], edx

skip_kernel_mapping:
    add esi, 4096
    add edi, 4
    dec ecx
    jnz page_mapping_loop

after_loop_kernel_maping:
    // VGA text buffer
    mov dword ptr [boot_page_table1 - 0xC0000000 + 1023*4], 0x000B8003

    // Page directory mappings
    mov eax, boot_page_table1 - 0xC0000000 + 0x003
    mov [boot_page_directory - 0xC0000000 + 0], eax
    
    mov eax, boot_page_table1 - 0xC0000000 + 0x003
    mov [boot_page_directory - 0xC0000000 + 768*4], eax

    // Load CR3
    mov ecx, boot_page_directory - 0xC0000000
    mov cr3, ecx

    // Enable paging + write protect
    mov ecx, cr0
    or  ecx, 0x80010000
    mov cr0, ecx

    // Jump to higher half
    lea ecx, [higher_half_entry]
    jmp ecx

.section .text

higher_half_entry:
    // Remove identity mapping
    mov dword ptr [boot_page_directory + 0], 0

    // Flush TLB
    mov ecx, cr3
    mov cr3, ecx

    // Set stack
    mov esp, stack_top

    // Enter kernel
    call kernel_main

halt_loop:
    cli
infinite_halt:
    hlt
    jmp infinite_halt
