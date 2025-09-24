; filepath: c:\Users\DorSh\Projects\MyOSv2\src\loader.asm
%define MAGIC      0x1badb002
%define FLAGS      (1<<0 | 1<<1)
%define CHECKSUM   -(MAGIC + FLAGS)

section .multiboot
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text
extern kernelMain
global loader

loader:
    mov esp, stack_top
    push eax          ; multiboot_structure
    push ebx          ; multiboot_magic
    call kernelMain

_stop:
    cli  ; disable interrupts
    hlt
    jmp _stop

section .bss
stack_bottom:
    resb 2*1024*1024  ; Reserve 2 MiB
stack_top: