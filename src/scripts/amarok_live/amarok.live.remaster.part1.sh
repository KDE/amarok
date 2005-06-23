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

ROOT=`kdialog --title "Choose working directory" --getexistingdirectory .`

if [ $ROOT = 0 ] ; then
    exit;
fi

# Determine if enough space

redo=0
enough=0
tmp=$ROOT
while [ "$redo" = "0" ]; do
    while [ "$enough" = "0" ]; do # loops until found something
        if [[ -n  `df | grep $tmp` ]] ; then # we got it in df, find the space left
            free=0
            if [[ "$tmp" = "/" ]]; then # can't just grep, multiple /s in df
                anotmp=`df | grep / |  sed "s~^\([^ ]*\) *\([^ ]*\) *\([^ ]*\) *\([^ ]*\) *\([^ ]*\) *\(/\)$~\4~"`  # now we have a string, first item is free space in /
                free=`echo $anotmp  | sed "s~^\([^ ]*\) \(.*\)~\1~"` # get first space-delimited item
            else
                free=`df -k | grep $tmp |  sed "s~^\([^ ]*\) *\([^ ]*\) *\([^ ]*\) *\([^ ]*\) *\([^ ]*\) *\([^ ]*\)\(.*\)~\4~"`
            fi
            if [[ $free -gt 1572864 ]] ; then
                enough=1
                redo=1
                break
            else
                kdialog --sorry "Not enough free space. Please select another folder."
                break
            fi
        else
            res=`echo "$tmp" | sed "s~/\(.*\)/\([^/]*\)$~/\1~"`
            if [[ "$tmp" = "$res" ]]; then # last one, regexp fails here
                tmp="/"
            else #normal, removes one dir from path
                tmp=$res
            fi
            echo "new tmp: " $tmp
        fi
    done
    if [[ "$redo" = "0" ]]; then
        ROOT=`kdialog --title "Choose working directory" --getexistingdirectory .`
    fi
done



# Mount the iso if not already mounted
if [ ! -d "$DATADIR" ]; then
   DATADIR=$ROOT/livecd_data$$
   mkdir -p "$DATADIR"
   mount -o loop "$iso" "$DATADIR"
fi

# Make the working directories and blow out the initrd.gz into a separate directory 
 
mkdir -p $ROOT/mklivecd/livecd
cp -a --preserve "$DATADIR"/* $ROOT/mklivecd/livecd/
mkdir -p $ROOT/mklivecd/initrd.dir
mkdir -p $ROOT/mklivecd/initrd.mnt
gunzip -c $ROOT/mklivecd/livecd/isolinux/initrd.gz > $ROOT/mklivecd/livecd/isolinux/initrd
mount -o loop $ROOT/mklivecd/livecd/isolinux/initrd $ROOT/mklivecd/initrd.mnt
(cd $ROOT/mklivecd/initrd.mnt ; tar cf - .) | (cd $ROOT/mklivecd/initrd.dir ; tar xf -)
umount $ROOT/mklivecd/initrd.mnt
rm -f $ROOT/mklivecd/livecd/isolinux/initrd


# cleanup all temporary files and directories

umount "$DATADIR" 2>/dev/null >/dev/null
if [ "$?" = "0" ]; then rmdir $DATADIR; fi

# at this point we unsquash the fs so the user can add their own music

if [[  `cat /proc/filesystems | grep squash | wc -l` = 0 ]]; then
	modprobe squashfs
	if [[  `cat /proc/filesystems | grep squash | wc -l` = 0 ]]; then

    	kdialog --title "amaroK livecd remaster" --error "You do not have squashfs support enabled. You need to have a patched kernel with squashfs. You can find more info about squashfs, and how to patch your kernel, here: http://tldp.org/HOWTO/SquashFS-HOWTO/"
	
		rm -rf $ROOT/mklivecd
	fi
fi

mkdir $ROOT/amarok.livecd/
mount -o loop -t squashfs $ROOT/mklivecd/livecd/livecd.sqfs $ROOT/amarok.livecd/

# gotta copy it locally so the user can add files to it

mkdir $ROOT/amarok.live/
kfmclient --title "amaroK livecd remaster" copy $ROOT/amarok.livecd/* $ROOT/amarok.live/
umount $ROOT/amarok.livecd/
rmdir $ROOT/amarok.livecd

kdialog --title "amaroK livecd remaster" --msgbox "Copying done. To add music to the amaroK livecd, place additional music in /tmp/amarok.live/music/ Please do not add more than about 380 mb, as then the resulting ISO will be too large to fit on a CD-ROM. Once you are done, run the amarok.live.remaster.part2.sh script and you are finished!."

fi

else 

kdialog --title "amaroK livecd remaster" --sorry "You must run this script as root. Try running 'kdesu sh amarok.live.remaster.part1.sh' instead."

fi
