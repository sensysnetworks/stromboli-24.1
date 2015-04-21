# Copyright (C) 2002 Lorenzo Dozio (dozio@aero.polimi.it)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#!/bin/sh

clear 

currentDir=$PWD

PKG_DIR=packages
SRC_DIR=src
USR_DIR=usr

BUSYBOX=busybox-0.60.3.tar.bz2
BUSYBOX_SITE=ftp://ftp.uclibc.org/busybox
BUSYBOX_DIR=src/busybox-0.60.3

UCLIBC=uClibc-0.9.12.tar.bz2
UCLIBC_SITE=ftp://ftp.uclibc.org/uClibc
UCLIBC_DIR=src/uClibc-0.9.12

#SFDISK=sfdisk.tar.bz2
#SFDISK_SITE=ftp://ftp.uclibc.org
#SFDISK_DIR=src/sfdisk

KERNEL=linux-2.4.18.tar.bz2
KERNEL_SITE=ftp://ftp.funet.fi/pub/linux/kernel/v2.4
KERNEL_DIR=src/linux

RTAI=rtai-24.1.9.tgz
RTAI_SITE=http://www.aero.polimi.it/RTAI
RTAI_DIR=src/rtai-24.1.9

BOOTUP=color
RES_COL=60
MOVE_TO_COL="echo -en \\033[${RES_COL}G"
SETCOLOR_SUCCESS="echo -en \\033[1;32m"
SETCOLOR_NORMAL="echo -en \\033[0;39m"

echo_success () {
	[ "$BOOTUP" = "color" ] && $MOVE_TO_COL
	echo -n "[  "
	[ "$BOOTUP" = "color" ] && $SETCOLOR_SUCCESS
	echo -n "OK"
	[ "$BOOTUP" = "color" ] && $SETCOLOR_NORMAL
	echo -n "  ]"
	echo -ne "\n"
}

echo -e "\nBuilding uRTAI.\n"

if [ ! -f $PKG_DIR/$BUSYBOX ]
then
	wget -P packages --passive $BUSYBOX_SITE/$BUSYBOX
fi
if [ ! -f $PKG_DIR/$UCLIBC ]
then
	wget -P packages --passive $UCLIBC_SITE/$UCLIBC
fi
#if [ ! -f $PKG_DIR/$SFDISK ]
#then
#	wget -P packages --passive $SFDISK_SITE/$SFDISK
#fi
if [ ! -f $PKG_DIR/$KERNEL ]
then
	wget -P packages --passive $KERNEL_SITE/$KERNEL
fi
if [ ! -f $PKG_DIR/$RTAI ]
then
	wget -P packages --passive $RTAI_SITE/$RTAI
fi

if [ ! -d $SRC_DIR ]
then
	mkdir src
fi
if [ ! -d $USR_DIR ]
then
	mkdir usr
fi

if [ ! -d $BUSYBOX_DIR ]
then
	cd src
	tar Ivxf ../$PKG_DIR/$BUSYBOX
	ln -s $currentDir/$BUSYBOX_DIR busybox
	cd -
fi
if [ ! -d $UCLIBC_DIR ]
then
	cd src
	tar Ivxf ../$PKG_DIR/$UCLIBC
	ln -s $currentDir/$UCLIBC_DIR uClibc
	cd -
fi
#if [ ! -d $SFDISK_DIR ]
#then
#	cd src
#	tar Ivxf ../$PKG_DIR/$SFDISK
#	cd -
#fi
if [ ! -d $KERNEL_DIR ]
then
	cd src
	tar Ivxf ../$PKG_DIR/$KERNEL
	cd -
fi
if [ ! -d $RTAI_DIR ]
then
	cd src
	tar xvzf ../$PKG_DIR/$RTAI
	ln -s $currentDir/$RTAI_DIR rtai
	cd -
fi

dd if=/dev/zero of=urtai bs=1k count=4096
#losetup -d /dev/loop0
losetup /dev/loop0 urtai
mke2fs -q -F -m 0 -i 2000 urtai
mkdir -p rootfs
mount -o loop urtai rootfs/

cd $KERNEL_DIR
KPATCHED="`cat .config |grep CONFIG_RTHAL | awk ' { printf "%.12s", $1 } '`"
if [ "$KPATCHED" != CONFIG_RTHAL ]
then
        echo "Patching kernel ..."
	patch -b -p1<../rtai/patches/patch-2.4.18-rthal5g
else
        echo "Kernel already patched"
fi
make menuconfig
make dep
#make clean
make bzImage
cd -

cd $RTAI_DIR
make menuconfig
make dep
make
cd -

sh patch.uClibc $currentDir
cd $UCLIBC_DIR
make
make install
make PREFIX=$currentDir/rootfs install_target
cd -

sh patch.busybox $currentDir
cd $BUSYBOX_DIR
make PREFIX=$currentDir/rootfs install
cd -

mkdir -p rootfs/proc
mkdir -p rootfs/floppy

mkdir -p rootfs/rtai
cp $RTAI_DIR/modules/rtai.o rootfs/rtai/rtai.o
cp $RTAI_DIR/modules/rtai_sched.o rootfs/rtai/rtai_sched.o
cp $RTAI_DIR/modules/rtai_fifos.o rootfs/rtai/rtai_fifos.o
cp $RTAI_DIR/modules/rtai_shm.o rootfs/rtai/rtai_shm.o
cp $RTAI_DIR/modules/rtai_lxrt.o rootfs/rtai/rtai_lxrt.o
cp $RTAI_DIR/modules/rtai_tasklets.o rootfs/rtai/rtai_tasklets.o

mkdir -p rootfs/dev
cd rootfs/dev
mknod console c 5 1
mknod full c 1 7
mknod kmem c 1 2
mknod mem c 1 1
mknod null c 1 3
mknod port c 1 4
mknod random c 1 8
mknod urandom c 1 9
mknod zero c 1 5
ln -s /proc/kcore core
mknod fd0 b 2 0
mknod hda b 3 0
mknod hda1 b 3 1
mknod hda2 b 3 2
mknod fla b 62 0
mknod fla1 b 62 1
for i in `seq 0 7`; do
	mknod loop$i b 7 $i
done
for i in `seq 0 9`; do
	mknod -m 666 rtf$i c 150 $i
done
mknod -m 666 rtai_shm c 10 254

for i in `seq 0 9`; do
	mknod ram$i b 1 $i
done
ln -s ram1 ram
mknod tty c 5 0
for i in `seq 0 9`; do
	mknod tty$i c 4 $i
done
for i in `seq 0 9`; do
	mknod vcs$i b 7 $i
done
ln -s vcs0 vcs
for i in `seq 0 9`; do
	mknod vcsa$i b 7 $i
done
ln -s vcsa0 vcsa
cd -

if [ ! -d etc.urtai ]
then
	tar xvzf etc.urtai.tgz
fi
cp -a etc.urtai rootfs/etc

umount rootfs
gzip -9 urtai

losetup -d /dev/loop0

echo -e "\nInsert a floppy and press ENTER"
read dummy

mformat a:
syslinux -s /dev/fd0
echo -e "Copying syslinux ...\c"
mcopy syslinux.cfg a:
echo_success
echo -e "\nCopying the root fs ...\c"
mcopy urtai.gz a:
echo_success
echo -e "\nCopying the kernel ...\c"
mcopy $currentDir/src/linux/arch/i386/boot/bzImage a:linux
echo_success

echo -e "\nOK. You did it.\n"
echo -e "Try it and good luck!\n\n"
