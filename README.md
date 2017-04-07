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
