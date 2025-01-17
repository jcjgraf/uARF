#!/bin/bash

# This script is designed to synchronize all files inside SRC to DST. The files
# COMMON_EXCLUDE and if -a it not used BUILD_EXCLUDE are excluded. Contains also a
# daemon that watches the files returned by WATCH_FILES_CMD and triggers a sync upon
# change.

CWD=$(pwd)
SCRIPT_PATH=$(realpath "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

SRC="$HOME/CurrentCourse/Code/uARF/" # Trailing slash matters!
DST="cn128:/data/jegraf/MA/uARF"

COMMON_EXCLUDE=(
    --exclude=.git \
    --exclude=.cache \
)

BUILD_EXCLUDE=(
    --exclude "*.o" \
    --exclude "*.bin" \
    --exclude "*.d" \
    --exclude "*.a" \
    --exclude "*.cpio.gz" \
    --exclude "*.ko" \
    --exclude "*.cmd" \
    --exclude "Module.symvers" \
    --exclude "modules.order" \
    --exclude "*.mod" \
    --exclude "*.mod.c" \
    --exclude=scripts/busybox \
    --exclude=scripts/initramfs \
    --exclude=scripts/bzImage \
)

WATCH_FILES_CMD=(
    find "$SRC" \
    -type f \
    -name "*.c" \
    -or -name "*.h" \
    -or -name "*.S" \
    -or -name ".config" \
    -or -name "Makefile" \
    -or -name "*.bin" \
    -or -name "*.ko" \
    # -or -name "*.sh" \
)

# Terminate the synchronization after time time
# Done because rsync somehow gets blocked sometimes
SYNC_TIMEOUT=20

all=false
force=false
daemon=false

function usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -a      Sync all files (including build artefacts etc.)"
    echo "  -f      Force sync, deleting all files at target that are not in the source."
    echo "  -d      Start sync daemon"
    echo "  -h      Show this help message"
    exit 1
}

function log() {
    printf "$(date +"%H:%M:%S") $* \n"
}

function log_err() {
    printf "$(date +"%H:%M:%S") ERROR: $* \n" >&2
    notify-send -u Critical "SYNC ERROR" "$*\n"
}

# Parse options
while getopts "afdh" opt; do
    case $opt in
        a) all=true ;;
        f) force=true ;;
        d) daemon=true ;;
        h) usage ;;
        *) usage ;;
    esac
done

# Composes the rsync command, based on the cli arguments
function get_sync_cmd() {
    local cmd=(rsync --info=NAME -a)
    cmd+=(${COMMON_EXCLUDE[@]})

    if [ "$all" = false ]; then
        cmd+=(${BUILD_EXCLUDE[@]})
    fi

    if [ "$force" = true ]; then
        cmd+=(--delete)
    fi

    cmd+=( "$SRC" )
    cmd+=( "$DST" )

    echo "${cmd[@]}"
}

# Returns a list of files to watch
function get_watch_files() {
    eval "${WATCH_FILES_CMD[@]}"
}

sync_cmd=$(get_sync_cmd)

if [ "$daemon" = true ]; then
    # TODO: should be update this list inside loop?
    watch_files=$(get_watch_files)
    log "Watching files: \n$watch_files"
    log "Running upon event: $sync_cmd"

    while true; do
        echo "==================="
        log "Start sync"
        timeout $SYNC_TIMEOUT $sync_cmd

        retVal=$?

        if [ $retVal -eq 124 ]; then
            log_err "Timeout triggered ($SYNC_TIMEOUT seconds)! Retry..."
            continue
        fi

        if [ $? -eq 125 ]; then
            log_err "timeout utility failed, exit"
            exit $?
        fi

        if [ $retVal -eq 125 ]; then
            log_err "timeout utility failed, exit"
            exit $?
        fi

        if [ $retVal -ne 0 ]; then
            log_err "Rsync Error Encountered: $retVal, exit"
            exit $?
        fi

        log "Sync done. Block for new event"
        notify-send -t 200 DONE

        # Wait for event to happen
        # Editors may not modify, but "replace" the file => monitor other events too
        inotifywait -e modify -e delete_self -e move_self $watch_files

        if [ $? -ne 0 ]; then
            log_err "inotifywait failed, exit"
            exit $?
        fi
    done
else
    log "Running: $sync_cmd"
    eval "$sync_cmd"
fi
