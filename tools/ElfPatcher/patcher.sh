#!/usr/bin/env bash

# This script patches an ELF binary by overwriting the machine code of an
# existing function with custom assembly. It works by assembling provided
# `.S` file into raw bytes and writing those bytes into the correct offset
# inside the target ELF.

set -u
set -e

exec 3>&1

PATCH_O="/tmp/patch.o"
PATCH_BIN="/tmp/patch.bin"

verbose=false

function log_debug() {
    if [ "$verbose" = true ]; then
        printf "$(date +"%H:%M:%S") $* \n" 1>&3
    fi
}

function log_info() {
    printf "$* \n" 1>&3
}

function log_err() {
    printf "$(date +"%H:%M:%S") ERROR: $* \n" >&2
}

function usage() {
    echo "Usage: $0 [OPTIONS] ELF PATCH OFFSET"
    echo ""
    echo "Positional Arguments"
    echo "  ELF     Path to ELF to patch."
    echo "  PATCH   Path to .S file containing a .text section with the patch."
    echo "  OFFSET  VA to insert patch at. Obtained via 'obdjump'".
    echo ""
    echo "Options"
    echo "  -h, --help      Show this help menu."
    echo "  -v, --verbose   Show verbose output."
    echo ""
}

PARSED_ARGUMENTS=$(getopt --name "$0" --options=hv --longoptions help,verbose -- "$@")
VALID_ARGUMENTS=$?
if [ "$VALID_ARGUMENTS" != "0" ]; then
    echo "Invalid argument"
    usage
fi

eval set -- "$PARSED_ARGUMENTS"
while true; do
    case "$1" in
        -v|--verbose)
            verbose=true
            shift
            ;;
        -h|--help)
            usage
            exit 1
            ;;
        --)
            shift
            break
            ;;
        *)
            # We check for invalid options before. So this should not happen
            echo "Should not be executed"
            exit 2
            ;;
    esac
done

if [[ $# != 3 ]]; then
    echo "Invalid usage"
    usage
    exit 1
fi

elf_path=$1
patch_path=$2
offset=$(($3))

log_debug "Parsed arguments: elf: $elf_path, patch: $patch_path, offset: $offset"

patched_elf_path="${elf_path}_patched"

[ -f $elf_path ] || { echo "File '$elf_path' does not exist"; exit 1; }
[ -f $patch_path ] || { echo "File '$patch_path' does not exist"; exit 1; }
[ -f $patched_elf_path ] && { echo "Output '$patched_elf_path' already exists. Overwriting"; }

log_debug "Calculating offset in elf to patch"
# Base VA
base_va=$(("0x$(readelf -W -S "$elf_path" | awk '/\.text/ {print $4}')"))
# Offset where .text section starts in elf
text_offset=$(("0x$(readelf -W -S "$elf_path" | awk '/\.text/ {print $5}')"))
# It is possible that base_va == text_offset
# TODO: only tested this case so far
if [[ $base_va != $text_offset ]]; then
    log_err "TODO: Verify that the offset is calculated correctly"
    exit 1
fi
elf_offset=$((text_offset + offset - base_va))
log_info "Calculated offset in elf $elf_offset"

log_info "Assemble patch $patch_path to $PATCH_O"
gcc -c "$patch_path" -o "$PATCH_O"

log_info "Extract .text from $PATCH_O to binary $PATCH_BIN"
objcopy -O binary --only-section=.text "$PATCH_O" "$PATCH_BIN"

patch_bin_size=$(stat -c %s "$PATCH_BIN")
log_info "Patch has a size of $patch_bin_size bytes"

log_info "Patching $elf_path using patch $patch_path at offset $elf_offset storing patched version to $patched_elf_path"
cp "$elf_path" "$patched_elf_path"
dd if="$PATCH_BIN" of="$patched_elf_path" bs=1 seek=$elf_offset count=$patch_bin_size conv=notrunc 2> /dev/null
rm -f "$PATCH_O" "$PATCH_BIN"
log_info "Done. Patched ELF is at $patched_elf_path"

