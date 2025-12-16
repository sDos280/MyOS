; Declare constants for the multiboot header.
%define ALIGN    1<<0             ; align loaded modules on page boundaries
%define MEMINFO  1<<1             ; provide memory map
%define FLAGS    ALIGN | MEMINFO  ; this is the Multiboot 'flag' field
%define MAGIC    0x1BADB002       ; 'magic number' lets bootloader find the header
%define CHECKSUM -(MAGIC + FLAGS) ; checksum of above, to prove we are multiboot

; Extern symbols
extern kernel_main
extern __kernel_start
extern __kernel_end

; Declare a multiboot header that marks the program as a kernel.
section .multiboot.data alloc write
multiboot_header:
    align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; Allocate the initial stack.
section .bootstrap_stack write nobits
stack_bottom:
    resb 16384 ; 16 KiB
stack_top:

; Preallocate pages used for paging. Don't hard-code addresses and assume they
; are available, as the bootloader might have loaded its multiboot structures or
; modules there. This lets the bootloader know it must avoid the addresses.
section .bss
    align 4096
boot_page_directory:
    resb 4096
boot_page_table1:
    resb 4096
; Further page tables may be required if the kernel grows beyond 3 MiB.

; The kernel entry point.
section .multiboot.text exec
global _start
_start:
	; Physical address of boot_page_table1.
	; TODO: I recall seeing some assembly that used a macro to do the
	;       conversions to and from physical. Maybe this should be done in this
	;       code as well?
	mov edi, (boot_page_table1 - 0xC0000000)
	; First address to map is address 0.
	; TODO: Start at the first kernel page instead. Alternatively map the first
	;       1 MiB as it can be generally useful, and there's no need to
	;       specially map the VGA buffer.
	mov esi, 0
	; Map 1023 pages. The 1024th will be the VGA text buffer.
	mov ecx, 1023

loop_start:
	; Only map the kernel.
	cmp esi, __kernel_start
	jl skip_map
	cmp esi, (__kernel_end - 0xC0000000)
	jge loop_end

	; Map physical address as "present, writable". Note that this maps
	; .text and .rodata as writable. Mind security and map them as non-writable.
	mov edx, esi
	or edx, 0x003
	mov [edi], edx

skip_map:
	; Size of page is 4096 bytes.
	add esi, 4096
	; Size of entries in boot_page_table1 is 4 bytes.
	add edi, 4
	; Loop to the next entry if we haven't finished.
	loop loop_start

loop_end:
	; Map VGA video memory to 0xC03FF000 as "present, writable".
	mov dword [(boot_page_table1 - 0xC0000000) + 1023 * 4], (0x000B8000 | 0x003)

	; The page table is used at both page directory entry 0 (virtually from 0x0
	; to 0x3FFFFF) (thus identity mapping the kernel) and page directory entry
	; 768 (virtually from 0xC0000000 to 0xC03FFFFF) (thus mapping it in the
	; higher half). The kernel is identity mapped because enabling paging does
	; not change the next instruction, which continues to be physical. The CPU
	; would instead page fault if there was no identity mapping.

	; Map the page table to both virtual addresses 0x00000000 and 0xC0000000.
	mov dword [(boot_page_directory - 0xC0000000) + 0], (boot_page_table1 - 0xC0000000 + 0x003)
	mov dword [(boot_page_directory - 0xC0000000) + 768 * 4], (boot_page_table1 - 0xC0000000 + 0x003)

	; Set cr3 to the address of the boot_page_directory.
	mov ecx, (boot_page_directory - 0xC0000000)
	mov cr3, ecx

	; Enable paging and the write-protect bit.
	mov ecx, cr0
	or ecx, 0x80010000
	mov cr0, ecx

	; Jump to higher half with an absolute jump. 
	lea ecx, [higher_half_start]
	jmp ecx

section .text

higher_half_start:
	; At this point, paging is fully set up and enabled.

	; Unmap the identity mapping as it is now unnecessary. 
	mov dword [boot_page_directory + 0], 0

	; Reload crc3 to force a TLB flush so the changes to take effect.
	mov ecx, cr3
	mov cr3, ecx

	; Set up the stack.
	mov esp, stack_top

	; Enter the high-level kernel.
	call kernel_main

	; Infinite loop if the system has nothing more to do.
	cli
halt_loop:
	hlt
	jmp halt_loop

; -----------------------------------------------------------------------------
; SECTION (note) - Inform the linker that the stack does not need to be executable
; -----------------------------------------------------------------------------
section .note.GNU-stack
