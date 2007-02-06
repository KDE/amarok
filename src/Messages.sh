#! /bin/sh
$EXTRACTRC `find . -name "*.rc" -o -name "*.ui" -o -name "*.kcfg"` > rc.cpp
LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -
o -name \*.cxx -o -name \*.ecpp -o -name \*.C`
if test -n "$LIST"; then
	$XGETTEXT $LIST -o $podir/amarok.pot
fi	
