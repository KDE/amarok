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

# Show safety check dialog
`kdialog --warningcontinuecancel "Really create the tag '#{tagname}' NOW in the svn repository?"`
if $?.exitstatus() == 2
    print "Aborted.\n"
    exit()
end

# Create destination folder
`svn mkdir https://svn.kde.org/home/kde/tags/amarok/#{tagname}`

source = "https://svn.kde.org/home/kde/trunk/extragear/multimedia/amarok"
docs   = "https://svn.kde.org/home/kde/trunk/extragear/multimedia/doc/amarok"
target = "https://svn.kde.org/home/kde/tags/amarok/#{tagname}/"

# Copy the files in the repository
`svn cp -m "Tag amaroK." #{source} #{target}`
`svn cp -m "Tag amaroK docs." #{docs} #{target}`


print "Tag created.\n"
