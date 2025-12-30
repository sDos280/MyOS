#ifndef LOCK_H
#define LOCK_H

typedef enum {
    LOCK_FREE,
    LOCK_LOCKED
} lock_state_e;

typedef struct lock_struct {
    volatile lock_state_e state;
} lock_t;

void lock_init(lock_t * lock);
void lock_acquire(lock_t * lock); /* acuqire the lock if free, else spinlock */
void lock_release(lock_t * lock); /* release the lock */

#endif // LOCK_H