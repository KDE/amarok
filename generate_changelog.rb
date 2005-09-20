#!/usr/bin/env ruby
#
# Script for generating a HTML page from amaroK's text ChangeLog
#
# (c) 2005 Mark Kretschmann <markey@web.de>, Ian Monroe <ian@monroe.nu>
# License: GPL V2


$input  = File.new( "ChangeLog",  File::RDONLY )
$output = File.new( "ChangeLog.html", File::CREAT | File::RDWR | File::TRUNC )

$changelog = $input.read


# Replace bug number with direct link to bugs.kde.org
allmatches = $changelog.scan( /BR [0-9]*/ )
for bug in allmatches
    bugnum = /[0-9].*/.match( bug )
    url = "<a href='http://bugs.kde.org/show_bug.cgi?id=#{bugnum}'>#{bug}</a>"
    $changelog.gsub!( bug, url )
end

# Make bullets
newOldArray = $changelog.split("VERSION 1.2.1:")
a=newOldArray[0].split('*')
a.shift
a.each{|s|
    $changelog.sub!("*#{s}","<li>#{s}</li>")
}

# Beautify heading
$changelog.gsub!( /amaroK ChangeLog\n\=*\n/, "<h2>amaroK ChangeLog</h2>" )

# Makes an extra </ul>... meh
['FEATURES','CHANGES','BUGFIXES'].each { |header|
    $changelog.gsub!( "#{header}:\n", "</ul><h4>#{header.capitalize!}</h4><ul>" )
}
$changelog.gsub!("VERSION 1.2.1:", "</ul>VERSION 1.2.1:" )

# Replace newlines in old format of changelog
$changelog.sub!(newOldArray[1],newOldArray[1].gsub( /\n/, "</br>\n" ))


# Format version headers
$changelog.gsub!(/(VERSION)(.*$)/, '</ul><h3>Version\2</h3>');
# Write destination file
$output.write( $changelog )

