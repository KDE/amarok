#!/bin/bash
#
# get-amarok-cvssnapshot.sh
# This script installs the current amaroK cvs snapshot on your pc.

OLDDIR=`pwd`
TMPDIR="/tmp/amaroK-`date +%Y%j%H%M%S`.tmp"
if [ "$1" = "-y" ]; then
	FILE="amarok-1.1-CVS-yesterday.tar.bz2"
else
	FILE="amarok-1.1-CVS.tar.bz2"
fi

mkdir $TMPDIR
cd $TMPDIR
wget http://amarok.sf.net/download/cvs-tarball/$FILE
tar xvjpf $FILE
cd amarok-1.1-CVS
make -f Makefile.cvs
sh configure --prefix=`kde-config --prefix`
make
if [ "$?" = "0" ]; then
        clear;
        echo "Compilation successful.";
        echo "Please enter your root password to install amaroK.";
        su -c "make install"
fi
cd $OLDDIR
rm -rf $TMPDIR
     
