#include "kernel/print.h"
#include "mm/pmm.h"
#include "multiboot_info.h"

static void print_u64_hex(uint64_t value)
{
    uint32_t high = (uint32_t)(value >> 32);
    uint32_t low  = (uint32_t)(value & 0xffffffff);

    if (high != 0)
    {
        printf("0x%x%08x", high, low);
    }
    else
    {
        printf("0x%x", low);
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
        printf("Invalid Multiboot magic: 0x%x\n", magic);
        return;
    }

    if (!mbi)
    {
        printf("Multiboot info pointer is NULL\n");
        return;
    }

    printf("\n");
    printf("============================================================\n");
    printf("                   MULTIBOOT v1 INFORMATION\n");
    printf("============================================================\n");

    printf("Magic: 0x%x\n", magic);
    printf("Info structure address: 0x%x\n", (uint32_t)mbi);
    printf("Flags: 0x%x\n", mbi->flags);

    printf("\n");
    printf("-------------------- Basic Memory Info ---------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_MEMORY)
    {
        printf("Lower memory: %u KB\n", mbi->mem_lower);
        printf("Upper memory: %u KB\n", mbi->mem_upper);
        printf("Total memory estimate: %u KB\n", mbi->mem_lower + mbi->mem_upper);
    }
    else
    {
        printf("Basic memory info: not provided\n");
    }

    printf("\n");
    printf("---------------------- Boot Device -------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_BOOTDEV)
    {
        uint32_t boot_device = mbi->boot_device;

        uint8_t drive = (boot_device >> 24) & 0xff;
        uint8_t part1 = (boot_device >> 16) & 0xff;
        uint8_t part2 = (boot_device >> 8) & 0xff;
        uint8_t part3 = boot_device & 0xff;

        printf("Boot device raw: 0x%x\n", boot_device);
        printf("Drive: 0x%x\n", drive);
        printf("Partition 1: 0x%x\n", part1);
        printf("Partition 2: 0x%x\n", part2);
        printf("Partition 3: 0x%x\n", part3);
    }
    else
    {
        printf("Boot device: not provided\n");
    }

    printf("\n");
    printf("-------------------- Command Line --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_CMDLINE)
    {
        printf("Command line address: 0x%x\n", mbi->cmdline);
        printf("Command line: %s\n", (const char *)mbi->cmdline);
    }
    else
    {
        printf("Command line: not provided\n");
    }

    printf("\n");
    printf("---------------------- Boot Modules ------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_MODS)
    {
        printf("Modules count: %u\n", mbi->mods_count);
        printf("Modules address: 0x%x\n", mbi->mods_addr);

        multiboot_module_t *mods = (multiboot_module_t *)mbi->mods_addr;

        for (uint32_t i = 0; i < mbi->mods_count; i++)
        {
            printf("\n");
            printf("Module #%u\n", i);
            printf("  Start: 0x%x\n", mods[i].mod_start);
            printf("  End:   0x%x\n", mods[i].mod_end);
            printf("  Size:  %u bytes\n", mods[i].mod_end - mods[i].mod_start);
            printf("  Cmdline address: 0x%x\n", mods[i].cmdline);

            if (mods[i].cmdline)
            {
                printf("  Cmdline: %s\n", (const char *)mods[i].cmdline);
            }
            else
            {
                printf("  Cmdline: none\n");
            }
        }
    }
    else
    {
        printf("Modules: not provided\n");
    }

    printf("\n");
    printf("---------------------- Symbol Info -------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_AOUT_SYMS)
    {
        printf("A.OUT symbol table provided\n");
        printf("Tab size: 0x%x\n", mbi->u.aout_sym.tabsize);
        printf("String size: 0x%x\n", mbi->u.aout_sym.strsize);
        printf("Address: 0x%x\n", mbi->u.aout_sym.addr);
        printf("Reserved: 0x%x\n", mbi->u.aout_sym.reserved);
    }
    else if (mbi->flags & MULTIBOOT_INFO_ELF_SHDR)
    {
        printf("ELF section header table provided\n");
        printf("Number of sections: %u\n", mbi->u.elf_sec.num);
        printf("Section header size: %u\n", mbi->u.elf_sec.size);
        printf("Section headers address: 0x%x\n", mbi->u.elf_sec.addr);
        printf("String table index: %u\n", mbi->u.elf_sec.shndx);
    }
    else
    {
        printf("Symbol info: not provided\n");
    }

    printf("\n");
    printf("---------------------- Memory Map --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_MEM_MAP)
    {
        printf("Memory map address: 0x%x\n", mbi->mmap_addr);
        printf("Memory map length: %u bytes\n", mbi->mmap_length);

        uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

        for (
            multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
            (uint32_t)mmap < mmap_end;
            mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size))
        )
        {
            printf("\n");
            printf("Memory region:\n");
            printf("  Entry size: %u\n", mmap->size);
            printf("  Base addr:  ");
            print_u64_hex(mmap->addr);
            printf("\n");

            printf("  Length:     ");
            print_u64_hex(mmap->len);
            printf(" bytes\n");

            printf("  End addr:   ");
            print_u64_hex(mmap->addr + mmap->len);
            printf("\n");
            printf("  Type:       %u - %s\n",
                   mmap->type,
                   mmap_type_to_string(mmap->type));
        }
    }
    else
    {
        printf("Memory map: not provided\n");
    }

    printf("\n");
    printf("---------------------- Drive Info --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_DRIVE_INFO)
    {
        printf("Drive info address: 0x%x\n", mbi->drives_addr);
        printf("Drive info length: %u bytes\n", mbi->drives_length);

        /*
         * Drive info is a variable-size BIOS-specific structure.
         * This function prints the raw location and length only.
         */
    }
    else
    {
        printf("Drive info: not provided\n");
    }

    printf("\n");
    printf("-------------------- Config Table --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_CONFIG_TABLE)
    {
        printf("Config table address: 0x%x\n", mbi->config_table);
    }
    else
    {
        printf("Config table: not provided\n");
    }

    printf("\n");
    printf("------------------- Bootloader Name ------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
    {
        printf("Bootloader name address: 0x%x\n", mbi->boot_loader_name);
        printf("Bootloader name: %s\n", (const char *)mbi->boot_loader_name);
    }
    else
    {
        printf("Bootloader name: not provided\n");
    }

    printf("\n");
    printf("----------------------- APM Table --------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_APM_TABLE)
    {
        struct multiboot_apm_info *apm =
            (struct multiboot_apm_info *)mbi->apm_table;

        printf("APM table address: 0x%x\n", mbi->apm_table);

        if (apm)
        {
            printf("Version: 0x%x\n", apm->version);
            printf("Code segment: 0x%x\n", apm->cseg);
            printf("Offset: 0x%x\n", apm->offset);
            printf("16-bit code segment: 0x%x\n", apm->cseg_16);
            printf("Data segment: 0x%x\n", apm->dseg);
            printf("Flags: 0x%x\n", apm->flags);
            printf("Code segment length: %u\n", apm->cseg_len);
            printf("16-bit code segment length: %u\n", apm->cseg_16_len);
            printf("Data segment length: %u\n", apm->dseg_len);
        }
    }
    else
    {
        printf("APM table: not provided\n");
    }

    printf("\n");
    printf("----------------------- VBE Info ---------------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_VBE_INFO)
    {
        printf("VBE control info address: 0x%x\n", mbi->vbe_control_info);
        printf("VBE mode info address: 0x%x\n", mbi->vbe_mode_info);
        printf("VBE mode: 0x%x\n", mbi->vbe_mode);
        printf("VBE interface segment: 0x%x\n", mbi->vbe_interface_seg);
        printf("VBE interface offset: 0x%x\n", mbi->vbe_interface_off);
        printf("VBE interface length: %u\n", mbi->vbe_interface_len);
    }
    else
    {
        printf("VBE info: not provided\n");
    }

    printf("\n");
    printf("-------------------- Framebuffer Info ----------------------\n");

    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
    {
        printf("Framebuffer address: ");
        print_u64_hex(mbi->framebuffer_addr);
        printf("\n");
        printf("Framebuffer pitch: %u bytes\n", mbi->framebuffer_pitch);
        printf("Framebuffer width: %u\n", mbi->framebuffer_width);
        printf("Framebuffer height: %u\n", mbi->framebuffer_height);
        printf("Framebuffer bpp: %u\n", mbi->framebuffer_bpp);
        printf("Framebuffer type: %u - %s\n",
               mbi->framebuffer_type,
               framebuffer_type_to_string(mbi->framebuffer_type));

        if (mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED)
        {
            printf("Palette address: 0x%x\n", mbi->framebuffer_palette_addr);
            printf("Palette colors: %u\n", mbi->framebuffer_palette_num_colors);
        }
        else if (mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB)
        {
            printf("Red field position: %u\n", mbi->framebuffer_red_field_position);
            printf("Red mask size: %u\n", mbi->framebuffer_red_mask_size);

            printf("Green field position: %u\n", mbi->framebuffer_green_field_position);
            printf("Green mask size: %u\n", mbi->framebuffer_green_mask_size);

            printf("Blue field position: %u\n", mbi->framebuffer_blue_field_position);
            printf("Blue mask size: %u\n", mbi->framebuffer_blue_mask_size);
        }
    }
    else
    {
        printf("Framebuffer info: not provided\n");
    }

    printf("\n");
    printf("============================================================\n");
    printf("                 END OF MULTIBOOT INFORMATION\n");
    printf("============================================================\n");
    printf("\n");
}

void multiboot_info_invalidate_unavailable_memory(multiboot_info_t *mbi)
{
    if (!mbi)
        return;

    if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP))
    {
        printf("Memory map: not provided. Unable to check what memory is vailable");
        return;
    }

    printf("Memory map address: 0x%x\n", mbi->mmap_addr);
    printf("Memory map length: %u bytes\n", mbi->mmap_length);

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
            pmm_alloc_frame_addr(p);
        }
    }
}