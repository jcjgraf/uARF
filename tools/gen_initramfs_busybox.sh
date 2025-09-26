#!/bin/bash

# Generate a busybox based initramfs, designed to be used with QEMU.

set -u
set -e
set -E
set -o pipefail

trap 'log_err "Command failed at line $LINENO: $BASH_COMMAND"' ERR

# Open a new file descriptor that redirects to stdout:
# For logging
exec 3>&1

BB_VER="1.31.0"

SCRIPT_DIR="$(realpath "$(dirname "$0")")"
CWD=$(pwd)

# Tmp file to store initramfs structure
INITRAMFS_DIR="${CWD}/initramfs"

INITRAMFS_CPIO_GZ_FILE="initramfs.cpio.gz"

BUSYBOX_PATH="${CWD}/busybox"

INIT="/bin/sh"
GUEST_CWD="/"

# Colors
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
RESET="\033[0m"

function log_debug() {
    if [ "$verbose" = true ]; then
        printf "${BLUE}[DEBUG]${RESET} %s\n" "$*" 1>&3
    fi
}

function log_info() {
    printf "${GREEN}[INFO]${RESET} %b\n" "$*" 1>&3
}

function log_err() {
    printf "${RED}[ERROR]${RESET} %s\n" "$*" >&2
}

function user_confirms() {
    read -p "Shall we proceed? [y/N] " -r
    [[ $REPLY =~ ^[Yy]$ ]]
}

out=$CWD
init=
guest_cwd=$GUEST_CWD
force=false
verbose=false
share=false
recreate=false

function usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "   -i, --init <INIT>      Init executable to run (default: $INIT)"
    echo "   -o, --out <DIR>        Path to store the initramfs.cpio.gz (default: $out)"
    echo "   -f, --force            Ask for specific parts to be re-created."
    echo "   -s, --share            Mount p9 share"
    echo "   -w, --cwd <DIR>        Working directory to use on the guest (default: $guest_cwd)"
    echo "   -h, --help             Display this help message."
    echo "   -v, --verbose          Display verbose output."
}

PARSED_ARGUMENTS=$(getopt --name "$0" --options=i:o:fsw:hv --longoptions init:,out:,force,share,cwd:help,verbose -- "$@")
VALID_ARGUMENTS=$?
if [ "$VALID_ARGUMENTS" != "0" ]; then
    echo "Invalid argument"
    usage
    exit 1
fi

eval set -- "$PARSED_ARGUMENTS"
while true; do
    case "$1" in
        -i|--init)
            init=`realpath "$2"`
            shift 2
            ;;
        -o|--out)
            out=`realpath "$2"`
            shift 2
            ;;
        -f|--force)
            force=true
            shift
            ;;
        -s|--share)
            share=true
            shift
            ;;
        -w|--cwd)
            guest_cwd="$2"
            shift 2
            ;;
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
            break;
            ;;
        *)
            # We check for invalid options before. So this should not happen
            echo "Should not be executed"
            exit 2
            ;;
    esac
done

log_debug "Arguments parsed"

# Download busybox binary to $1
function get_busybox() {
    local busybox_bin=$1

    log_info "Downloading busybox binary to $busybox_bin"
    curl -L "https://www.busybox.net/downloads/binaries/${BB_VER}-defconfig-multiarch-musl/busybox-x86_64" > "$busybox_bin"
}

# Create initramfs file structure at $1 using busybox $2
function gen_initramfs() {
    local initramfs_dir=$1
    local busybox_bin=$2
    local guest_cwd=$3
    local share=$4

    # if [ ! -d $INITRAMFS_DIR ]; then
    log_info "Creating initramfs file structure at $initramfs_dir"
    mkdir -p $initramfs_dir
    cd $initramfs_dir
    mkdir -p bin dev etc lib mnt/host0 proc sbin sys tmp var
    cd bin
    cp $busybox_bin busybox
    chmod +x busybox
    ln -s busybox mount
    ln -s busybox sh

    if [ -n "$init" ]; then
        cp $init user_init
        chmod +x user_init
        #     init="exec /bin/user_init"
        # else
        #     init="setsid cttyhack $INIT"
    fi

    cd ..

    # Create init script
    echo "Creating init file"
    cat > init << EOF
#!/bin/busybox sh
/bin/busybox --install -s /bin
mount -t devtmpfs   devtmpfs    /dev
mount -t proc       proc        /proc
mount -t sysfs      sysfs       /sys
mount -t tmpfs      tmpfs       /tmp
$([ "$share" = true ] && echo "mount -t 9p host0 /mnt/host0 -o trans=virtio,version=9p2000.L")
echo "root:x:0:0:root:/root:/bin/sh" > /etc/passwd
echo "root::0:0:99999:7:::" > /etc/shadow
echo "root:x:0:" > /etc/group
chmod 644 /etc/passwd
chmod 640 /etc/shadow
chmod 644 /etc/group
echo "nameserver 10.0.2.3" > /etc/resolv.conf
ifconfig eth0 10.0.2.15
route add default gw 10.0.2.2
insmod /mnt/host0/uARF/kmods/pi-guest.ko
insmod /mnt/host0/uARF/kmods/rap-guest.ko
echo 512 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
$(echo "cd $guest_cwd")
$([ -n "$init" ] && echo "exec /bin/usr_init" || echo "setsid cttyhack $INIT")
EOF
    chmod +x init
}

# Create gz compressed cpio archive
function compress_initramfs() {
    local from=$1
    local to=$2
    echo "Creating initramfs.cpio.gz"
    cd $from
    find . | cpio -o -H newc | gzip > "$to/$INITRAMFS_CPIO_GZ_FILE"
}

if [[ -f "$BUSYBOX_PATH" && "$force" = true ]]; then
    log_info "Busybox exists locally. Do you want to redownload it?"
    if user_confirms; then
        rm -rf "$BUSYBOX_PATH"
        recreate=true
    fi
fi

if [ ! -f "$BUSYBOX_PATH" ]; then
    log_info "Downloading Busybox"
    get_busybox "$BUSYBOX_PATH"
else
    log_debug "Busybox already downloaded. Skip"
fi

if [ -d "$INITRAMFS_DIR" ]; then
    if [ "$recreate" = true ]; then
        log_debug "Regenerating initramfs as redownloaded busybox."
        rm -rf "$INITRAMFS_DIR"
    elif [ "$force" = true ]; then
        log_info "Initramfs already exists. Do you want to recreate it?"
        if user_confirms; then
            rm -rf "$INITRAMFS_DIR"
            recreate=true
        fi
    fi
fi

if [ ! -d "$INITRAMFS_DIR" ]; then
    log_info "Generating initramfs"
    gen_initramfs $INITRAMFS_DIR $BUSYBOX_PATH $guest_cwd $share
    log_info "Compress initramfs"
    compress_initramfs $INITRAMFS_DIR $out
else
    log_debug "Initramfs already exists. Skip"
fi

cd "$CWD"

log_info "Done"
