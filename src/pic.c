#include "pic.h"

void pic_sendEOI(uint8_t irq)
{
	if (irq >= 32 && irq <= 47) {
        // If the interrupt came from the slave PIC (IRQ8–15)
        if (irq >= 40) {
            outb(0xA0, PIC_EOI); // EOI to slave
        }
        outb(0x20, PIC_EOI); // Always send EOI to master
    }
}

void pic_remap()
{
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, 0x20);                 // ICW2: Master PIC vector offset to 0x20
	io_wait();
	outb(PIC2_DATA, 0x28);                 // ICW2: Slave PIC vector offset to 0x28
	io_wait();
	outb(PIC1_DATA, 1 << CASCADE_IRQ);        // ICW3: tell Master PIC that there is a slave PIC at IRQ2
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
	
	outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	// Unmask both PICs.
	outb(PIC1_DATA, 0);
	outb(PIC2_DATA, 0);
}