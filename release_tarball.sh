#!/bin/bash

version=`kdialog --inputbox "amaroK version: "` 
cvsroot=`kdialog --inputbox "CVS root directory: "`

folder=amarok-$version
mkdir $folder
pushd $folder
cvs -d $cvsroot co -l kdeextragear-1
pushd kdeextragear-1

cvs -z3 co -r KDE_3_2_BRANCH admin 
cvs -z3 up amarok
cvs -z3 up doc/amarok

# Remove CVS relevant files
find -name "CVS" -exec rm -rf {} \;
find -name ".cvsignore" -exec rm {} \;

pushd amarok

# Move some important files to the root folder
mv amarok.lsm ..
mv AUTHORS ..
mv ChangeLog ..
mv COPYING ..
mv INSTALL ..
mv README ..
mv TODO ..

pushd src
cat amarok.h | sed -e "s/APP_VERSION \".*\"/APP_VERSION \"$version"\"/ > amarok.h
popd

rm -rf debian

popd # kdeextragear-1

# Prevent using unsermake
oldmake=$UNSERMAKE
export UNSERMAKE="" && make -f Makefile.cvs
export UNSERMAKE=$oldmake

find -name "*" -exec touch {} \;
rm -rf autom4te.cache
rm stamp-h.in

mv * ..
popd # amaroK-foo
rm -rf kdeextragear-1
popd # root
tar -cf $folder.tar $folder
bzip2 $folder.tar
rm -rf $folder

echo
md5sum $folder.tar.bz2

echo
echo "Congratulations :) amaroK $version tarball generated."
echo "Now drink a few pints, have some fun on #amarok"
echo

