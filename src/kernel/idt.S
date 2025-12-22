.intel_syntax noprefix

.global flush_idt
.extern idt_ptr

.section .text
flush_idt:
    lidt [idt_ptr]  // Load the IDT descriptor into the IDTR register
    ret
