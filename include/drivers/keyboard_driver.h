#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include "kernel/description_tables.h"
#include "drivers/keys.h"

#define KEY_QUEUE_SIZE 50

typedef struct key_queue_struct {
    char queue[KEY_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
} key_queue_t;

char key_to_ascii(uint8_t key);
void keyboard_driver_init(); // initialize keyboard driver
uint32_t keyboard_handler(cpu_status_t* regs);  // the keyboard interrupt handler
uint8_t is_key_pressed(uint8_t key);
char get_asynchronized_char(); // get the next get asynchronized char (0 if there is no new char)

#endif // KEYBOARD_DRIVER_H