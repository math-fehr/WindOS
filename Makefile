ARMGNU = arm-none-eabi

BUILD = build/

SOURCE = src/

TARGET = img/kernel.img
TARGET_QEMU = img/kernel_qemu.img

IMGDIR = img/

LINKER = kernel.ld

LINKER_QEMU = kernel_qemu.ld

MAP = kernel.map

OBJECTS =  $(patsubst $(SOURCE)%.s,$(BUILD)%.o,$(wildcard $(SOURCE)*.s))

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

OBJECTS_C =  $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(call rwildcard, $(SOURCE), *.c))

LIBGCC = $(shell dirname `$(ARMGNU)-gcc -print-libgcc-file-name`)

CFLAGS = -O0 -Wall -Wextra -nostdlib -lgcc -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7-a -mtune=cortex-a7 -std=gnu99 -D RPI2
QEMU = ~/opt/qemu-fvm/arm-softmmu/fvm-arm

SD_NAPPY = /media/nappy/boot/

DEPDIR = .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell mkdir -p $(BUILD) >/dev/null)
$(shell mkdir -p $(BUILD)/libc >/dev/null)


all: $(TARGET)

qemu: $(TARGET_QEMU)

rpi: CFLAGS = -O2 -Wall -Wextra -nostdlib -lgcc -std=gnu99 -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s
rpi: all

mkfs:
	qemu-img create fs.img 100M
	mkfs -t fat fs.img

img: fs.img

fs.img: $(wildcard $(IMGDIR)*)
	mcopy -D o -i fs.img $? ::

run: $(TARGET_QEMU)
	$(QEMU) -kernel $(TARGET_QEMU) -m 256 -M raspi2 -monitor stdio -serial pty

runs: $(TARGET_QEMU)
	$(QEMU) -kernel $(TARGET_QEMU) -m 256 -M raspi2 -serial stdio

#minicom:
#    minicom -b 115200 -o -D /dev/pts/1

$(TARGET) : $(BUILD)output.elf
	$(ARMGNU)-objcopy $(BUILD)output.elf -O binary $(TARGET)

$(TARGET_QEMU) : $(BUILD)output_qemu.elf
	$(ARMGNU)-objcopy $(BUILD)output_qemu.elf -O binary $(TARGET_QEMU)

$(BUILD)output.elf : $(OBJECTS) $(OBJECTS_C) $(LINKER)
	$(ARMGNU)-ld --no-undefined -L$(LIBGCC) $(OBJECTS) $(OBJECTS_C) -Map $(MAP) -o $(BUILD)output.elf -T $(LINKER) -lgcc

$(BUILD)output_qemu.elf : $(OBJECTS) $(OBJECTS_C) $(LINKER)
	$(ARMGNU)-ld --no-undefined -L$(LIBGCC) $(OBJECTS) $(OBJECTS_C) -Map $(MAP) -o $(BUILD)output_qemu.elf -T $(LINKER_QEMU) -lgcc

$(BUILD)%.o: $(SOURCE)%.s
	$(ARMGNU)-as -I $(SOURCE) $< -o $@


-include $(BUILD)$(OBJECTS_C:.o=.d)

$(BUILD)%.o: $(SOURCE)%.c
	$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@
	$(ARMGNU)-gcc $(CFLAGS) -MM $< -o $(BUILD)$*.d
	@mv -f $(BUILD)$*.d $(BUILD)$*.d.tmp
	@sed -e 's|.*:|$(BUILD)$*.o:|' < $(BUILD)$*.d.tmp > $(BUILD)$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILD)$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILD)$*.d
	@rm -f $(BUILD)$*.d.tmp

copy_nappy: all
	cp $(IMGDIR)* $(SD_NAPPY)

clean:
	rm -r $(BUILD)*
