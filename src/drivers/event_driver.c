#include "drivers/event_driver.h"

void event_init_event_handler(event_handler_t * eh) {
    if (!eh) return;
    memset(eh->events_queue, 0, sizeof(event_t)*EVENT_HANDLER_QUEUE_SIZE);
    eh->head_events_queue = -1;
    eh->tail_events_queue = -1;
    lock_init(&eh->lock);
}

uint8_t event_is_events_queue_empty(event_handler_t * eh) {
    if (!eh) return 0;
    return eh->head_events_queue == -1;
}

uint8_t event_is_events_queue_full(event_handler_t * eh) {
    if (!eh) return 0;
    return (eh->tail_events_queue + 1) % EVENT_HANDLER_QUEUE_SIZE == eh->head_events_queue;
}

void event_put_event(event_handler_t * eh, event_type_t type, uint32_t code) {
    if (!eh) return;

    /* FIX: that gives deadlock */
    //if (event_is_events_queue_full(eh))
    //    event_get_event(eh);  // The quere is full, so remove one element to clear spot
    
    lock_acquire(&eh->lock);

    if (event_is_events_queue_empty(eh)) {
        eh->head_events_queue = 0; // Initialize front for the first element
    }

    event_t e;
    e.type = type;
    e.code = code;

    eh->tail_events_queue = (eh->tail_events_queue + 1) % EVENT_HANDLER_QUEUE_SIZE;
    eh->events_queue[eh->tail_events_queue] = e;

    lock_release(&eh->lock);
}

uint8_t event_get_event(event_handler_t * eh, event_t * event_out) {
    if (!eh || !event_out) return 0;

    lock_acquire(&eh->lock);

    if (event_is_events_queue_empty(eh)) {
        lock_release(&eh->lock);
        return 0;
    }

    event_t e = eh->events_queue[eh->head_events_queue];

    if (eh->head_events_queue == eh->tail_events_queue) { 
        // If queue becomes empty after getting this element, reset both pointers to -1
        eh->head_events_queue = -1;
        eh->tail_events_queue = -1;
    } else {
        // Use modulo to wrap the head index around
        eh->head_events_queue = (eh->head_events_queue + 1) % EVENT_HANDLER_QUEUE_SIZE;
    }

    *event_out = e;
    lock_release(&eh->lock);

    return 1;
}