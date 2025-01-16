#!/bin/bash

# Default configurations
CWD=$(pwd)
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

KERNEL="${SCRIPTPATH}/bzImage"
INITRAMFS="${SCRIPTPATH}/initramfs.cpio.gz"
SHARE_DIR="${SCRIPTPATH}/toShare"

function usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -k <kernel>       Path to kernel image (default: $KERNEL)"
    echo "  -i <initramfs>    Path to initramfs (default: $INITRAMFS)"
    echo "  -s <share_dir>    Path to shared directory (default: $SHARE_DIR)"
    echo "  -h                Show this help message"
    exit 1
}

# Parse options
while getopts "k:i:m:s:a:gh" opt; do
    case $opt in
        k) KERNEL=$OPTARG ;;
        i) INITRAMFS=$OPTARG ;;
        s) SHARE_DIR=$OPTARG ;;
        h) usage ;;
        *) usage ;;
    esac
done

# Validate inputs
if [ ! -f "$KERNEL" ]; then
    echo "Error: Kernel image not found at $KERNEL"
    exit 1
fi

if [ ! -f "$INITRAMFS" ]; then
    echo "Error: Initramfs not found at $INITRAMFS"
    exit 1
fi

if [ ! -d "$SHARE_DIR" ]; then
    echo "Error: Shared directory not found at $SHARE_DIR"
    exit 1
fi

# TODO: Assert KVM is available

# Run QEMU
qemu-system-x86_64 \
    -kernel "$KERNEL" \
    -initrd "$INITRAMFS" \
    -nographic \
    -append "console=ttyS0 nokaslr" \
    -m 512 \
    -cpu host \
    -enable-kvm \
    -gdb tcp::1234 \
    -virtfs local,path=$SHARE_DIR,mount_tag=host0,security_model=mapped
