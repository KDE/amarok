#!/usr/bin/env ruby
#
# Ruby script for generating Amarok tarball releases from KDE SVN
#
# (C) 2006-2007 Harald Sitter <sitter.harald@gmail.com>
# (C) 2005 Mark Kretschmann <kretschmann@kde.org>
# Some parts of this code taken from cvs2dist
# License: GNU General Public License V2

require 'fileutils'

tag             = nil
useStableBranch = false

# Ask whether using branch or trunk
location = `kdialog --combobox "Select checkout's place:" "Trunk" "Stable" "Tag"`.chomp()
if location == "Tag"
  tag = `kdialog --inputbox "Enter tag name: "`.chomp()
elsif location == "Stable"
  useStableBranch = true
end

# Ask user for targeted application version
if tag and not tag.empty?()
  version = tag
else
  version  = `kdialog --inputbox "Enter Amarok version: "`.chomp()
end

user     = `kdialog --inputbox "Your SVN user:"`.chomp()
protocol = `kdialog --radiolist "Do you use https or svn+ssh?" https https 0 "svn+ssh" "svn+ssh" 1`.chomp()

name         = "amarok"
folder       = "amarok-#{version}"
# l10n_branch  = true


# Prevent using unsermake
oldmake = ENV["UNSERMAKE"]
ENV["UNSERMAKE"] = "no"


# Remove old folder, if exists
FileUtils.rm_rf( folder )
FileUtils.rm_rf( "#{folder}.tar.bz2" )

Dir.mkdir( folder )
Dir.chdir( folder )

branch="trunk"

if useStableBranch
  branch = "branches/stable/"
  `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/extragear/multimedia/`
  Dir.chdir( "multimedia" )
elsif not tag.empty?()
  l10n = `kdialog --combobox "Get translation from:" "Trunk" "Stable" "Not at all"`.chomp()
  if l10n == "Trunk"
    l10n_branch = "trunk"
  elsif l10n == "Stable"
    l10n_branch = "branches/stable/"
  else
    l10n_branch = nil
  end
  `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/extragear/multimedia`
  Dir.chdir( "multimedia" )
else
  `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/trunk/extragear/multimedia`
  Dir.chdir( "multimedia" )
end

`svn up amarok`
`svn up -N doc`
`svn up doc/amarok`
`svn co #{protocol}://#{user}@svn.kde.org/home/kde/branches/KDE/3.5/kde-common/admin`


unless l10n_branch == nil
  unless l10n_branch == true
    branch = l10n_branch
  end
  puts "\n"
  puts "**** l10n ****"
  puts "\n"

  i18nlangs = `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/l10n/subdirs`
  Dir.mkdir( "l10n" )
  Dir.chdir( "l10n" )

  # docs
  for lang in i18nlangs
    lang.chomp!()
    FileUtils.rm_rf( "../doc/#{lang}" )
    FileUtils.rm_rf( name )
    docdirname = "l10n/#{lang}/docs/extragear-multimedia/amarok"
    `svn co -q #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/#{docdirname} > /dev/null 2>&1`
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
    `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/#{pofilename} 2> /dev/null | tee l10n/amarok.po`
    next if FileTest.size( "l10n/amarok.po" ) == 0

    dest = "po/#{lang}"
    Dir.mkdir( dest )
    print "Copying #{lang}'s #{name}.po over ..  "
    FileUtils.mv( "l10n/amarok.po", dest )
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
    # Remove xx language
    FileUtils.rm_rf( "po/xx" )
  else
    FileUtils.rm_rf( "po" )
  end

  FileUtils.rm_rf( "l10n" )
end

puts "\n"


# Remove SVN data folder
`find -name ".svn" | xargs rm -rf`

# TODO: what is this supposed to do, why did we include it, and does the script
#       actually work without it?
# if useStableBranch
#     `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/trunk/extragear/multimedia`
#     `mv multimedia/* .`
#     FileUtils.rm_rf( "multimedia" )
# end

Dir.chdir( "amarok" )

# Move some important files to the root folder
FileUtils.mv( "AUTHORS", ".." )
FileUtils.mv( "ChangeLog", ".." )
FileUtils.mv( "COPYING", ".." )
FileUtils.mv( "COPYING.LIB", ".." )
FileUtils.mv( "COPYING-DOCS", ".." )
FileUtils.mv( "INSTALL", ".." )
FileUtils.mv( "README", ".." )
FileUtils.mv( "TODO", ".." )

# This stuff doesn't belong in the tarball
FileUtils.rm_rf( "release_scripts" )
FileUtils.rm_rf( "src/engine/gst10" ) #Removed for now
FileUtils.rm_rf( "supplementary_scripts" )

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

FileUtils.rm_rf( "autom4te.cache" )
FileUtils.rm_rf( "stamp-h.in" )


puts "**** Compressing..  "
`mv * ..`
Dir.chdir( ".." ) # Amarok-foo
FileUtils.rm_rf( "multimedia" )
Dir.chdir( ".." ) # root folder
`tar -cf #{folder}.tar #{folder}`
`bzip2 #{folder}.tar`
FileUtils.rm_rf( folder )
puts "done.\n"


ENV["UNSERMAKE"] = oldmake


puts "\n"
puts "====================================================="
puts "Congratulations :) Amarok #{version} tarball generated.\n"
puts "Now follow the steps in the RELEASE_HOWTO, from\n"
puts "SECTION 3 onwards.\n"
puts "\n"
puts "Then drink a few pints and have some fun on #amarok\n"
puts "\n"
puts "MD5 checksum: " + `md5sum #{folder}.tar.bz2`
puts "\n"
puts "\n"
