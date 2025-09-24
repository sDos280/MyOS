global flush_gdt
extern gdt_ptr

flush_gdt:
    lgdt [gdt_ptr]              ; Load the new GDT

    ; Far jump to reload CS with selector 0x08
    jmp 0x08:.reload_cs

.reload_cs:
    mov ax, 0x10                 ; Kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret                          ; Return to C caller