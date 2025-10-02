#!/usr/bin/env bash

# Attach GDB to running QEMU instance

set -e
set -u

SCRIPT_DIR="$(realpath "$(dirname "$0")")"
cd "$SCRIPT_DIR"

uid=$(id -u)

qemu_pid=$(pgrep -u $uid qemu-system-x86 || true) # pgrep returns error if not found
num_pids=$(wc -l <<< "$qemu_pid")

[ $num_pids -eq 0 ] && { echo "Are you sure QEMU is running? Exit"; exit 1; }
[ $num_pids -ne 1 ] && { echo "Multiple instances of QEMU are running. Exit"; exit 1; }

echo "Attaching to '$qemu_pid'"

sudo gdb -p $qemu_pid --command=gdb.script
