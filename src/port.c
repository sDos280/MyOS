#include "port.h"

// Write a byte to the specified port
void outb(unsigned short port, unsigned char value) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Read a byte from the specified port
unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Write a word (2 bytes) to the specified port
void outw(unsigned short port, unsigned short value) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(value), "Nd"(port));
}

// Read a word (2 bytes) from the specified port
unsigned short inw(unsigned short port) {
    unsigned short ret;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void io_wait(){
    __asm__ __volatile__ ("outb %%al, $0x80" : : "a"(0));
}