#!/usr/bin/env ruby
#
# Ruby script for generating amaroK tarball releases from KDE SVN
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# Some parts of this code taken from cvs2dist
# License: GNU General Public License V2


branch = "branches/stable"
tag = ""

unless $*.empty?()
    case $*[0]
        when "--branch"
            branch = `kdialog --inputbox "Enter branch name: " "branches/stable"`.chomp()
        when "--tag"
            tag = `kdialog --inputbox "Enter tag name: "`.chomp()
        else
            puts("Unknown option #{$1}. Use --branch or --tag.\n")
    end
end

# Ask user for targeted application version
user = `kdialog --inputbox "Your SVN user:"`.chomp()
protocol = `kdialog --radiolist "Do you use https or svn+ssh?" https https 0 "svn+ssh" "svn+ssh" 1`.chomp()

puts "\n"
puts "**** l10n ****"
puts "\n"

i18nlangs = `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/l10n/subdirs`
Dir.mkdir( "l10n" )
Dir.chdir( "l10n" )
# docs
for lang in i18nlangs
    lang.chomp!()
    Dir.mkdir(lang)
    Dir.chdir(lang)
    for part in ['data', 'messages', 'docmessages', 'docs']
        puts "Copying #{lang}'s #{part} over..  "
        Dir.mkdir(part)
        Dir.chdir(part)
        docdirname = "l10n/#{lang}/#{part}/extragear-multimedia"
        `svn co #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/#{docdirname}`
        print docdirname;
        unless FileTest.exists?( "extragear-multimedia" ) 
            Dir.chdir("..")
            Dir.rmdir(part)
            next
        end
        puts( "done.\n" )
        Dir.chdir("..")
    end
    Dir.chdir("..")
end

puts "\n"