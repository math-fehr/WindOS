# WindOS
Experimental Raspberry Pi OS

# QEMU
L'installation se fait en entrant les commandes suivantes dans le dossier qemu-fvm:
* ./configure --target-list=arm-softmmu
* make

# OS Build
La construction de l'OS se fait simplement via le Makefile.
* make runs lance l'OS dans Qemu.
* Le fichier binaire construit est kernel.img ou kernel_qemu.img (dans img) selon la cible.
* ramfs_content est copié dans le système de fichier en mémoire.

# User build
Pour construire des applications utilisateur, lancer
arm-none-eabi-gcc [source.c] libc/\*.c -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=soft -mtune=cortex-a7 -std=gnu11 -static
