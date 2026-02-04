#ifndef PMEM_H
#define PMEM_H

#include "types.h"

#define FRAME_SIZE 0x1000
#define PMM_BIT_FIELD_ARR_SIZE ((0xFFFFFFFF / FRAME_SIZE) / 32)  /* 32 bits in an uint32 */

void pmm_init();

void* pmm_alloc_frame_addr(void * paddr);  /* paddr - the address to try to allocate from the page, should be frame aligned */
void* pmm_alloc_frame();
void pmm_free_frame(void* paddr);

#endif // PMEM_H