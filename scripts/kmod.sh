#!/bin/bash

# Reload the kernel module provided as an argument.
# It is assumed that the kernel module resides at KMOD_BASE/$1/$1.ko

KMOD_BASE=/mnt/host0/kmods
CWD=$(pwd)

function usage() {
    echo "Usage: $0 [OPTIONS] <MODULE_NAME>"
    echo ""
    echo "Options"
    echo "  -t, --tmp       Copy module to /tmp and load from there."
    echo "  -s, --sudo      Use sudo for rmmod and insmod."
    echo "  -h, --help      Display this help message."
    echo "  -v, --verbose   Show verbose output."
    echo ""
}

function log() {
    if [ "$verbose" = true ]; then
        printf "$(date +"%H:%M:%S") $* \n"
    fi
}

function log_err() {
    printf "$(date +"%H:%M:%S") ERROR: $* \n" >&2
}

use_tmp=false
use_sudo=false
verbose=false
module_name=

while [ "$#" -gt 0 ]; do
    case $1 in 
        -h|--help)
            usage
            exit
            ;;
        -t|--tmp)
            use_tmp=true
            shift
            ;;
        -s|--sudo)
            use_sudo=true
            shift
            ;;
        -v|--verbose)
            verbose=true
            shift
            ;;
        -*)
            log_err "Unknown option \"$1\""
            usage
            exit 1
            ;;
        *)
            if [ -n "$module_name" ]; then
                log_err "Only one module can be specified"
                usage
                exit 1
            fi
            module_name=$1
            shift
    esac
done

if [ -z "$module_name" ]; then
    log "Required argument MODULE_NAME missing"
    usage
    exit 1
fi

module_file=$module_name.ko
module_path=$(find . -name $module_file -exec realpath {} \;)

if [ -z "$module_path" ]; then
    echo "No file named $module_file found in subdirectories"
    exit 1
fi

if [ "$use_tmp" = true ]; then
    log "Copy module from $module_file to /tmp/$module_file"
    cp "$module_path" /tmp
    module_path=/tmp/$module_file
fi

prefix=
if [ "$use_sudo" = true ]; then
    log "Use sudo for (re)loading the module"
    prefix=sudo
fi

if lsmod | grep -q "$module_name"; then
    log "Module '$module_name' is loaded. Reloading..."
    eval "$prefix rmmod $module_name"
    eval "$prefix insmod $module_path"
else
    log "Module '$module_name' is not loaded. Loading..."
    eval "$prefix insmod $module_path"
fi

if lsmod | grep -q "$module_name"; then
    echo "Module $module_name has been (re)loaded"
else
    echo "ERROR: FAILED TO (RE)LOAD MODULE $module_name"
fi
