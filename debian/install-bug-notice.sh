#! /bin/sh	    

for p in "$@"; do
    mkdir -p debian/$p/usr/share/bug/$p
    cp debian/bug/* debian/$p/usr/share/bug/$p
done
