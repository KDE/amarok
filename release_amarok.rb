#!/usr/bin/env ruby
#
# Ruby script for generating amaroK tarball releases from CVS
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# Some parts of this code taken from cvs2dist
# License: GPL V2


version = `kdialog --inputbox "amaroK version: "`
username = `kdialog --inputbox "CVS username: "`

cvsroot = ":pserver:$username@cvs.kde.org:/home/kde"
folder = "amarok-$version"
doi18n = "yes"
log = "/dev/null"

# Prevent using unsermake
oldmake = $UNSERMAKE
`export UNSERMAKE=no`

# Remove old folder, if exists
`rm -rf $folder 2> /dev/null`
`rm -rf $folder.tar.bz2 2> /dev/null`

`mkdir $folder`
`pushd $folder`
`cvs -d $cvsroot co -l kdeextragear-1`
`cvs -z3 -d $cvsroot co kdeextragear-1/amarok`
`cvs -z3 -d $cvsroot co -l kdeextragear-1/doc`
`cvs -z3 -d $cvsroot co kdeextragear-1/doc/amarok`
`pushd kdeextragear-1`

'cvs -z3 -d $cvsroot co admin'

print
print "**** i18n ****"
print

# we check out kde-i18n/subdirs in kde-i18n..
if $doi18n = "yes"
    print "cvs co kde-i18n/subdirs" >> $log
    cvs -z3 -d $cvsroot -q co -P kde-i18n/subdirs > /dev/null 2>&1
    i18nlangs="$(cat kde-i18n/subdirs)"
    echo "available languages: $i18nlangs" >> $log

    # docs
    for lang in $i18nlangs; do
        test -d doc/$lang && rm -Rf doc/$lang
        docdirname="kde-i18n/$lang/docs/kdeextragear-1/amarok"
        echo "cvs co $docdirname" >> $log
        cvs -z3 -q -d "$cvsroot" co $branch -P "$docdirname" > /dev/null 2>&1
        if [ ! -d "$docdirname" ]; then
            echo "$lang's $name documentation does not exist." >> $log
            continue
        end
        echo "Copying $lang's $name documentation over... "
        cp -R $docdirname doc/$lang
        cat doc/$lang/Makefile.am | sed -e "s/AUTO/amarok/" > doc/$lang/Makefile.am.nw
        mv doc/$lang/Makefile.am.nw doc/$lang/Makefile.am
        echo "$lang documentation included." >> $log
    end

    print

    mkdir po
    for lang in $i18nlangs; do
        pofilename="kde-i18n/$lang/messages/kdeextragear-1/amarok.po";
        echo "cvs co $pofilename" >> $log
        cvs -z3 -d $cvsroot -q co -P "$pofilename" > /dev/null 2>&1
        if [ ! -f "$pofilename" ]; then
            echo "$lang's $name.po does not exist." >> $log
            continue
        fi

        dest=po/$lang
        mkdir $dest
        echo -n "Copying $lang's $name.po over... "
        echo "$lang's $name.po file included." >> $log
        cp $pofilename $dest
        echo "done."

        echo "KDE_LANG = $lang
SUBDIRS = \$(AUTODIRS)
POFILES = AUTO" > $dest/Makefile.am

        subdirs="non_empty"
    done

    if [ -z "$subdirs" ]; then
        rm -Rf po
    else
        echo "SUBDIRS = \$(AUTODIRS)" > po/Makefile.am
    fi

    rm -rf kde-i18n
end

