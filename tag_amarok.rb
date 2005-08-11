#!/usr/bin/env ruby
#
# This script creates a new tag in SVN for amaroK. Upon startup, it asks you for name
# of the tag to create (e.g. "1.3-beta3"). Please note that the script creates the tag
# immediately on the live SVN repository.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


# Ask user for tag name
tagname  = `kdialog --inputbox "Enter tag name (e.g. "1.3-beta3"): "`.chomp()


`kdialog --warningcontinuecancel "Really create the tag '#{tagname}' NOW in the svn repository?"`
if $?.exitstatus() == 2
    print "Aborted.\n"
    exit()
end


`svn mkdir https://svn.kde.org/home/kde/tags/amarok/#{tagname}`

source = "https://svn.kde.org/home/kde/trunk/extragear/multimedia/amarok"
target = "https://svn.kde.org/home/kde/tags/amarok/#{tagname}/"
`svn cp #{source} #{target}`

print "Tag created.\n"
