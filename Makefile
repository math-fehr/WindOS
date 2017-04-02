ARMGNU = arm-none-eabi

BUILD = build/

SOURCE = src/

TARGET = img/kernel.img

IMGDIR = img/

LINKER = kernel.ld

MAP = kernel.map

OBJECTS =  $(patsubst $(SOURCE)%.s,$(BUILD)%.o,$(wildcard $(SOURCE)*.s))

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

OBJECTS_C =  $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(call rwildcard, $(SOURCE), *.c))

LIBGCC = $(shell dirname `$(ARMGNU)-gcc -print-libgcc-file-name`)

CFLAGS = -O2 -Wall -Wextra -nostdlib -lgcc -mcpu=arm1176jzf-s -fpic -std=gnu99

QEMU = ~/opt/src/qemu-fvm/arm-softmmu/fvm-arm

DEPDIR = .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell mkdir -p $(BUILD) >/dev/null)
$(shell mkdir -p $(BUILD)/libc >/dev/null)


all: $(TARGET)

mkfs:
	qemu-img create fs.img 100M
	mkfs -t fat fs.img

img: fs.img

fs.img: $(wildcard $(IMGDIR)*)
	mcopy -D o -i fs.img $? ::

run1: $(TARGET)
	qemu-system-arm -kernel img/kernel.img -cpu arm1176 -m 256 -M versatilepb -no-reboot  -serial stdio -append "root=/dev/sda2 panic=1" -hda fs.img

runs1: $(TARGET)
	qemu-system-arm -kernel img/kernel.img -cpu arm1176 -m 256 -M versatilepb -no-reboot  -serial stdio -append "root=/dev/sda2 panic=1" -hda fs.img

run: $(TARGET)
	$(QEMU) -kernel $(TARGET) -m 256 -M raspi2 -monitor stdio -serial pty

runs: $(TARGET)
	$(QEMU) -kernel $(TARGET) -m 256 -M raspi2 -serial stdio

$(TARGET) : $(BUILD)output.elf
	$(ARMGNU)-objcopy $(BUILD)output.elf -O binary $(TARGET)

$(BUILD)output.elf : $(OBJECTS) $(OBJECTS_C) $(LINKER)
	$(ARMGNU)-ld --no-undefined -L$(LIBGCC) $(OBJECTS) $(OBJECTS_C) -Map $(MAP) -o $(BUILD)output.elf -T $(LINKER) -lgcc

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


clean:
	rm $(BUILD)*
