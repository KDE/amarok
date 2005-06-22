#!/usr/bin/env bash

#first, resquash the users' new fs
echo
echo "The music that you added is now being squashed and added"
echo "to the livecd. Please be VERY patient as this step can "
echo "take a LONG time."
echo
mksquashfs /tmp/amarok.live/ /tmp/mklivecd/livecd/livecd.sqfs -noappend


olddir=`pwd`
cd /tmp/mklivecd
dd if=/dev/zero of=livecd/isolinux/initrd bs=1k count=4646
mke2fs -q -m 0 -F -N 1250 -s 1 livecd/isolinux/initrd
mount -o loop -t ext2 livecd/isolinux/initrd initrd.mnt
rm -rf initrd.mnt/lost+found
(cd initrd.dir ; tar cf - .) | (cd initrd.mnt ; tar xf -)
umount initrd.mnt
cd livecd/isolinux
gzip -9 initrd
cd ..
ll
rm -f livecd.iso
mkisofs -J -R -V "Livecd Test" -o /root/livecd.iso -b isolinux/isolinux.bin -c isolinux/boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table .
cd $olddir
mv /root/livecd.iso amaroK.live.custom.iso

echo
echo "Livecd creation is done!. The new amaroK live! image is in"
echo `pwd`," called amaroK.live.custom.iso. You can burn that with"
echo "any standard burner and enjoy!"
echo
