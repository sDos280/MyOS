.intel_syntax noprefix

.global scheduler_context_switch_asm
.global scheduler_start_thread_asm

.extern sheduler_thread_exit

.section .text
// the scheduling function can be summerized to:
// schedule:
//     pusha
//     move to other thread stack
//     popa
//     ret
//
// when each new thread starts with a stack like this:
// ====== Stack High Adresses (32 bit) ====== 
// entry
// General registers
// ====== Stack Lower Adresses (32 bit) ======
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
    sti               // Enable interrupt (to enable interrupts in next thread)
    ret               // Move to the pushed instruction address

scheduler_start_first_thread_asm:
    mov edx, [4+esp]  // Thread stack address

    mov esp, edx      // Move to the next thread stack
    
    // When we get here, the thread's stack looks like this:
    // ====== Stack High Adresses (32 bit) ====== 
    // scheduler_thread_exit
    // entry
    // edi
    // esi
    // ebp
    // esp
    // ebx
    // edx
    // ecx
    // eax
    // ====== Stack Lower Adresses (32 bit) ====== 

    popa              // Pops   edi,esi,ebp,esp (not really),ebx,edx,ecx,eax
    pop edx           // Set edx as the entry

    call edx
    
    // We shouldn't be here !!!
    ud2
