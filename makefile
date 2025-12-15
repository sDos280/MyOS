# =========================
# Tools
# =========================
CC    = gcc
AS    = nasm
LD    = ld
QEMU  = qemu-system-i386

# =========================
# Directories
# =========================
SOURCE_DIR  = src
INCLUDE_DIR = include
BUILD_DIR   = build
ISO_DIR     = iso

# =========================
# Files
# =========================
KERNEL_ELF   = $(BUILD_DIR)/mykernel.elf
KERNEL_BIN   = $(BUILD_DIR)/mykernel.bin
ISO_IMAGE    = $(BUILD_DIR)/mykernel.iso
VIRTUAL_DISK = $(BUILD_DIR)/vrdisk.img

# =========================
# Flags
# =========================
CFLAGS  = -m32 -nostdlib -fno-builtin -fno-exceptions -fno-leading-underscore -I$(INCLUDE_DIR)
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386

# =========================
# Sources (1-level deep)
# =========================
SOURCES_C := $(wildcard $(SOURCE_DIR)/*.c $(SOURCE_DIR)/*/*.c)
SOURCES_S := $(wildcard $(SOURCE_DIR)/*.asm $(SOURCE_DIR)/*/*.asm)

OBJECTS_C := $(patsubst $(SOURCE_DIR)/%.c,  $(BUILD_DIR)/%.o, $(SOURCES_C))
OBJECTS_S := $(patsubst $(SOURCE_DIR)/%.asm,$(BUILD_DIR)/%.o, $(SOURCES_S))
OBJECTS   := $(OBJECTS_S) $(OBJECTS_C)

# =========================
# Default target
# =========================
all: $(KERNEL_ELF) $(KERNEL_BIN)

# =========================
# Compile C files
# =========================
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# =========================
# Assemble ASM files
# =========================
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# =========================
# Link kernel ELF
# =========================
$(KERNEL_ELF): linker.ld $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -T $< -o $@ $(OBJECTS)

# =========================
# Raw binary
# =========================
$(KERNEL_BIN): $(KERNEL_ELF)
	objcopy -O binary $< $@

# =========================
# ISO
# =========================
iso: $(KERNEL_ELF)
	@mkdir -p $(ISO_DIR)/boot
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/mykernel.elf
	grub-mkrescue --output=$(ISO_IMAGE) $(ISO_DIR)

# =========================
# Run
# =========================
run: iso $(VIRTUAL_DISK)
	$(QEMU) -m 4G -cdrom $(ISO_IMAGE) -hda $(VIRTUAL_DISK)

# =========================
# Debug
# enter gdb in wsl
# gdb /mnt/c/Users/DorSh/Projects/MyOSv3/build/mykernel.elf
# target remote :1234
# =========================
debug: CFLAGS += -g
debug: iso $(VIRTUAL_DISK)
	$(QEMU) -m 4G -cdrom $(ISO_IMAGE) -s -S -hda $(VIRTUAL_DISK)

# =========================
# Virtual disk
# =========================
$(VIRTUAL_DISK):
	@mkdir -p $(BUILD_DIR)
	qemu-img create $@ 1G

# =========================
# Cleanup
# =========================
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)/boot/mykernel.elf
