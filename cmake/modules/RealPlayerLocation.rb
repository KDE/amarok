#!/usr/bin/env ruby

#prefer realplay
exe = `which hxplay` if( (exe = `which realplay`).size == 0 )
exe.chomp!
begin
    link = File.readlink(exe)
    if link =~ /\.\./ then
        executable =  File.expand_path( File.dirname( exe )+ "/" + link )
    else
        executable = link
    end
rescue File.readlink(exe) #if fails readlink:
    executable = exe
end
puts File.dirname( executable )
