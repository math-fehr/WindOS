# WindOS
Experimental Raspberry Pi OS

# QEMU
Pour des raisons expérimentales, la version de Qemu des dépôts ne supporte pas l'émulation du raspberrypi 2. On utilise la version suivante:
https://github.com/fanwenyi0529/qemu-fvm/

L'installation se fait en entrant les commandes suivantes dans le dossier racine:
* ./configure --target-list=arm-softmmu
* make

L'exécutable produit est alors arm-softmmu/fvm-arm.
