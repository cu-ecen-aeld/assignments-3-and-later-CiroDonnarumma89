#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    echo "Cleaning Linux kernel"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    echo "Generating defconfig for Linux Kernel"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    echo "Building Linux Kernel"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all -j4
    echo "Building Linux Kernel Modules"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules -j4
    echo "Building Device Trees"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs

fi

cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi


mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs

mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p urs/bin usr/lib urs/sbin
mkdir -p var/log 

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    echo "Cleaning BusyBox"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- distclean
    echo "Generating defconfig for BusyBox"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
else
    cd busybox
fi

echo "Building BusyBox"
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -j4
echo "Installing BusyBox"
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- CONFIG_PREFIX=${OUTDIR}/rootfs -j4 install
cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Add library dependencies to rootfs
SYSROOT=$( ${CROSS_COMPILE}gcc -print-sysroot )
cp ${SYSROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
cp ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
cp ${SYSROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64
cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib

# Make device nodes
cd ${OUTDIR}/rootfs/dev

sudo mknod -m 666 null c 1 3
sudo mknod -m 666 tty c 5 0
sudo mknod -m 666 console c 5 1

ln -s console stdin
ln -s console stdout
ln -s console stderr

# Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cd ${OUTDIR}/rootfs/home/
mkdir -p conf
cp ${FINDER_APP_DIR}/writer .
cp ${FINDER_APP_DIR}/finder.sh .
cp ${FINDER_APP_DIR}/conf/assignment.txt ./conf
cp ${FINDER_APP_DIR}/conf/username.txt ./conf
cp ${FINDER_APP_DIR}/finder-test.sh .
cp ${FINDER_APP_DIR}/autorun-qemu.sh .


# Chown the root directory
sudo chown root:root ${OUTDIR}/rootfs/home

# Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio