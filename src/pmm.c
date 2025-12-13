#include "pmm.h"
#include "utils.h"

#define FRAME_ALIGN(addr) (addr & ~0xFFF)
#define FRAME_INDEX(addr) (addr >> 12)
#define FRAME_BIT_FIELD_INDEX(addr) (FRAME_INDEX(addr) / 32)
#define FRAME_BIT_FIELD_INNER_INDEX(addr) (FRAME_INDEX(addr) % 32)

static uint32_t bit_field[PMM_BIT_FIELD_ARR_SIZE];

void pmm_init() {
    memset(bit_field, 0, sizeof(uint32_t) * PMM_BIT_FIELD_ARR_SIZE);
}

void* pmm_alloc_frame_addr(void * paddr) {
    paddr = (void *)FRAME_ALIGN((uint32_t)paddr); /* make sure addr is frame aligned */
    uint32_t bit_field_index = FRAME_BIT_FIELD_INDEX((uint32_t)paddr);
    uint32_t bit_field_inner_index = FRAME_BIT_FIELD_INNER_INDEX((uint32_t)paddr);

    if (bit_field[bit_field_index] & (1 << bit_field_inner_index)) /* is frame is already used */
        return NULL;
    
    /* mark the frame as used */
    bit_field[bit_field_index] = bit_field[bit_field_index] | (1 << bit_field_inner_index);

    return paddr;
}

void* pmm_alloc_frames_addr(void * paddr, size_t count) {
    uint32_t bit_field_index;
    uint32_t bit_field_inner_index;
    uint32_t caddr;
    
    paddr = (void *)FRAME_ALIGN((uint32_t)paddr); /* make sure addr is frame aligned */

    for (size_t i = 0; i < count; i++) { /* make sure all addr and address are not used */
        caddr = paddr + i * FRAME_SIZE;

        bit_field_index = FRAME_BIT_FIELD_INDEX((uint32_t)caddr);
        bit_field_inner_index = FRAME_BIT_FIELD_INNER_INDEX((uint32_t)caddr);

        if (bit_field[bit_field_index] & (1 << bit_field_inner_index)) /* is frame is already used */
            return NULL;
    }

    /* all frames are allowed to be allocated */

    for (size_t i = 0; i < count; i++) { /* allocate all of them */
        caddr = paddr + i * FRAME_SIZE;
        pmm_alloc_frame_addr(caddr);
    }

    return paddr;
}

static void * get_last_free_frame() {
    for (size_t b = 0; b < PMM_BIT_FIELD_ARR_SIZE; b++)
        if (bit_field[b] != 0xFFFFFFFF)
            for (size_t i = 0; i < 32; i++)
                if (!(bit_field[b] & (1 << i))) /* frame is free */
                    return (void *)(FRAME_SIZE * (32 * b + i));
    
    return NULL;
}

void* pmm_alloc_frame() {
    void * addr = get_last_free_frame();
    addr = pmm_alloc_frame_addr(addr);

    return addr;
}

void pmm_free_frame(void* paddr) {
    paddr = (void *)FRAME_ALIGN((uint32_t)paddr); /* make sure addr is frame aligned */

    uint32_t bit_field_index = FRAME_BIT_FIELD_INDEX((uint32_t)paddr);
    uint32_t bit_field_inner_index = FRAME_BIT_FIELD_INNER_INDEX((uint32_t)paddr);

    bit_field[bit_field_index] = bit_field[bit_field_index] & ~(1 << bit_field_inner_index);  /* set frame to be unused */
}

void pmm_free_frames(void* paddr, size_t count) {
    uint32_t caddr;

    paddr = (void *)FRAME_ALIGN((uint32_t)paddr); /* make sure addr is frame aligned */

    for (size_t i = 0; i < count; i++) { /* allocate all of them */
        caddr = (uint32_t)(paddr + i * FRAME_SIZE);
        pmm_free_frame((void *)caddr);
    }
}