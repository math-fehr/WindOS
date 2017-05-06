ARMGNU = arm-none-eabi

BUILD = build/

SOURCE = src/

RAMFS = ramfs_content/

TARGET = img/kernel.img
TARGET_QEMU = img/kernel_qemu.img

IMGDIR = img/

LINKER = kernel.ld
LINKER_QEMU = kernel_qemu.ld

USR_SRC 	= usr/
USR_LIB		= libc/cstubs.c
USR_BINDIR 	= $(RAMFS)bin/

# recursive wildcard.
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

OBJECTS 	= $(BUILD)fs.img $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))
OBJECTS_C 	= $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(call rwildcard, $(SOURCE), *.c))
RAMFS_OBJ 	= $(call rwildcard, $(RAMFS), *)
USR_BIN	  	= $(patsubst $(USR_SRC)%,$(USR_BINDIR)%, $(wildcard $(USR_SRC)*))

LIBGCC = $(shell dirname `$(ARMGNU)-gcc -print-libgcc-file-name`)

LIB_USPI          = uspi/lib/libuspi.a
LIB_USPI_CFG      = uspi/Config.mk
LIB_USPI_DIR      = uspi/lib/
USPI_INCLUDE_DIR  = uspi/include

CLIBS = -L$(LIB_USPI_DIR) -luspi

CFLAGS = -O2 -Wall -Wextra -nostdlib -lgcc -std=gnu99 $(INCLUDE_C) -mno-unaligned-access $(CLIBS) -I $(USPI_INCLUDE_DIR)

HARDWARE_FLAGS = -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=soft \
								 -mtune=cortex-a7
RPI_FLAG = -D RPI2
RPI_FLAG_S = --defsym RPI2=1

SFLAGS = $(INCLUDE_C)

QEMU = qemu-fvm/arm-softmmu/fvm-arm

SD_NAPPY = /media/nappy/boot/
SD_LORTEX = /media/lucas/460D-5801/

DEPDIR = .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell mkdir -p $(BUILD) >/dev/null)
$(shell mkdir -p $(RAMFS) >/dev/null)
$(shell mkdir -p $(USR_BINDIR) >/dev/null)

print-%  : ; @echo $* = $($*)

#all: builds the kernel image for the real hardware. RPI2 flag by default.
all: $(LIB_USPI) $(TARGET) $(USR_BIN)

#qemu: builds the kernel image for qemu emulation.
qemu: $(LIB_USPI) $(TARGET_QEMU) $(USR_BIN)

#rpi: sets the flag for the RPI1 build.
rpi: RPI_FLAG = -D RPI
rpi: HARDWARE_FLAGS = -mfpu=vfp -mfloat-abi=soft -march=armv6zk -mtune=arm1176jzf-s
rpi: RPI_FLAG_S = --defsym RPI2=1
rpi: all

#rpi: sets the flag for the RPI2 build.
rpi2: RPI_FLAG = -D RPI2
rpi2: all

# Builds a 1MB filesystem
$(BUILD)fs.img: $(RAMFS_OBJ)
	genext2fs -b 4096 -i 256 -d $(RAMFS) $(BUILD)fs.tmp
	$(ARMGNU)-ld -b binary -r -o $(BUILD)fs.ren $(BUILD)fs.tmp
	$(ARMGNU)-objcopy --rename-section .data=.fs \
										--set-section-flags .data=alloc,code,load \
										$(BUILD)fs.ren $(BUILD)fs.img

run: $(TARGET_QEMU) $(USR_BIN)
	$(QEMU) -kernel $(TARGET_QEMU) -m 256 -M raspi2 -monitor stdio -serial pty -serial pty

runs: $(TARGET_QEMU) $(USR_BIN)
	$(QEMU) -kernel $(TARGET_QEMU) -m 256 -M raspi2 -serial pty -serial stdio


minicom:
	minicom -b 115200 -o -D /dev/pts/2

$(TARGET) : $(BUILD)output.elf
	$(ARMGNU)-objcopy $(BUILD)output.elf -O binary $(TARGET)

$(TARGET_QEMU) : $(BUILD)output_qemu.elf
	$(ARMGNU)-objcopy $(BUILD)output_qemu.elf -O binary $(TARGET_QEMU)

$(BUILD)output.elf : $(LIB_USPI) $(OBJECTS) $(OBJECTS_C) $(LINKER)
	$(ARMGNU)-ld --no-undefined -L$(LIBGCC) $(OBJECTS) $(OBJECTS_C) $(LIBC) \
							 -o $(BUILD)output.elf -T $(LINKER) -lg -lgcc

$(BUILD)output_qemu.elf : $(LIB_USPI) $(OBJECTS) $(OBJECTS_C) $(LINKER)
	$(ARMGNU)-ld --no-undefined -L$(LIBGCC) $(OBJECTS) $(OBJECTS_C) $(LIBC) \
							 -o $(BUILD)output_qemu.elf -T $(LINKER_QEMU) -lg -lgcc

$(BUILD)%.o: $(SOURCE)%.S
	$(ARMGNU)-gcc $(CFLAGS) $(HARDWARE_FLAGS) -I $(SOURCE) -c $< -o $@ $(RPI_FLAG)
#$(ARMGNU)-as -I $(SOURCE) $< -o $@ $(SFLAGS) $(RPI_FLAG)

-include $(BUILD)$(OBJECTS_C:.o=.d)

$(BUILD)%.o: $(SOURCE)%.c
	$(ARMGNU)-gcc $(CFLAGS) $(HARDWARE_FLAGS) $(RPI_FLAG) -c $< -o $@
	$(ARMGNU)-gcc $(CFLAGS) $(HARDWARE_FLAGS) $(RPI_FLAG) -MM $< -o $(BUILD)$*.d
	@mv -f $(BUILD)$*.d $(BUILD)$*.d.tmp
	@sed -e 's|.*:|$(BUILD)$*.o:|' < $(BUILD)$*.d.tmp > $(BUILD)$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILD)$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILD)$*.d
	@rm -f $(BUILD)$*.d.tmp


$(LIB_USPI): $(LIB_USPI_CFG)
	make -C $(LIB_USPI_DIR)

$(LIB_USPI_CFG):
	echo "RASPPI = 1" > $@ ; \
	echo "PREFIX = arm-none-eabi-" >> $@ ;\

# Userspace environment build.
$(USR_BINDIR)%: $(USR_SRC)%/* $(USR_LIB)
	$(ARMGNU)-gcc $(USR_SRC)$*/* $(USR_LIB) -std=gnu99 -static -o $@


copy_nappy: all
	cp $(IMGDIR)* $(SD_NAPPY)
copy_lortex: rpi
	cp $(IMGDIR)* $(SD_LORTEX)

clean:
	@rm -fr $(BUILD)*
	@rm -f $(TARGET)
	@rm -f $(TARGET_QEMU)
	make -C $(LIB_USPI_DIR) clean
	rm -f $(LIB_USPI_CFG)
