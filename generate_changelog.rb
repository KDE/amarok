#!/usr/bin/env ruby
#
# Script for generating a HTML page from amaroK's text ChangeLog
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GPL V2


$input  = File.new( "ChangeLog",  File::RDONLY )
$output = File.new( "ChangeLog.html", File::CREAT | File::RDWR | File::TRUNC )

$changelog = $input.read
$rx = /BR [0-9]*/

allmatches = $changelog.scan( $rx )


for bug in allmatches
    bugnum = /[0-9].*/.match( bug )
#     puts bugnum

    url = "<a href='http://bugs.kde.org/show_bug.cgi?id=#{bugnum}'>#{bug}</a>"
    $changelog = $changelog.gsub( bug, url )
end


$changelog = $changelog.gsub( /amaroK ChangeLog\n\=*\n/, "<h2>amaroK ChangeLog</h2>" )

# Replace newlines
$changelog = $changelog.gsub( "\n", "</BR>\n" )


puts $changelog
$output.write( $changelog )

