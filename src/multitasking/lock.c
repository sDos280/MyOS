#include "multitasking/lock.h"

void lock_init(lock_t * lock) {
    lock->state = LOCK_FREE;
}
void lock_acquire(lock_t * lock) {
    /* Fix: a contact switch may happen in the check, should not happen */
    while (lock->state != LOCK_FREE)
        __asm__ __volatile__("pause"); /* this thread can't free the lock, so pause is ok. 
                                        The scheduler when moving on should free this*/
    
    lock->state= LOCK_LOCKED;
}

void lock_release(lock_t * lock) {
    lock->state = LOCK_FREE;
}