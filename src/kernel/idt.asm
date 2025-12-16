global flush_idt
extern idt_ptr

flush_idt:
    lidt [idt_ptr]  ; Load the IDT descriptor into the IDTR register
    ret

; -----------------------------------------------------------------------------
; SECTION (note) - Inform the linker that the stack does not need to be executable
; -----------------------------------------------------------------------------
section .note.GNU-stack