#!/usr/bin/env bash
#set -x 
# Disassemble a mklivecd iso file into the /tmp/mklivecd directory
# Author: Ivan Kerekes <ikerekes@gmail.com>
#
# Based on a script authored by Ivan Kerekes <ikerekes@gmail.com>
# and modified by Leo Franchi, Mike Diehl and Greg Meyer 

# We need to be root to mount the loopback

# check for root and exit if not
if [ `/usr/bin/whoami` = 'root' ]; then

kdialog --title "amaroK livecd remaster" --yesno "Welcome to the amaroK live cd remaster utility. The first step is to select the iso image, would you like to continue."

if [ $? = 0 ] ; then

iso=`kdialog --getopenfilename /home "*.iso"`

# Mount the iso if not already mounted
if [ ! -d "$DATADIR" ]; then
   DATADIR=/tmp/livecd_data$$
   mkdir -p "$DATADIR"
   mount -o loop "$iso" "$DATADIR"
fi

# Make the working directories and blow out the initrd.gz into a separate directory 
 
mkdir -p /tmp/mklivecd/livecd
cp -a --preserve "$DATADIR"/* /tmp/mklivecd/livecd/
mkdir -p /tmp/mklivecd/initrd.dir
mkdir -p /tmp/mklivecd/initrd.mnt
gunzip -c /tmp/mklivecd/livecd/isolinux/initrd.gz > /tmp/mklivecd/livecd/isolinux/initrd
mount -o loop /tmp/mklivecd/livecd/isolinux/initrd /tmp/mklivecd/initrd.mnt
(cd /tmp/mklivecd/initrd.mnt ; tar cf - .) | (cd /tmp/mklivecd/initrd.dir ; tar xf -)
umount /tmp/mklivecd/initrd.mnt
rm -f /tmp/mklivecd/livecd/isolinux/initrd


# cleanup all temporary files and directories

umount "$DATADIR" 2>/dev/null >/dev/null
if [ "$?" = "0" ]; then rmdir $DATADIR; fi

# at this point we unsquash the fs so the user can add their own music

if [[  `cat /proc/filesystems | grep squash | wc -l` = 0 ]]; then
    kdialog --title "amaroK livecd remaster" --msgbox "You do not have squashfs support enabled. You need to have a patched kernel with squashfs. You can find more info about squashfs, and how to patch your kernel, here: http://tldp.org/HOWTO/SquashFS-HOWTO/"
    exit
fi

mkdir /tmp/amarok.livecd/
mount -o loop -t squashfs /tmp/mklivecd/livecd/livecd.sqfs /tmp/amarok.livecd/

# gotta copy it locally so the user can add files to it

mkdir /tmp/amarok.live/
kfmclient --title "amaroK livecd remaster" copy /tmp/amarok.livecd/* /tmp/amarok.live/
umount /tmp/amarok.livecd/
rmdir /tmp/amarok.livecd

kdialog --title "amaroK livecd remaster" --msgbox "Copying done. To add music to the amaroK livecd, place additional music in /tmp/amarok.live/music/ Please do not add more than about 380 mb, as then the resulting ISO will be too large to fit on a CD-ROM. Once you are done, run the amarok.live.remaster.part2.sh script and you are finished!."

fi

else 

kdialog --title "amaroK livecd remaster" --sorry "You must run this script as root. Try running 'kdesu sh amarok.live.remaster.part1.sh' instead."

fi
