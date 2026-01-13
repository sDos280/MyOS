.intel_syntax noprefix

.global scheduler_context_switch_asm
.global scheduler_start_first_thread_asm

.section .text
scheduler_context_switch_asm:
    mov ecx, [4+esp]  // Pointer to current thread stack 
    mov edx, [8+esp]  // Next thread stack address

    pusha             // Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    mov [ecx], esp    // Save the current thread stack address to the pointer passed by the caller
    mov esp, edx      // Move to the next thread stack

    popa              // Pops   edi,esi,ebp,esp,ebx,edx,ecx,eax
    ret               // Move to the pushed instruction address

scheduler_start_first_thread_asm:
    mov edx, [4+esp]  // Thread stack address

    mov esp, edx      // Move to the next thread stack
    
    popa              // Pops   edi,esi,ebp,esp,ebx,edx,ecx,eax
    ret               // Move to the pushed instruction address
