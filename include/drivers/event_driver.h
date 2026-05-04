#ifndef EVENT_DRIVER_H
#define EVENT_DRIVER_H

#include "keys.h"
#include "multitasking/lock.h"

#define EVENT_HANDLER_QUEUE_SIZE  200

typedef enum event_type_struct {
    KEY_PRESS,
} event_type_t;

typedef struct event_struct {
    event_type_t type;
    /* In case of key press, code (key) as defined in keys.h */
    uint32_t code;
} event_t;


typedef struct event_handler_struct {
    event_t events_queue[EVENT_HANDLER_QUEUE_SIZE]; /* queue of events captured by the os */
    int32_t head_events_queue;                   /* the head index of events_queue */
    int32_t tail_events_queue;                   /* the tail index of events_queue */
    lock_t lock;
} event_handler_t;

void event_init_event_handler(event_handler_t * eh);
uint8_t event_is_events_queue_empty(event_handler_t * eh);  /* 1 - empty, 0 - not empty */
uint8_t event_is_events_queue_full(event_handler_t * eh);   /* 1 - full, 0 - not full */
void event_put_event(event_handler_t * eh, event_type_t type, uint32_t code);    
uint8_t event_get_event(event_handler_t * eh, event_t *event_out); /* 1 - there is an pending event, else/error 0*/

#endif // EVENT_DRIVER_H