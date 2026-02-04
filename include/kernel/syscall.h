#ifndef SYSCALL_H
#define SYSCALL_H

#include "kernel/description_tables.h"
#include "types.h"

#define PPROT_READ     0x1
#define PPROT_WRITE    0x2
#define PPROT_EXEC     0x4
#define PPROT_USER     0x8
#define PPROT_KER      0x10

#define SYSC_MMAP     0x5a
#define SYSC_MUNMAP   0x5b

void syscall_init();
uint32_t syscall_handler(cpu_status_t * regs);
void * mmap(void *addr, size_t length, uint32_t flags);
void * mmap_irq_handler(void *addr, size_t length, uint32_t flags);

#endif // SYSCALL_H
