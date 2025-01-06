#!/bin/bash

qemu-system-x86_64 \
    -kernel ~/Documents/Programming/LinuxKernelTorvalds/arch/x86_64/boot/bzImage \
    -nographic \
    -append "console=ttyS0" \
    -initrd ./initramfs.cpio.gz \
    -m 512 \
    -virtfs local,path=../,mount_tag=host0,security_model=mapped
