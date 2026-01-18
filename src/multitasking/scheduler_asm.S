.intel_syntax noprefix

.global scheduler_context_switch_asm
.global scheduler_start_thread_asm

.extern sheduler_thread_exit

.section .text
scheduler_context_switch_asm:
    mov ecx, [4+esp]  // Pointer to current thread stack 
    mov edx, [8+esp]  // Next thread stack address

    pusha             // Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    test ecx, ecx     // Is current pointer NULL?
    jz .skip_save
    mov [ecx], esp    // Save the current thread stack address to the pointer passed by the caller
.skip_save:
    mov esp, edx      // Move to the next thread stack

    popa              // Pops   edi,esi,ebp,esp,ebx,edx,ecx,eax
    // Fix: sti may not me a great idea, since an interrupt may accure between sti and ret
    sti               // Enable interrupt (to enable interrupts in next thread) 
    ret               // Move to the pushed instruction address
