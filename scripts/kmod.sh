#!/bin/bash

# Reload the kernel module provided as an argument.
# It is assumed that the kernel module resides at KMOD_BASE/$1/$1.ko

CWD=$(pwd)

function usage() {
    echo "Usage: $0 [OPTIONS] <MODULE_NAME>"
    echo ""
    echo "Options"
    echo "  -t, --tmp       Copy module to /tmp and load from there."
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

if lsmod | grep -qE "^$module_name\s+"; then
    log "Module '$module_name' is loaded. Reloading..."
    rmmod $module_name
    out=$?
    if [ $out -ne 0 ]; then
        log_err "Removing module"
        exit 1
    fi
else
    log "Module '$module_name' is not loaded. Loading..."
fi

insmod $module_path
out=$?
if [ $out -ne 0 ]; then
    log_err "Inserting module"
    exit 1
fi

if lsmod | grep -qE "^$module_name\s+"; then
    echo "Module $module_name has been (re)loaded"
else
    log_err "ERROR: FAILED TO (RE)LOAD MODULE $module_name"
    exit 1
fi
