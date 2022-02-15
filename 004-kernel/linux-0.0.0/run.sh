#!/bin/bash
set -u -e

cat > bochsrc.bxrc << EOF
romimage: file=/usr/share/bochs/BIOS-bochs-latest, address=0xfffe0000
megs: 16
vgaromimage: file=/usr/share/bochs/VGABIOS-lgpl-latest
floppya: 1_44=a.img, status=inserted
boot: floppy
private_colormap: enabled=0
fullscreen: enabled=0
screenmode: name="sample"
EOF

# 1
make

# 2
bximage

# 3
dd if=boot of=a.img bs=512 conv=notrunc

bochs -f ./bochsrc.bxrc
