#!/usr/bin/env ruby
#
# Ruby script for generating amaroK tarball releases from CVS
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# Some parts of this code taken from cvs2dist
# License: GPL V2


def log( text )
    puts( "#{text}\n" )
end


# version  = `kdialog --inputbox "amaroK version: "`.chomp
# username = `kdialog --inputbox "CVS username: "`.chomp

puts( "\n " )
puts( "Enter amaroK version: " )
version = gets.chomp
puts( "Enter CVS username: " )
username = gets.chomp
puts( "\n " )

name     = "amaroK"
cvsroot  = ":pserver:#{username}@cvs.kde.org:/home/kde"
folder   = "amarok-#{version}"
doi18n   = "yes"
$log      = "/dev/null"


# Prevent using unsermake
oldmake = ENV["UNSERMAKE"]
ENV["UNSERMAKE"] = "no"

# Remove old folder, if exists
`rm -rf #{folder} 2> /dev/null`
`rm -rf folder.tar.bz2 2> /dev/null`

Dir.mkdir( folder )
Dir.chdir( folder )

`cvs -d #{cvsroot} co -l kdeextragear-1`
`cvs -z3 -d #{cvsroot} co kdeextragear-1/amarok`
`cvs -z3 -d #{cvsroot} co -l kdeextragear-1/doc`
`cvs -z3 -d #{cvsroot} co kdeextragear-1/doc/amarok`
Dir.chdir( "kdeextragear-1" )
'cvs -z3 -d #{cvsroot} co admin'

print "\n"
print "**** i18n ****"
print "\n"


# we check out kde-i18n/subdirs in kde-i18n..
if doi18n == "yes"
    log( "cvs co kde-i18n/subdirs" )
    `cvs -z3 -d #{cvsroot} -q co -P kde-i18n/subdirs > /dev/null 2>&1`
    i18nlangs = `cat kde-i18n/subdirs`
    log( "Available languages: #{i18nlangs}" )

    # docs
    for lang in i18nlangs
        lang = lang.chomp
        if FileTest.exists? "doc/#{lang}"
            `rm -Rf doc/#{lang}`
        end
        docdirname = "kde-i18n/#{lang}/docs/kdeextragear-1/amarok"
        log( "cvs co #{docdirname}" )
        `cvs -z3 -q -d "#{cvsroot}" co -P "#{docdirname}" > /dev/null 2>&1`
        if not FileTest.exists? ( docdirname ) then next end
        print "Copying #{lang}'s #{name} documentation over...\n"
        `cp -R #{docdirname} doc/#{lang}`
        log( "#{lang} documentation included." )
    end


    print "\n"
    exit


    Dir.mkdir( "po" )
    for lang in i18nlangs
        lang = lang.chomp
        pofilename = "kde-i18n/#{lang}/messages/kdeextragear-1/amarok.po"
        `echo "cvs co #{pofilename}" >> #{$log}`
        `cvs -z3 -d #{cvsroot} -q co -P "#{pofilename}" > /dev/null 2>&1`
        if not FileTest.exists? pofilename
            `echo "#{lang}'s #{name}.po does not exist." >> #{$log}`
        end

        dest = po/#{lang}
        `mkdir #{dest}`
        print "Copying #{lang}'s #{name}.po over... "
        `echo "#{lang}'s #{name}.po file included." >> #{$log}`
        `cp #{pofilename} #{dest}`
        print "done."

        echo "KDE_LANG = $lang
SUBDIRS = \$(AUTODIRS)
POFILES = AUTO" > $dest/Makefile.am

        subdirs="non_empty"
    end

    if FileTest.exists? subdirs
        `rm -Rf po`
    else
        `echo "SUBDIRS = \$(AUTODIRS)" > po/Makefile.am`
    end

    `rm -rf kde-i18n`
end

print

# Remove CVS relevant files
`find -name "CVS" -exec rm -rf {} \; 2> /dev/null`
`find -name ".cvsignore" -exec rm {} \;`

`pushd amarok`

# Move some important files to the root folder
`mv amarok.lsm ..`
`mv AUTHORS ..`
`mv ChangeLog ..`
`mv COPYING ..`
`mv INSTALL ..`
`mv README ..`
`mv TODO ..`

`pushd src`
`cat amarok.h | sed -e "s/APP_VERSION \".*\"/APP_VERSION \"#{version}"\"/ | tee amarok.h > /dev/null`
`popd`

`rm -rf debian`

`popd` # kdeextragear-1
`print`

`find -name "*" -exec touch {} \;`

`make -f Makefile.cvs`

`rm -rf autom4te.cache`
`rm stamp-h.in`

`mv * ..`
`popd # amaroK-foo`
`rm -rf kdeextragear-1`
`popd # root`
`tar -cf #{folder}.tar #{folder}`
`bzip2 #{folder}.tar`
`rm -rf #{folder}`


ENV["UNSERMAKE"] = oldmake


print "\n"
print "====================================================="
print "Congratulations :) amaroK #{version} tarball generated."
print "Now follow the steps in the RELEASE_HOWTO, from"
print "SECTION 3 onwards."
print "\n"
print "Then drink a few pints and have some fun on #amarok"
print "\n"
print "MD5 checksum: " + `md5sum #{folder}.tar.bz2`
print "\n"
print "\n"



