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
