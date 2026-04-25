#include "multitasking/lock.h"

void lock_init(lock_t * lock) {
    lock->state = LOCK_FREE;
}
void lock_acquire(lock_t *lock) {
    while (__sync_lock_test_and_set(&lock->state, LOCK_LOCKED)) {
        while (lock->state == LOCK_LOCKED) {
            __asm__ __volatile__("pause");
        }
    }
}

void lock_release(lock_t *lock) {
    __sync_lock_release(&lock->state);
}