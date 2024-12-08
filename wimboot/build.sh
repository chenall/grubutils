#!/bin/bash

rm -rf build

git clone https://github.com/ipxe/wimboot.git build && cd build
git checkout v2.8.0

git apply ../0001_Add-EFI-LoadFile2-and-InitrdMedia-headers.patch
git apply ../0002_Fix-optinal-header-in-wimboot-2.8.0.patch
git apply ../0003_Provide-common-vdisk_read_mem_file-cpio-handler.patch
git apply ../0004_Support-EFI-linux-initrd-media-loading.patch

make -C src wimboot.x86_64

mv src/wimboot.x86_64 ../wimboot
