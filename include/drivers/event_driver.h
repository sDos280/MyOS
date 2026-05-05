#ifndef EVENT_DRIVER_H
#define EVENT_DRIVER_H

#include "drivers/keys.h"
#include "multitasking/lock.h"
#include "utils/ring_queue.h"

#define EVENT_HANDLER_QUEUE_SIZE  200

typedef enum event_type_struct {
    KEY_PRESS,
    KEY_RELEASE,
} event_type_t;

typedef struct event_struct {
    event_type_t type;
    /* In case of key press, code (key) as defined in keys.h */
    uint32_t code;
} event_t;


typedef struct event_handler_struct {
    ring_queue_t events_queue;
} event_handler_t;

ring_queue_status_t event_init_event_handler(event_handler_t * eh);
uint8_t event_is_events_queue_empty(event_handler_t * eh);  /* 1 - empty, 0 - not empty */
uint8_t event_is_events_queue_full(event_handler_t * eh);   /* 1 - full, 0 - not full */
ring_queue_status_t event_push_event(event_handler_t * eh, event_type_t type, uint32_t code);    
ring_queue_status_t event_pop_event(event_handler_t * eh, event_t *event_out);
void event_destroy_event_handler(event_handler_t * eh);

#endif // EVENT_DRIVER_H