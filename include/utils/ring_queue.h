// ring_queue.h

#ifndef RING_QUEUE_H
#define RING_QUEUE_H

#include <types.h>
#include <multitasking/lock.h>

/**
 * ring_queue_t - Generic fixed-size ring-buffer queue.
 *
 * The queue stores raw bytes internally, so it can hold any element type.
 * The user provides element_size and capacity during initialization.
 */
typedef struct ring_queue
{
    uint8_t  *buffer;
    uint32_t element_size;
    uint32_t capacity;

    uint32_t head;
    uint32_t tail;
    uint32_t count;

    lock_t lock;
} ring_queue_t;

/**
 * ring_queue_status_t - Status codes returned by queue operations.
 */
typedef enum ring_queue_status
{
    RING_QUEUE_OK = 0,
    RING_QUEUE_ERR_NULL,
    RING_QUEUE_ERR_ALLOC,
    RING_QUEUE_ERR_FULL,
    RING_QUEUE_ERR_EMPTY,
    RING_QUEUE_ERR_BAD_ARG
} ring_queue_status_t;

/**
 * ring_queue_init - Initialize a ring queue.
 *
 * @queue         Pointer to the queue object.
 * @capacity      Maximum number of elements the queue can hold.
 * @element_size  Size in bytes of each element.
 * @return        RING_QUEUE_OK on success, error code otherwise.
 */
ring_queue_status_t ring_queue_init(ring_queue_t *queue,
                                    uint32_t capacity,
                                    uint32_t element_size);

/**
 * ring_queue_destroy - Free the queue internal buffer.
 *
 * @queue  Pointer to the queue object.
 */
void ring_queue_destroy(ring_queue_t *queue);

/**
 * ring_queue_is_empty - Check whether the queue is empty.
 *
 * @queue   Pointer to the queue object.
 * @return  1 if empty, 0 otherwise.
 */
uint8_t ring_queue_is_empty(const ring_queue_t *queue);

/**
 * ring_queue_is_full - Check whether the queue is full.
 *
 * @queue   Pointer to the queue object.
 * @return  1 if full, 0 otherwise.
 */
uint8_t ring_queue_is_full(const ring_queue_t *queue);

/**
 * ring_queue_count - Get number of stored elements.
 *
 * @queue   Pointer to the queue object.
 * @return  Number of elements currently stored.
 */
uint32_t ring_queue_count(const ring_queue_t *queue);

/**
 * ring_queue_capacity - Get maximum queue capacity.
 *
 * @queue   Pointer to the queue object.
 * @return  Maximum number of elements the queue can hold.
 */
uint32_t ring_queue_capacity(const ring_queue_t *queue);

/**
 * ring_queue_push - Push one element into the queue.
 *
 * @queue    Pointer to the queue object.
 * @element  Pointer to the element to copy into the queue.
 * @return   RING_QUEUE_OK on success, error code otherwise.
 */
ring_queue_status_t ring_queue_push(ring_queue_t *queue,
                                    const void *element);

/**
 * ring_queue_pop - Pop one element from the queue.
 *
 * @queue        Pointer to the queue object.
 * @out_element  Out: pointer to writable memory where the popped element is copied.
 * @return       RING_QUEUE_OK on success, error code otherwise.
 */
ring_queue_status_t ring_queue_pop(ring_queue_t *queue,
                                   void *out_element);

/**
 * ring_queue_peek - Read the front element without removing it.
 *
 * @queue        Pointer to the queue object.
 * @out_element  Out: pointer to writable memory where the front element is copied.
 * @return       RING_QUEUE_OK on success, error code otherwise.
 */
ring_queue_status_t ring_queue_peek(const ring_queue_t *queue,
                                    void *out_element);

/**
 * ring_queue_clear - Clear the queue.
 *
 * This only resets the queue indices. It does not erase the internal buffer.
 *
 * @queue  Pointer to the queue object.
 */
void ring_queue_clear(ring_queue_t *queue);

#endif // RING_QUEUE_H