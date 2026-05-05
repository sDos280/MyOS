// ring_queue.c


#include "mm/kheap.h"
#include "utils/ring_queue.h"
#include "utils/utils.h"

ring_queue_status_t ring_queue_init(ring_queue_t *queue,
                                    uint32_t capacity,
                                    uint32_t element_size)
{
    if (!queue)
    {
        return RING_QUEUE_ERR_NULL;
    }

    if (capacity == 0 || element_size == 0)
    {
        return RING_QUEUE_ERR_BAD_ARG;
    }

    queue->buffer = (uint8_t *)kalloc(capacity * element_size);
    if (!queue->buffer)
    {
        return RING_QUEUE_ERR_ALLOC;
    }

    lock_init(&queue->lock);

    queue->capacity = capacity;
    queue->element_size = element_size;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    return RING_QUEUE_OK;
}

void ring_queue_destroy(ring_queue_t *queue)
{
    if (!queue)
    {
        return;
    }

    if (queue->buffer)
    {
        kfree(queue->buffer);
    }

    queue->buffer = 0;
    queue->capacity = 0;
    queue->element_size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

uint8_t ring_queue_is_empty(const ring_queue_t *queue)
{
    if (!queue)
    {
        return 1;
    }

    return queue->count == 0;
}

uint8_t ring_queue_is_full(const ring_queue_t *queue)
{
    if (!queue)
    {
        return 0;
    }

    return queue->count == queue->capacity;
}

uint32_t ring_queue_count(const ring_queue_t *queue)
{
    if (!queue)
    {
        return 0;
    }

    return queue->count;
}

uint32_t ring_queue_capacity(const ring_queue_t *queue)
{
    if (!queue)
    {
        return 0;
    }

    return queue->capacity;
}

ring_queue_status_t ring_queue_push(ring_queue_t *queue,
                                    const void *element)
{
    uint32_t offset;

    if (!queue || !queue->buffer || !element)
    {
        return RING_QUEUE_ERR_NULL;
    }

    lock_acquire(&queue->lock);

    if (queue->count == queue->capacity)
    {
        lock_release(&queue->lock);
        return RING_QUEUE_ERR_FULL;
    }

    offset = queue->tail * queue->element_size;

    memcpy(queue->buffer + offset, element, queue->element_size);

    queue->tail++;
    if (queue->tail == queue->capacity)
    {
        queue->tail = 0;
    }

    queue->count++;

    lock_release(&queue->lock);

    return RING_QUEUE_OK;
}

ring_queue_status_t ring_queue_pop(ring_queue_t *queue,
                                   void *out_element)
{
    uint32_t offset;

    if (!queue || !queue->buffer || !out_element)
    {
        return RING_QUEUE_ERR_NULL;
    }

    if (queue->count == 0)
    {
        return RING_QUEUE_ERR_EMPTY;
    }

    lock_acquire(&queue->lock);

    offset = queue->head * queue->element_size;

    memcpy(out_element, queue->buffer + offset, queue->element_size);

    queue->head++;
    if (queue->head == queue->capacity)
    {
        queue->head = 0;
    }

    queue->count--;

    lock_release(&queue->lock);

    return RING_QUEUE_OK;
}

ring_queue_status_t ring_queue_peek(const ring_queue_t *queue,
                                    void *out_element)
{
    uint32_t offset;

    if (!queue || !queue->buffer || !out_element)
    {
        return RING_QUEUE_ERR_NULL;
    }

    if (queue->count == 0)
    {
        return RING_QUEUE_ERR_EMPTY;
    }

    lock_acquire(&queue->lock);

    offset = queue->head * queue->element_size;

    memcpy(out_element, queue->buffer + offset, queue->element_size);

    lock_release(&queue->lock);

    return RING_QUEUE_OK;
}

void ring_queue_clear(ring_queue_t *queue)
{
    if (!queue)
    {
        return;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}