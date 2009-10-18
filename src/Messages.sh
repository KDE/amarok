#! /bin/sh
$EXTRACTRC `find . -name "*.rc" -o -name "*.ui" -o -name "*.kcfg"` >> rc.cpp
$EXTRACTATTR --attr=layout,name data/DefaultPlaylistLayouts.xml >> rc.cpp
LIST=`find . -name \*.h -o -name \*.cpp`
if test -n "$LIST"; then
	$XGETTEXT $LIST -o $podir/amarok.pot
fi
rm -f rc.cpp
