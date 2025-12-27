#include "kernel/description_tables.h"
#include "kernel/print.h"
#include "io/pic.h"
#include "utils.h"

gdt_entry_t gdt_entries[GDT_ENTRIES];
descriptor_ptr_t gdt_ptr;
idt_gate_t idt_entries[256];
descriptor_ptr_t idt_ptr;
isr_handler interrupt_handlers[256];

void initiate_descriptor(gdt_entry_t *gdt_entry, uint32_t base, uint32_t limit, uint16_t flag)
{
    memset(gdt_entry, 0, sizeof(gdt_entry_t)); // Clear out the descriptor first
    gdt_entry->limit_low    = (limit & 0x0000FFFF);         // set limit bits 15:0
    gdt_entry->base_low     = (base  & 0x0000FFFF);         // set base bits 15:0
    gdt_entry->base_middle  = (base  >> 16) & 0x000000FF; // set base bits 23:16
    gdt_entry->access       =  flag        & 0x000000FF;         // set type, p, dpl, s fields
    gdt_entry->granularity  = ((limit >> 16) & 0x0000000F); // set limit bits 19:16
    gdt_entry->granularity |=  (flag  >>  8) & 0x000000F0; // set g, d/b, l and avl fields
    gdt_entry->base_high    = (base  >> 24) & 0x000000FF; // set base bits 31:24
}

void gdt_init()
{
    initiate_descriptor(&gdt_entries[0], 0, 0, 0);                // Null segment
    initiate_descriptor(&gdt_entries[1], 0, 0xFFFFFFFF, GDT_CODE_PL0); // Code segment
    initiate_descriptor(&gdt_entries[2], 0, 0xFFFFFFFF, GDT_DATA_PL0); // Data segment

    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    flush_gdt();
}

void initialize_gate(uint32_t idt_entry_number, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt_gate_t *irq_gate = &idt_entries[idt_entry_number];

    memset(irq_gate, 0, sizeof(idt_gate_t)); // Clear out the descriptor first

    irq_gate->base_low = base & 0xFFFF;
    irq_gate->sel      = sel;
    irq_gate->always0  = 0;
    irq_gate->flags    = flags;
    irq_gate->base_high= (base >> 16) & 0xFFFF;
}

void idt_init(){
    idt_ptr.limit = (sizeof(idt_gate_t) * 256) -1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(idt_gate_t)*256);
    memset(&interrupt_handlers, 0, sizeof(isr_handler)*256);

    // Remap the PIC
    pic_remap();
    
    initialize_gate( 0, (uint32_t)isr0 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 1, (uint32_t)isr1 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 2, (uint32_t)isr2 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 3, (uint32_t)isr3 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 4, (uint32_t)isr4 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 5, (uint32_t)isr5 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 6, (uint32_t)isr6 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 7, (uint32_t)isr7 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 8, (uint32_t)isr8 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate( 9, (uint32_t)isr9 , 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(10, (uint32_t)isr10, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(11, (uint32_t)isr11, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(12, (uint32_t)isr12, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(13, (uint32_t)isr13, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(14, (uint32_t)isr14, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(15, (uint32_t)isr15, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(16, (uint32_t)isr16, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(17, (uint32_t)isr17, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(18, (uint32_t)isr18, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(19, (uint32_t)isr19, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(20, (uint32_t)isr20, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(21, (uint32_t)isr21, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(22, (uint32_t)isr22, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(23, (uint32_t)isr23, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(24, (uint32_t)isr24, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(25, (uint32_t)isr25, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(26, (uint32_t)isr26, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(27, (uint32_t)isr27, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(28, (uint32_t)isr28, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(29, (uint32_t)isr29, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(30, (uint32_t)isr30, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(31, (uint32_t)isr31, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(32, (uint32_t)isr32, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(33, (uint32_t)isr33, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(34, (uint32_t)isr34, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(35, (uint32_t)isr35, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(36, (uint32_t)isr36, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(37, (uint32_t)isr37, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(38, (uint32_t)isr38, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(39, (uint32_t)isr39, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(40, (uint32_t)isr40, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(41, (uint32_t)isr41, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(42, (uint32_t)isr42, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(43, (uint32_t)isr43, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(44, (uint32_t)isr44, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(45, (uint32_t)isr45, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(46, (uint32_t)isr46, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(47, (uint32_t)isr47, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);
    initialize_gate(48, (uint32_t)isr48, 0x08, IDT_FLAGS_PRESENT | IDT_TYPE_INTERRUPT | IDT_DPL_KERNEL);

    flush_idt();
}


void isr_stub_handler(registers_t regs){
    static uint16_t isr_tick = 0;

    if (interrupt_handlers[regs.int_no]) {
        isr_handler handler = interrupt_handlers[regs.int_no];
        handler(&regs);
    } else {
        printf("No handler registered for this interrupt.\n");
        printf("Received interrupt: %x   Err code: %x   Tick: %d\n", regs.int_no, regs.err_code, isr_tick);
    }
    
    isr_tick++;
    
    pic_sendEOI(regs.int_no); // If the interrupt involved the PIC irq send EOI
}

void register_interrupt_handler(uint8_t isr_number, isr_handler handler){
    interrupt_handlers[isr_number] = handler;
}