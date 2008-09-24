#!/bin/bash

# Assume you have mysql sources at ~/dev/mysql
# Create dir ~/dev/mysql/patches and copy patches over there
# Then run this script from ~/dev/mysql dir like this: ./build-mysqle.sh --prefix=${HOME}/usr
# This script respects CFLAGS, CXXFLAGS and MAKEOPTS variables

echo ">>> Hi there!"

if [ -e .patched ]
then
    echo ">>> Looks like sources are already patched. Remove .patched if that's untrue"
else
    echo -n "Where're mysql patches located? [patches] "
    read -e ENTERED_PATCHDIR
    
    PATCHDIR=${ENTERED_PATCHDIR:-patches}
    patchcount=0
    failcount=0
    for k in  ${PATCHDIR}/*.patch
    do
        echo ">>> Applying patch $k #" $((patchcount++))
        patch -p1 < $k || echo "!!! Oh noes! patch failed! Failure count:" $((failcount++))
    done
    
    [ $failcount -gt 0 ] && echo "!!! Some patches could not be applied"
    [ $patchcount -le 0 ] && { echo "!!! No patches found, did you misspell patch directory?"; exit 1; }

    echo $PATCHDIR > .patched
fi

if [ -e .configured ]
then
    echo ">>> Looks like sources are already configured. Remove .configured if that's untrue"
else
    MYOPTS="--without-server --with-embedded-server --without-docs --without-man --without-bench --without-ssl --without-extra-tools --with-pic"
    EXTRAOPTS="$@"
    CFLAGS="-fPIC -ggdb ${CFLAGS}"
    CXXFLAGS="-fPIC -ggdb ${CXXFLAGS}"
    echo ">>> Ready to run configure like this:"
    echo "CFLAGS=\"${CFLAGS}\" CXXFLAGS=\"${CXXFLAGS}\" ./configure ${MYOPTS} ${EXTRAOPTS}"
    echo -n ">>> Configuring in "
    countdown=6
    while [ $countdown -gt 0 ]
    do
        echo -n $((countdown--)) ""
        sleep 1
    done
    
    CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" ./configure ${MYOPTS} ${EXTRAOPTS} || {
        echo "!!! configure failed, aborting"
        exit 2
    }

    touch .configured
fi

echo ">>> Running make"

make ${MAKEOPTS} || {
    echo "!!! make failed, aborting"
    exit 3
}

echo ">>> Reverting mysql.h patch"
patch -R -p1 < $( cat .patched )/printerrorbeforeexit.patch 

echo ">>> Running make install"

make install || {
    echo "!!! make install failed"
    exit 4
}

echo ">>> Reapplying mysql.h patch for future builds"
patch -p1 < $( cat .patched )/printerrorbeforeexit.patch 

echo ">>> Done."

