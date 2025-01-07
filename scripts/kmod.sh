#!/bin/sh

# Reload the kernel module provided as an argument.
# It is assumed that the kernel module resides at KMOD_BASE/$1/$1.ko

KMOD_BASE=/mnt/host0/kmods

MODULE_NAME="$1"  # Accept module name as a command line argument

# Check if the module name is provided
if [ -z "$MODULE_NAME" ]; then
    echo "Usage: $0 <module_name>"
    exit 1
fi

module_path="${KMOD_BASE}/${MODULE_NAME}/${MODULE_NAME}.ko"

if [ ! -f "$module_path" ]; then
    echo "$module_path does not exist"
    exit
fi

if lsmod | grep -q "$MODULE_NAME"; then
    echo "Module '$MODULE_NAME' is loaded. Reloading..."
    rmmod "$MODULE_NAME"
    insmod "$module_path"
else
    # echo "Module '$MODULE_NAME' is not loaded. Loading..."
    insmod "$module_path"
fi

if lsmod | grep -q "$MODULE_NAME"; then
    echo "Reloaded module"
else
    echo "ERROR: FAILED TO RELOAD MODULE"
fi
