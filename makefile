# =========================
# Tools
# =========================
CC    = i686-elf-gcc
AS    = i686-elf-as
LD    = i686-elf-ld
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
CFLAGS  = -m32 -fno-pic -ffreestanding -fno-stack-protector -fno-builtin -fno-exceptions -fno-leading-underscore -I$(INCLUDE_DIR)
ASFLAGS = -32
LDFLAGS = -m elf_i386 -nostdlib

# =========================
# Sources (1-level deep)
# =========================
SOURCES_C := $(shell find $(SOURCE_DIR) -name '*.c')
SOURCES_S := $(shell find $(SOURCE_DIR) -name '*.S' -o -name '*.asm')

OBJECTS_C := $(patsubst $(SOURCE_DIR)/%, $(BUILD_DIR)/%, $(SOURCES_C:.c=.o))
OBJECTS_S := $(patsubst $(SOURCE_DIR)/%, $(BUILD_DIR)/%, $(SOURCES_S:.S=.o))
OBJECTS_S := $(patsubst $(SOURCE_DIR)/%, $(BUILD_DIR)/%, $(OBJECTS_S:.asm=.o))
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
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.S
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
	i686-elf-objcopy -O binary $< $@

# =========================
# ISO
# =========================
iso: $(KERNEL_ELF)
	@mkdir -p $(ISO_DIR)/boot
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/mykernel.elf
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

# =========================
# Run
# =========================
run: iso $(VIRTUAL_DISK)
	$(QEMU) -m 4G -cdrom $(ISO_IMAGE) -hda $(VIRTUAL_DISK)

# =========================
# Debug
# enter gdb in wsl
# gdb /mnt/c/Users/.../Projects/MyOSv3/build/mykernel.elf
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
