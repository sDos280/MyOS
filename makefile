# Tools
CC = gcc
AS = nasm
LD = ld
QEMU = qemu-system-i386

# Directories
SOURCE_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Flags
CFLAGS = -m32 -nostdlib -fno-builtin -fno-exceptions -fno-leading-underscore -I$(INCLUDE_DIR)
ASFLAGS = -f elf32
LDFLAGS = -melf_i386

# Files
SOURCES_C = $(wildcard $(SOURCE_DIR)/*.c)
SOURCES_S = $(wildcard $(SOURCE_DIR)/*.asm)
OBJECTS_C = $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES_C))
OBJECTS_S = $(patsubst $(SOURCE_DIR)/%.asm, $(BUILD_DIR)/%.o, $(SOURCES_S))
OBJECTS = $(OBJECTS_S) $(OBJECTS_C)

KERNEL_ELF = $(BUILD_DIR)/mykernel.elf
KERNEL_BIN = $(BUILD_DIR)/mykernel.bin
ISO_IMAGE  = $(BUILD_DIR)/mykernel.iso
VIRTUAL_DISK = $(BUILD_DIR)/vrdisk.img

# Default target
all: $(KERNEL_ELF) $(KERNEL_BIN)

# Create build directory if not exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile C files
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble .asm files
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel ELF
$(KERNEL_ELF): linker.ld $(OBJECTS)
	$(LD) $(LDFLAGS) -T $< -o $@ $(OBJECTS)

# Make raw binary from ELF
$(KERNEL_BIN): $(KERNEL_ELF)
	objcopy -O binary $< $@

# Create the ISO file using GRUB
iso: $(KERNEL_ELF)
	mkdir -p iso/boot/grub
	cp $(KERNEL_ELF) iso/boot/mykernel.elf
	echo 'set timeout=0'                      > iso/boot/grub/grub.cfg
	echo 'set default=0'                     >> iso/boot/grub/grub.cfg
	echo ''                                  >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/mykernel.elf'    >> iso/boot/grub/grub.cfg
	echo '  boot'                            >> iso/boot/grub/grub.cfg
	echo '}'                                 >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$(ISO_IMAGE) iso
	rm -rf iso

# Run with QEMU (after ISO is created)
run: iso
	$(QEMU) -m 4G -cdrom $(ISO_IMAGE) -hda $(VIRTUAL_DISK)

# Run with QEMU in debug mode
# enter gdb in wsl
# gdb /mnt/c/Users/DorSh/Projects/MyOSv3/build/mykernel.elf
# target remote :1234
debug: CFLAGS += -g
debug: iso
	$(QEMU) -cdrom $(ISO_IMAGE) -s -S -hda $(VIRTUAL_DISK)

# Cleanup
clean:
	rm -rf $(BUILD_DIR)/*.o $(KERNEL_BIN) $(KERNEL_ELF) $(ISO_IMAGE) $(VIRTUAL_DISK)
	qemu-img create $(VIRTUAL_DISK) 1G 
