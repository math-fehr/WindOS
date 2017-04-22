# WindOS
Experimental Raspberry Pi OS

# QEMU
L'installation se fait en entrant les commandes suivantes dans le dossier qemu-fvm:
* ./configure --target-list=arm-softmmu
* make

# NEWLIB
L'installation se fait en entrant les commandes suivantes dans le dossier newlib-cygwin (long):
./configure --target arm-none-eabi --disable-newlib-supplied-syscalls
make

# Génerer un système de fichiers ext2 grâce à genext2fs
genext2fs -b 1024 -d ramfs_content/ fs.out


0x00000000 -> 0x7fffffff = user process memory (not mapped)
0x80000000 -> 0xbeffffff = mapped to physical memory
0xbf000000 -> 0xbfffffff => 0x20000000 -> 0x20ffffff RPI1
                         => 0x3f000000 -> 0x3fffffff RPI2
(peripherals are also mapped in the same way but with cache disabled)
0xc0000000 -> 0xefffffff = kernel dynamic data (=> mapped to KERNEL_IMAGE_SIZE (blocks allocated on the run with sbrk))
0xf0000000 -> (0xf0000000 + KERNEL_IMAGE_SIZE + kstart) = kernel code (=> mapped to 0x0 -> (KERNEL_IMAGE_SIZE + kstart))
