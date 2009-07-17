#!/bin/bash

# This will pull the latest version of qtscriptgenerator
# and build it against the first version of QT found
# (either specified by $QTDIR or by qmake)

# Based on the AUR script (http://aur.archlinux.org) by
# deltaecho

# whether to install the qtscript bindings documentation
INSTALL_DOCS="true"
# whether to remove the build directory after installing
# (the downloaded source directory will be kept regardless)
CLEANUP="false"

_gitroot="git://labs.trolltech.com/qtscriptgenerator"
_gitname="qtscriptgenerator-git"

_scriptdir="$(dirname "$0")"

qmake="qmake"
if [ -n "$QTDIR" ]; then
	if [ -x "$QTDIR/bin/qmake" ]; then
		qmake="$QTDIR/bin/qmake"
	fi
fi

# Export the include dir
if [ -z "$QTDIR" ]; then
	export QTDIR="$($qmake -query QT_INSTALL_PREFIX)"
fi
export INCLUDE="$($qmake -query QT_INSTALL_HEADERS)"
PLUGINS_INST="$($qmake -query QT_INSTALL_PLUGINS)"
DOCS_INST="$($qmake -query QT_INSTALL_DOCS)"

if [ -d ${_gitname}/.git ]; then
	echo ">>> Updating GIT working copy..."
	( cd ${_gitname} && git pull ) || echo ">>> WARNING: failed to update GIT working copy"
else
	echo ">>> Pulling qtscriptgenerator from GIT repository..."
	git clone ${_gitroot} ${_gitname} || exit 1
fi

echo ">>> Creating the build directory..."
rm -rf ${_gitname}-build
cp -a ${_gitname} ${_gitname}-build
cd ${_gitname}-build

echo ">>> Patching the generator..."
patch -Np1 -i "${_scriptdir}/qtscriptgenerator-includefix.patch" || exit 1
# this disables phonon and xmlpatterns
patch -Np1 -i "${_scriptdir}/qtscriptgenerator-qtcopy.patch" || exit 1


#  Compile and run the generator
# to "generate" the source and
# header files for the bindings
cd generator
echo ">>> Building the generator..."
qmake && make || exit 1
echo ">>> Generating bindings sources..."
./generator || exit 1
cd ..

echo ">>> Compiling bindings..."
#  Compile the bindings
cd qtbindings
qmake && make || exit 1
cd ..

echo ">>> Installing bindings..."
#  Install the qtscript bindings
mkdir -p "${PLUGINS_INST}/script" || exit 1
cp -a plugins/script/* "${PLUGINS_INST}/script" || exit 1

#  Determine whether to install the documentation
if [ "${INSTALL_DOCS}" == "true" ]; then
	echo ">>> Installing documentation..."
	mkdir -p "${DOCS_INST}/qtscript" || exit 1
	cp -a doc/* "${DOCS_INST}/qtscript" || exit 1
fi

if [ "${CLEANUP}" == "true" ]; then
	echo ">>> Cleaning up build directory..."
	cd ..
	rm -rf ${_gitname}-build
fi

