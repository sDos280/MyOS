.intel_syntax noprefix

.global switch_to_task_asm


.section .text
switch_to_task_asm:
    mov ecx, [8+esp]  // Pointer to current thread stack 
    mov edx, [4+esp]  // Next thread stack address

    pusha             // Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    mov [ecx], esp    // Save the current thread stack address to the pointer passed by the caller
    mov esp, edx      // Move to the next thread stack

    popa              // Pops   edi,esi,ebp,esp,ebx,edx,ecx,eax
    ret               // Move to the pushed instruction address