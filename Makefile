ARMGNU = arm-none-eabi

BUILD = build/

SOURCE = src/

RAMFS = ramfs_content/

TARGET = img/kernel.img
TARGET_QEMU = img/kernel_qemu.img

RAMIMG = img/ram.img
RAMIMG_QEMU = img/ram_qemu.img

IMGDIR = img/

LINKER = kernel.ld
LINKER_QEMU = kernel_qemu.ld

# recursive wildcard.
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

OBJECTS 	=  $(patsubst $(SOURCE)%.s,$(BUILD)%.o,$(wildcard $(SOURCE)*.s))
OBJECTS_C = $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(call rwildcard, $(SOURCE), *.c))
RAMFS_OBJ = $(call rwildcard, $(RAMFS), *)

LIBGCC = $(shell dirname `$(ARMGNU)-gcc -print-libgcc-file-name`)

CFLAGS = -O2 -Wall -Wextra -nostdlib -lgcc -std=gnu99 $(INCLUDE_C)

HARDWARE_FLAGS = -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=soft \
								 -mtune=cortex-a7
RPI_FLAG = -D RPI2

SFLAGS = $(INCLUDE_C)

QEMU = qemu-fvm/arm-softmmu/fvm-arm

SD_NAPPY = /media/nappy/boot/

DEPDIR = .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell mkdir -p $(BUILD) >/dev/null)

#all: builds the kernel image for the real hardware. RPI2 flag by default.
all: $(RAMIMG)

#qemu: builds the kernel image for qemu emulation.
qemu: $(RAMIMG_QEMU)

#rpi: sets the flag for the RPI1 build.
rpi: RPI_FLAG = -D RPI
rpi: all

#rpi: sets the flag for the RPI2 build.
rpi2: RPI_FLAG = -D RPI2
rpi2: all

$(BUILD)fs.img: $(RAMFS_OBJ)
	genext2fs -b 4096 -d $(RAMFS) $(BUILD)fs.img

run: $(RAMIMG_QEMU)
	$(QEMU) -kernel $(RAMIMG_QEMU) -m 256 -M raspi2 -monitor stdio -serial pty

runs: $(RAMIMG_QEMU)
	$(QEMU) -kernel $(RAMIMG_QEMU) -m 256 -M raspi2 -serial stdio


$(RAMIMG): $(TARGET) $(BUILD)fs.img
	qemu-img create $(RAMIMG) 20M
	dd if=$(TARGET) of=$(RAMIMG) bs=2048 conv=notrunc
	dd if=$(BUILD)fs.img of=$(RAMIMG) bs=2048 seek=8M oflag=seek_bytes

$(RAMIMG_QEMU): $(TARGET_QEMU) $(BUILD)fs.img
	qemu-img create $(RAMIMG_QEMU) 20M
	dd if=$(TARGET_QEMU) of=$(RAMIMG_QEMU) bs=2048 conv=notrunc
	dd if=$(BUILD)fs.img of=$(RAMIMG_QEMU) bs=2048 seek=8M oflag=seek_bytes

$(TARGET) : $(BUILD)output.elf
	$(ARMGNU)-objcopy $(BUILD)output.elf -O binary $(TARGET)

$(TARGET_QEMU) : $(BUILD)output_qemu.elf
	$(ARMGNU)-objcopy $(BUILD)output_qemu.elf -O binary $(TARGET_QEMU)

$(BUILD)output.elf : $(OBJECTS) $(OBJECTS_C) $(LINKER)
	$(ARMGNU)-ld --no-undefined -L$(LIBGCC) $(OBJECTS) $(OBJECTS_C) $(LIBC) \
							 -o $(BUILD)output.elf -T $(LINKER) -lgcc -lg

$(BUILD)output_qemu.elf : $(OBJECTS) $(OBJECTS_C) $(LINKER)
	$(ARMGNU)-ld --no-undefined -L$(LIBGCC) $(OBJECTS) $(OBJECTS_C) $(LIBC) \
							 -o $(BUILD)output_qemu.elf -lgcc -lg -T $(LINKER_QEMU)

$(BUILD)%.o: $(SOURCE)%.s
	$(ARMGNU)-as -I $(SOURCE) $< -o $@ $(SFLAGS)

-include $(BUILD)$(OBJECTS_C:.o=.d)

$(BUILD)%.o: $(SOURCE)%.c
	$(ARMGNU)-gcc $(CFLAGS) $(HARDWARE_FLAGS) $(RPI_FLAG) -c $< -o $@
	$(ARMGNU)-gcc $(CFLAGS) $(HARDWARE_FLAGS) $(RPI_FLAG) -MM $< -o $(BUILD)$*.d
	@mv -f $(BUILD)$*.d $(BUILD)$*.d.tmp
	@sed -e 's|.*:|$(BUILD)$*.o:|' < $(BUILD)$*.d.tmp > $(BUILD)$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILD)$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILD)$*.d
	@rm -f $(BUILD)$*.d.tmp

copy_nappy: all
	cp $(IMGDIR)* $(SD_NAPPY)

clean:
	@rm -fr $(BUILD)*
	@rm -f $(TARGET)
	@rm -f $(TARGET_QEMU)
	@rm -f $(RAMIMG)
	@rm -f $(RAMIMG_QEMU)
