#!/usr/bin/env bash

#first, resquash the users' new fs

if [ `/usr/bin/whoami` = 'root' ]; then

	WORK=$1

	dcop --all-users amarok playlist shortStatusMessage "The music that you added is now being squashed and added to the livecd. Please be VERY patient as this step can take a LONG time."
	
	mksquashfs $WORK/amarok.live/ $WORK/mklivecd/livecd/livecd.sqfs -noappend


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
	
	kdialog --title "amaroK livecd remaster" --yesno "Livecd creation is done. The new amaroK live image is in `pwd`, called amaroK.live.custom.iso. You can burn that with any standard burner and enjoy.\nDo you want to make more cds later? If so, please click yes, and you can simply re-run part2 of this script. If you are done, click no and the temporary files will be erased. You will need to rerun part1 to make another cd."
	
	if [[ $? = 1 ]]; then
		rm -rf $WORK/mklivecd/
		rm -rf $WORK/amarok.live/
        echo "end" > /tmp/amarok.script
	fi

	
else
	kdialog --title "amaroK livecd remaster" --sorry "You must run this script as root. Try running 'kdesu sh amarok.live.remaster.part1.sh' instead."

fi
