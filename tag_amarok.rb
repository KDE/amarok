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

# Create folder
`svn mkdir https://svn.kde.org/home/kde/tags/amarok/#{tagname}`

# Copy the code in the repository
source = "https://svn.kde.org/home/kde/trunk/extragear/multimedia/amarok"
target = "https://svn.kde.org/home/kde/tags/amarok/#{tagname}/"
`svn cp -m "Create new tag for amaroK." #{source} #{target}`


print "Tag created.\n"
