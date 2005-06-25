#!/usr/bin/env bash

############################################################################
# Second part of the amaroK LiveCD remastering scripts
# Reassemble the iso after adding media files
#
# Based on a script authored by Ivan Kerekes <ikerekes@gmail.com>
# and modified by Leo Franchi <lfranchi at gmail.com>, Mike Diehl 
# <madpenguin8 at yahoo.com> and Greg Meyer <greg at gkmweb.com> 
#
# See the file called README for more information
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

# script must be run as root, first check for and exit if not
# then resquash the users' new fs

if [ `/usr/bin/whoami` = 'root' ]; then

	WORK=$1

	dcop --all-users amarok playlist shortStatusMessage "The music that you added is now being squashed and added to the livecd. Please be VERY patient as this step can take a LONG time."
	
	which mksquashfs
    if [[ $? == 0 ]]; then
        mksquashfs $WORK/amarok.live/ $WORK/mklivecd/livecd/livecd.sqfs -noappend
    else
        dcop --all-users amarok playlist popupMessage "Squashfs-tools not found! Make sure mksquashfs is in your root \$PATH"
        exit
    fi

	olddir=`pwd`
	cd $WORK/mklivecd
	dd if=/dev/zero of=livecd/isolinux/initrd bs=1k count=4646
	mke2fs -q -m 0 -F -N 1250 -s 1 livecd/isolinux/initrd
	mount -o loop -t ext2 livecd/isolinux/initrd initrd.mnt
	rm -rf initrd.mnt/lost+found
	(cd initrd.dir ; tar cf - .) | (cd initrd.mnt ; tar xf -)
	umount initrd.mnt
	cd livecd/isolinux
	gzip -f -9 initrd
	cd ..
	ll
	rm -f livecd.iso
	mkisofs -J -R -V "Livecd Test" -o $WORK/amaroK.live.custom.iso -b isolinux/isolinux.bin -c isolinux/boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table .
	cd $olddir
#	mv $WORK/amaroK.live.custom.iso .

    which k3b

	if [[ $? == 0 ]] ; then
        k3b --cdimage $WORK/amaroK.live.custom.iso
    else 
        kdialog --title "amaroK livecd remaster" --yesno "Livecd creation is done. The new amaroK live image is in `pwd`, called amaroK.live.custom.iso. You can burn that with any standard burner and enjoy.\nDo you want to make more cds later? If so, please click yes, and you can simply re-run part2 of this script. If you are done, click no and the temporary files will be erased. You will need to rerun part1 to make another cd."
    fi

	if [[ $? = 1 ]]; then
		rm -rf $WORK/mklivecd/
		rm -rf $WORK/amarok.live/
        echo "end" > /tmp/amarok.script
	fi

	
else
	kdialog --title "amaroK livecd remaster" --sorry "You must run this script as root. Try running 'kdesu sh amarok.live.remaster.part1.sh' instead."

fi
