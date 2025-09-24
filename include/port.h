#ifndef PORT_H
#define PORT_H

#include "types.h"


void outb(uint16_t port, uint8_t value); // Write a byte to the specified port
uint8_t inb(uint16_t port); // Read a byte from the specified port
void outw(uint16_t port, uint16_t value); // Write a word (16 bits) to the specified port
uint16_t inw(uint16_t port); // Read a word (16 bits) from the specified port
void io_wait(); // Wait for an I/O operation to complete

#endif // PORT_H