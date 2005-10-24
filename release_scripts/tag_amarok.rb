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
user = `kdialog --inputbox "Your SVN user:"`.chomp()
protocol = `kdialog --radiolist "Do you use https or svn+ssh?" https https 0 "svn+ssh" "svn+ssh" 1`.chomp()

source = "#{protocol}://#{user}@svn.kde.org/home/kde/trunk/extragear/multimedia"

`svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/tags/amarok/`
Dir.chdir('amarok')
`svn mkdir #{tagname}`
`svn mkdir #{tagname}/doc`
`svn cp #{source}/amarok #{tagname}/amarok`
`svn cp #{source}/doc/amarok #{tagname}/doc`

print "Now commit tag #{tagname} with svn ci."
