ARMGNU = arm-none-eabi

BUILD = build/

SOURCE = src/

TARGET = img/kernel.img

IMGDIR = img/

LINKER = kernel.ld

MAP = kernel.map

OBJECTS =  $(patsubst $(SOURCE)%.s,$(BUILD)%.o,$(wildcard $(SOURCE)*.s))

OBJECTS_C =  $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))

LIBGCC = $(shell dirname `$(ARMGNU)-gcc -print-libgcc-file-name`)

QEMU = ~/opt/src/qemu/build/arm-softmmu/qemu-system-arm

DEPDIR = .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell mkdir -p $(BUILD) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

all: $(TARGET)

rebuild: all

mkfs:
	qemu-img create fs.img 100M
	mkfs -t fat fs.img

img: fs.img

fs.img: $(wildcard $(IMGDIR)*)
	mcopy -D o -i fs.img $? ::

run: $(TARGET)
	#qemu-system-arm -kernel img/kernel.img -cpu arm1176 -m 256 -M versatilepb -no-reboot  -serial stdio -append "root=/dev/sda2 panic=1" -hda fs.img
	$(QEMU) -kernel $(TARGET) -m 256 -M raspi2 -monitor stdio -serial pty

runs: $(TARGET)
	$(QEMU) -kernel $(TARGET) -m 256 -M raspi2 -serial stdio

$(TARGET) : $(BUILD)output.elf
	$(ARMGNU)-objcopy $(BUILD)output.elf -O binary $(TARGET)

$(BUILD)output.elf : $(OBJECTS) $(OBJECTS_C) $(LINKER)
	$(ARMGNU)-ld --no-undefined -L$(LIBGCC) $(OBJECTS) $(OBJECTS_C) -Map $(MAP) -o $(BUILD)output.elf -T $(LINKER) -lgcc

$(BUILD)%.o: $(SOURCE)%.s
	$(ARMGNU)-as -I $(SOURCE) $< -o $@

$(BUILD)%.o: $(SOURCE)%.c
$(BUILD)%.o: $(SOURCE)%.c $(DEPDIR)/%.d
	$(ARMGNU)-gcc $(DEPFLAGS) -mcpu=arm1176jzf-s -fpic -std=gnu99 -c $< -o $@ -O2 -Wall -Wextra -nostdlib -lgcc

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))

clean:
	rm $(BUILD)*
