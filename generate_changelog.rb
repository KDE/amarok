#!/usr/bin/env ruby
#
# Script for generating a HTML page from amaroK's text ChangeLog
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GPL V2


$input  = File.new( "ChangeLog",  File::RDONLY )
$changelog = $input.read
$output = File.new( "ChangeLog.html", File::CREAT | File::RDWR | File::TRUNC )

$rx = /BR [0-9]*/

allmatches = $changelog.scan( $rx )


for bug in allmatches
    bugnum = /[0-9].*/.match( bug )
#     puts bugnum

    url = "<a href='http://bugs.kde.org/show_bug.cgi?id=#{bugnum}'>#{bug}</a>"
    $changelog = $changelog.gsub( bug, url )
end

# Replace newlines
$changelog = $changelog.gsub( /\n/, "</BR>" )


puts $changelog
$output.write( $changelog )

