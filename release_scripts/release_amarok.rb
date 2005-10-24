#!/usr/bin/env ruby
#
# Ruby script for generating amaroK tarball releases from KDE SVN
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# Some parts of this code taken from cvs2dist
# License: GNU General Public License V2


branch = ""
tag = ""

unless $*.empty?()
    case $*[0]
        when "--branch"
            branch = `kdialog --inputbox "Enter branch name: "`.chomp()
        when "--tag"
            tag = `kdialog --inputbox "Enter tag name: "`.chomp()
        else
            puts("Unknown option #{$1}. Use --branch or --tag.\n")
    end
end

# Ask user for targeted application version
version  = `kdialog --inputbox "Enter amaroK version: "`.chomp()
user = `kdialog --inputbox "Your SVN user:"`.chomp()
protocol = `kdialog --radiolist "Do you use https or svn+ssh?" https https 0 "svn+ssh" "svn+ssh" 1`.chomp()

name     = "amarok"
folder   = "amarok-#{version}"
do_l10n  = "yes"


# Prevent using unsermake
oldmake = ENV["UNSERMAKE"]
ENV["UNSERMAKE"] = "no"

# Remove old folder, if exists
`rm -rf #{folder} 2> /dev/null`
`rm -rf folder.tar.bz2 2> /dev/null`

Dir.mkdir( folder )
Dir.chdir( folder )


if not branch.empty?()
    `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/branches/amarok/#{branch}`
elsif not tag.empty?()
    `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/tags/amarok/#{tag}`
else
    `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/trunk/extragear/multimedia`
end

Dir.chdir( "multimedia" )
`svn up amarok`
`svn up -N doc`
`svn up doc/amarok`
`svn co #{protocol}://#{user}@svn.kde.org/home/kde/branches/KDE/3.5/kde-common/admin`


if do_l10n == "yes"
    puts "\n"
    puts "**** l10n ****"
    puts "\n"

    i18nlangs = `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/trunk/l10n/subdirs`
    Dir.mkdir( "l10n" )
    Dir.chdir( "l10n" )

    # docs
    for lang in i18nlangs
        lang.chomp!()
        `rm -Rf ../doc/#{lang}`
        `rm -rf amarok`
        docdirname = "l10n/#{lang}/docs/extragear-multimedia/amarok"
        `svn co -q #{protocol}://#{user}@svn.kde.org/home/kde/trunk/#{docdirname} > /dev/null 2>&1`
        next unless FileTest.exists?( "amarok" )
        print "Copying #{lang}'s #{name} documentation over..  "
        `cp -R amarok/ ../doc/#{lang}`

        # we don't want KDE_DOCS = AUTO, cause that makes the
        # build system assume that the name of the app is the
        # same as the name of the dir the Makefile.am is in.
        # Instead, we explicitly pass the name..
        makefile = File.new( "../doc/#{lang}/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
        makefile << "KDE_LANG = #{lang}\n"
        makefile << "KDE_DOCS = #{name}\n"
        makefile.close()

        puts( "done.\n" )
    end

    Dir.chdir( ".." ) # multimedia
    puts "\n"

    $subdirs = false
    Dir.mkdir( "po" )

    for lang in i18nlangs
        lang.chomp!()
        pofilename = "l10n/#{lang}/messages/extragear-multimedia/amarok.po"
        `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/trunk/#{pofilename} 2> /dev/null | tee l10n/amarok.po`
        next if FileTest.size( "l10n/amarok.po" ) == 0

        dest = "po/#{lang}"
        Dir.mkdir( dest )
        print "Copying #{lang}'s #{name}.po over ..  "
        `mv l10n/amarok.po #{dest}`
        puts( "done.\n" )

        makefile = File.new( "#{dest}/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
        makefile << "KDE_LANG = #{lang}\n"
        makefile << "SUBDIRS  = $(AUTODIRS)\n"
        makefile << "POFILES  = AUTO\n"
        makefile.close()

        $subdirs = true
    end

    if $subdirs
        makefile = File.new( "po/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
        makefile << "SUBDIRS = $(AUTODIRS)\n"
        makefile.close()
    else
        `rm -Rf po`
    end

    `rm -rf l10n`
end

puts "\n"


# Remove SVN data folder
`find -name ".svn" | xargs rm -rf`

Dir.chdir( "amarok" )

# Move some important files to the root folder
`mv amarok.lsm ..`
`mv AUTHORS ..`
`mv ChangeLog ..`
`mv COPYING ..`
`mv INSTALL ..`
`mv README ..`
`mv TODO ..`

Dir.chdir( "src" )

# Exchange APP_VERSION string with targeted version
file = File.new( "amarok.h", File::RDWR )
str = file.read()
file.rewind()
file.truncate( 0 )
str.sub!( /APP_VERSION \".*\"/, "APP_VERSION \"#{version}\"" )
file << str
file.close()


Dir.chdir( ".." ) # amarok
Dir.chdir( ".." ) # multimedia
puts( "\n" )

`find | xargs touch`


puts "**** Generating Makefiles..  "
`make -f Makefile.cvs`
puts "done.\n"

`rm -rf autom4te.cache`
`rm stamp-h.in`


puts "**** Compressing..  "
`mv * ..`
Dir.chdir( ".." ) # amaroK-foo
`rm -rf multimedia`
Dir.chdir( ".." ) # root folder
`tar -cf #{folder}.tar #{folder}`
`bzip2 #{folder}.tar`
`rm -rf #{folder}`
puts "done.\n"


ENV["UNSERMAKE"] = oldmake


puts "\n"
puts "====================================================="
puts "Congratulations :) amaroK #{version} tarball generated.\n"
puts "Now follow the steps in the RELEASE_HOWTO, from\n"
puts "SECTION 3 onwards.\n"
puts "\n"
puts "Then drink a few pints and have some fun on #amarok\n"
puts "\n"
puts "MD5 checksum: " + `md5sum #{folder}.tar.bz2`
puts "\n"
puts "\n"


