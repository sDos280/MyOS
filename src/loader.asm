; filepath: c:\Users\DorSh\Projects\MyOSv2\src\loader.asm
%define MAGIC      0x1badb002
%define FLAGS      (1<<0 | 1<<1)
%define CHECKSUM   -(MAGIC + FLAGS)

extern stack_end
extern kernelMain
global loader

section .multiboot
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text
global loader

loader:
    mov esp, stack_end      ; set valid stack pointer
    push eax                ; multiboot_magic
    push ebx                ; multiboot_structure
    call kernelMain

_stop:
    cli  ; disable interrupts
    hlt
    jmp _stop