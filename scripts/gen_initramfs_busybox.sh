#!/bin/bash

BB_VER="1.31.0"

CWD=$(pwd)

INITRAMFS_DIR="${CWD}/initramfs"
BUSYBOX_PATH="${CWD}/busybox"
INITRAMFS_CPIO_GZ_PATH="${CWD}/initramfs.cpio.gz"

# Get busybox
if [ ! -f $BUSYBOX_PATH ]; then
    echo "Downloading busybox"
    curl -L "https://www.busybox.net/downloads/binaries/${BB_VER}-defconfig-multiarch-musl/busybox-x86_64" > "${BUSYBOX_PATH}"
fi

if [ ! -d $INITRAMFS_DIR ]; then
    # Create initramfs file structure
    echo "Creating initramfs file structure"
    mkdir -p "${INITRAMFS_DIR}"
    cd "${INITRAMFS_DIR}"
    mkdir -p bin dev etc lib mnt/host0 proc sbin sys tmp var
    cd bin
    cp "${BUSYBOX_PATH}" busybox
    chmod +x busybox
    ln -s busybox mount
    ln -s busybox sh
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
mount -t 9p         host0       /mnt/host0  -o trans=virtio,version=9p2000.L
echo "root:x:0:0:root:/root:/bin/sh" > /etc/passwd
echo "root::0:0:99999:7:::" > /etc/shadow
echo "root:x:0:" > /etc/group
chmod 644 /etc/passwd
chmod 640 /etc/shadow
chmod 644 /etc/group
setsid cttyhack sh
exec /bin/sh
EOF
    chmod +x init
fi

# Create gz compressed cpio archive
echo "Creating initramfs.cpio.gz"
cd "${INITRAMFS_DIR}"
find . | cpio -o -H newc | gzip > "${INITRAMFS_CPIO_GZ_PATH}"

cd "${CWD}"
