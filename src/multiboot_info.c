#include "kernel/early_print.h"
#include "mm/pmm.h"
#include "multiboot_info.h"

static void print_u64_hex(uint64_t value)
{
    uint32_t high = (uint32_t)(value >> 32);
    uint32_t low  = (uint32_t)(value & 0xffffffff);

    if (high != 0)
    {
        early_printf("0x%x%08x", high, low);
    }
    else
    {
        early_printf("0x%x", low);
    }
}

static const char *mmap_type_to_string(uint32_t type)
{
    switch (type)
    {
        case MULTIBOOT_MEMORY_AVAILABLE:
            return "Available RAM";

        case MULTIBOOT_MEMORY_RESERVED:
            return "Reserved";

        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
            return "ACPI reclaimable";

        case MULTIBOOT_MEMORY_NVS:
            return "NVS";

        case MULTIBOOT_MEMORY_BADRAM:
            return "Bad RAM";

        default:
            return "Unknown";
    }
}

static const char *framebuffer_type_to_string(uint8_t type)
{
    switch (type)
    {
        case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
            return "Indexed color";

        case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
            return "RGB";

        case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
            return "EGA text";

        default:
            return "Unknown";
    }
}

void multiboot_info_print(uint32_t magic, const multiboot_info_t *mbi)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        early_printf("Invalid Multiboot magic: 0x%x\n", magic);
        return;
    }

    if (!mbi)
    {
        early_printf("Multiboot info pointer is NULL\n");
        return;
    }

    early_printf("\n");
    early_printf("============================================================\n");
    early_printf("                   MULTIBOOT v1 INFORMATION\n");
    early_printf("============================================================\n");

    early_printf("Magic: 0x%x\n", magic);
    early_printf("Info structure address: 0x%x\n", (uint32_t)mbi);
    early_printf("Flags: 0x%x\n", mbi->flags);

    early_printf("\n");
    early_printf("-------------------- Basic Memory Info ---------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_MEMORY)
    {
        early_printf("Lower memory: %u KB\n", mbi->mem_lower);
        early_printf("Upper memory: %u KB\n", mbi->mem_upper);
        early_printf("Total memory estimate: %u KB\n", mbi->mem_lower + mbi->mem_upper);
    }
    else
    {
        early_printf("Basic memory info: not provided\n");
    }

    early_printf("\n");
    early_printf("---------------------- Boot Device -------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_BOOTDEV)
    {
        uint32_t boot_device = mbi->boot_device;

        uint8_t drive = (boot_device >> 24) & 0xff;
        uint8_t part1 = (boot_device >> 16) & 0xff;
        uint8_t part2 = (boot_device >> 8) & 0xff;
        uint8_t part3 = boot_device & 0xff;

        early_printf("Boot device raw: 0x%x\n", boot_device);
        early_printf("Drive: 0x%x\n", drive);
        early_printf("Partition 1: 0x%x\n", part1);
        early_printf("Partition 2: 0x%x\n", part2);
        early_printf("Partition 3: 0x%x\n", part3);
    }
    else
    {
        early_printf("Boot device: not provided\n");
    }

    early_printf("\n");
    early_printf("-------------------- Command Line --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_CMDLINE)
    {
        early_printf("Command line address: 0x%x\n", mbi->cmdline);
        early_printf("Command line: %s\n", (const char *)mbi->cmdline);
    }
    else
    {
        early_printf("Command line: not provided\n");
    }

    early_printf("\n");
    early_printf("---------------------- Boot Modules ------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_MODS)
    {
        early_printf("Modules count: %u\n", mbi->mods_count);
        early_printf("Modules address: 0x%x\n", mbi->mods_addr);

        multiboot_module_t *mods = (multiboot_module_t *)mbi->mods_addr;

        for (uint32_t i = 0; i < mbi->mods_count; i++)
        {
            early_printf("\n");
            early_printf("Module #%u\n", i);
            early_printf("  Start: 0x%x\n", mods[i].mod_start);
            early_printf("  End:   0x%x\n", mods[i].mod_end);
            early_printf("  Size:  %u bytes\n", mods[i].mod_end - mods[i].mod_start);
            early_printf("  Cmdline address: 0x%x\n", mods[i].cmdline);

            if (mods[i].cmdline)
            {
                early_printf("  Cmdline: %s\n", (const char *)mods[i].cmdline);
            }
            else
            {
                early_printf("  Cmdline: none\n");
            }
        }
    }
    else
    {
        early_printf("Modules: not provided\n");
    }

    early_printf("\n");
    early_printf("---------------------- Symbol Info -------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_AOUT_SYMS)
    {
        early_printf("A.OUT symbol table provided\n");
        early_printf("Tab size: 0x%x\n", mbi->u.aout_sym.tabsize);
        early_printf("String size: 0x%x\n", mbi->u.aout_sym.strsize);
        early_printf("Address: 0x%x\n", mbi->u.aout_sym.addr);
        early_printf("Reserved: 0x%x\n", mbi->u.aout_sym.reserved);
    }
    else if (mbi->flags & MULTIBOOT_INFO_ELF_SHDR)
    {
        early_printf("ELF section header table provided\n");
        early_printf("Number of sections: %u\n", mbi->u.elf_sec.num);
        early_printf("Section header size: %u\n", mbi->u.elf_sec.size);
        early_printf("Section headers address: 0x%x\n", mbi->u.elf_sec.addr);
        early_printf("String table index: %u\n", mbi->u.elf_sec.shndx);
    }
    else
    {
        early_printf("Symbol info: not provided\n");
    }

    early_printf("\n");
    early_printf("---------------------- Memory Map --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_MEM_MAP)
    {
        early_printf("Memory map address: 0x%x\n", mbi->mmap_addr);
        early_printf("Memory map length: %u bytes\n", mbi->mmap_length);

        uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

        for (
            multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
            (uint32_t)mmap < mmap_end;
            mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size))
        )
        {
            early_printf("\n");
            early_printf("Memory region:\n");
            early_printf("  Entry size: %u\n", mmap->size);
            early_printf("  Base addr:  ");
            print_u64_hex(mmap->addr);
            early_printf("\n");

            early_printf("  Length:     ");
            print_u64_hex(mmap->len);
            early_printf(" bytes\n");

            early_printf("  End addr:   ");
            print_u64_hex(mmap->addr + mmap->len);
            early_printf("\n");
            early_printf("  Type:       %u - %s\n",
                   mmap->type,
                   mmap_type_to_string(mmap->type));
        }
    }
    else
    {
        early_printf("Memory map: not provided\n");
    }

    early_printf("\n");
    early_printf("---------------------- Drive Info --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_DRIVE_INFO)
    {
        early_printf("Drive info address: 0x%x\n", mbi->drives_addr);
        early_printf("Drive info length: %u bytes\n", mbi->drives_length);

        /*
         * Drive info is a variable-size BIOS-specific structure.
         * This function prints the raw location and length only.
         */
    }
    else
    {
        early_printf("Drive info: not provided\n");
    }

    early_printf("\n");
    early_printf("-------------------- Config Table --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_CONFIG_TABLE)
    {
        early_printf("Config table address: 0x%x\n", mbi->config_table);
    }
    else
    {
        early_printf("Config table: not provided\n");
    }

    early_printf("\n");
    early_printf("------------------- Bootloader Name ------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
    {
        early_printf("Bootloader name address: 0x%x\n", mbi->boot_loader_name);
        early_printf("Bootloader name: %s\n", (const char *)mbi->boot_loader_name);
    }
    else
    {
        early_printf("Bootloader name: not provided\n");
    }

    early_printf("\n");
    early_printf("----------------------- APM Table --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_APM_TABLE)
    {
        struct multiboot_apm_info *apm =
            (struct multiboot_apm_info *)mbi->apm_table;

        early_printf("APM table address: 0x%x\n", mbi->apm_table);

        if (apm)
        {
            early_printf("Version: 0x%x\n", apm->version);
            early_printf("Code segment: 0x%x\n", apm->cseg);
            early_printf("Offset: 0x%x\n", apm->offset);
            early_printf("16-bit code segment: 0x%x\n", apm->cseg_16);
            early_printf("Data segment: 0x%x\n", apm->dseg);
            early_printf("Flags: 0x%x\n", apm->flags);
            early_printf("Code segment length: %u\n", apm->cseg_len);
            early_printf("16-bit code segment length: %u\n", apm->cseg_16_len);
            early_printf("Data segment length: %u\n", apm->dseg_len);
        }
    }
    else
    {
        early_printf("APM table: not provided\n");
    }

    early_printf("\n");
    early_printf("----------------------- VBE Info ---------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_VBE_INFO)
    {
        early_printf("VBE control info address: 0x%x\n", mbi->vbe_control_info);
        early_printf("VBE mode info address: 0x%x\n", mbi->vbe_mode_info);
        early_printf("VBE mode: 0x%x\n", mbi->vbe_mode);
        early_printf("VBE interface segment: 0x%x\n", mbi->vbe_interface_seg);
        early_printf("VBE interface offset: 0x%x\n", mbi->vbe_interface_off);
        early_printf("VBE interface length: %u\n", mbi->vbe_interface_len);
    }
    else
    {
        early_printf("VBE info: not provided\n");
    }

    early_printf("\n");
    early_printf("-------------------- Framebuffer Info ----------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
    {
        early_printf("Framebuffer address: ");
        print_u64_hex(mbi->framebuffer_addr);
        early_printf("\n");
        early_printf("Framebuffer pitch: %u bytes\n", mbi->framebuffer_pitch);
        early_printf("Framebuffer width: %u\n", mbi->framebuffer_width);
        early_printf("Framebuffer height: %u\n", mbi->framebuffer_height);
        early_printf("Framebuffer bpp: %u\n", mbi->framebuffer_bpp);
        early_printf("Framebuffer type: %u - %s\n",
               mbi->framebuffer_type,
               framebuffer_type_to_string(mbi->framebuffer_type));

        if (mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED)
        {
            early_printf("Palette address: 0x%x\n", mbi->framebuffer_palette_addr);
            early_printf("Palette colors: %u\n", mbi->framebuffer_palette_num_colors);
        }
        else if (mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB)
        {
            early_printf("Red field position: %u\n", mbi->framebuffer_red_field_position);
            early_printf("Red mask size: %u\n", mbi->framebuffer_red_mask_size);

            early_printf("Green field position: %u\n", mbi->framebuffer_green_field_position);
            early_printf("Green mask size: %u\n", mbi->framebuffer_green_mask_size);

            early_printf("Blue field position: %u\n", mbi->framebuffer_blue_field_position);
            early_printf("Blue mask size: %u\n", mbi->framebuffer_blue_mask_size);
        }
    }
    else
    {
        early_printf("Framebuffer info: not provided\n");
    }

    early_printf("\n");
    early_printf("============================================================\n");
    early_printf("                 END OF MULTIBOOT INFORMATION\n");
    early_printf("============================================================\n");
    early_printf("\n");
}

void multiboot_info_invalidate_unavailable_memory(multiboot_info_t *mbi)
{
    if (!mbi)
        return;

    if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP))
    {
        early_printf("Memory map: not provided. Unable to check what memory is vailable");
        return;
    }

    early_printf("Memory map address: 0x%x\n", mbi->mmap_addr);
    early_printf("Memory map length: %u bytes\n", mbi->mmap_length);

    uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

    for (
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
        (uint32_t)mmap < mmap_end;
        mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size)))
    {
        /* if memory is not RAM available, then invalidate that memory region */
        /* note that all memory information here is physical memory spesified */
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) continue;
        
        #define FRAME_SIZE          0x1000 
        #define FRAME_MASK(addr)    (addr & 0xFFFFF000) 

        /* check if memory address exceed 4Gb address */
        /* note that uint64 bit here is gcc magic (we use 62 bit system)*/
        if (mmap->addr >= 0x100000000ULL) continue;

        for (uint64_t p = FRAME_MASK(mmap->addr); 
             p <= FRAME_MASK(mmap->size + mmap->addr); 
             p += FRAME_SIZE) 
        {
            pmm_alloc_frame_addr((void *)(uint32_t)p);
        }
    }
}