#!/usr/bin/env ruby
#
# Script for generating a HTML page from Amarok's text ChangeLog
#
# (c) 2005-2006,2009
#    Mark Kretschmann <kretschmann@kde.org>,
#    Ian Monroe <ian@monroe.nu>,
#    Magnus Bergmark <magnus.bergmark@gmail.com>
# License: GPL V2

require 'cgi'

$input  = File.new( "ChangeLog",  File::RDONLY )
$output = File.new( "ChangeLog.html", File::CREAT | File::RDWR | File::TRUNC )

$changelog = CGI::escapeHTML($input.read)

# Remove the vim stuff
$changelog.gsub!(/# vim:.*$/, '')

# Remove whitespace from lines with only whitespace
$changelog.gsub!(/^\s+$/, '')

# Collapse multiple empty lines
$changelog.gsub!(/\n{2,}/, "\n")

# Replace bug number with direct link to bugs.kde.org
$changelog.gsub!(/BR (\d+)/, '<a href="https://bugs.kde.org/show_bug.cgi?id=\\1">\\0</a>')

# Make bullets
bullet_item_regexp =
  /
    ^\s{3,4}           # Start of line and three to four whitespace
    \*\s               # The actual bullet
    (
      [^\n]*           # Match everything up to the first newline
      (\s{6}[^\n]+)*   # Match every following line if its indented
    )
  /xm;

$changelog.gsub!(bullet_item_regexp) do |match|
  # Remove all the indentation spaces, too
  "\t<li>#{$1.gsub(/\s{2,}/, ' ')}</li>"
end

# Beautify heading
$changelog.gsub!(/Amarok ChangeLog\n\=*\n/, "<h1>Amarok ChangeLog</h1>\n")

# Create headers
%w{FEATURES CHANGES BUGFIXES}.each do |header|
  $changelog.gsub!("#{header}:\n", "<h3>#{header.capitalize}</h3>")
end

# Format version headers
$changelog.gsub!(/VERSION(.*$)/, '<h2>Version\\1</h2>');

# Create <ul> and </ul> pairs around the headers
# Lists start after a <h3></h3> and ends with the first other headline
$changelog.gsub!(%r{(</h3>)(.*?)(<h\d>)}m, "\\1\n<ul>\n\\2\n</ul>\n\n\\3")

# Now, we must parse the old syntax. It'll begin with version 1.2.1
old_syntax_marker = "<h2>Version 1.2.1:</h2>"
$changelog.sub!(/(#{old_syntax_marker})(.*)/m) do |old_contents|
  # The syntax here is pretty similar to the old lists, but we don't have any nice
  # bullet points in the file already. We will instead look at indentation
  bullet_item_regexp =
    /
      ^\s{2}             # Start of line and two whitespace
      (
        [^\n]*           # Match everything up to the first newline
        (\s{4}[^\n]+)*   # Match every following line if its indented
      )
    /xm;

  old_contents.gsub!(bullet_item_regexp) do |match|
    # Remove all the indentation spaces, too
    "\t<li>#{$1.gsub(/\s{2,}/, ' ')}</li>"
  end

  # We also need to place <ul> and </ul> pairs everywhere
  # Lists start after a <h3></h3> and ends with the first other headline or EOF
  old_contents.gsub!(%r{(</h2>)(.*?)(<h\d>|\Z)}m, "\\1\n<ul>\n\\2\n</ul>\n\n\\3")

  old_contents
end

# Write destination file
$output.write( $changelog )

