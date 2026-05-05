#include "drivers/event_driver.h"

ring_queue_status_t event_init_event_handler(event_handler_t * eh) {
    if (!eh) return RING_QUEUE_ERR_BAD_ARG;

    return ring_queue_init(&eh->events_queue, EVENT_HANDLER_QUEUE_SIZE, sizeof(event_t));
}

uint8_t event_is_events_queue_empty(event_handler_t * eh) {
    if (!eh) return RING_QUEUE_ERR_BAD_ARG;
    return ring_queue_is_empty(&eh->events_queue);
}

uint8_t event_is_events_queue_full(event_handler_t * eh) {
    if (!eh) return RING_QUEUE_ERR_BAD_ARG;
    return ring_queue_is_full(&eh->events_queue);
}

ring_queue_status_t event_push_event(event_handler_t * eh, event_type_t type, uint32_t code) {
    if (!eh) return RING_QUEUE_ERR_BAD_ARG;

    event_t e;
    e.type = type;
    e.code = code;

    return ring_queue_push(&eh->events_queue, &e);
}

ring_queue_status_t event_pop_event(event_handler_t * eh, event_t * event_out) {
    if (!eh || !event_out) return RING_QUEUE_ERR_BAD_ARG;

    return ring_queue_pop(&eh->events_queue, event_out);
}

void event_destroy_event_handler(event_handler_t * eh) { 
    ring_queue_destroy(&eh->events_queue);
}