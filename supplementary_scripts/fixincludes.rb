#!/usr/bin/ruby

puts "File: #{$*[0]}"

includes = {}

File.readlines($*[0]).each do |line|
    incre = Regexp.new('#include <((K|Q)[a-zA-z\d]*)>')
    incmatch = incre.match(line)
    if incmatch and not incmatch[1] == "QtDebug" and not incmatch[1] == "KLocale"
        puts "Match: #{incmatch[1]}"
        includes[incmatch[1]] = 0
    else
        incre2 = Regexp.new('#include <.*/((K|Q).*)>')
        incmatch2 = incre2.match(line)
        if incmatch2
            puts "Match2: #{incmatch2[1]}"
            includes[incmatch2[1]] = 0
        else
            includes.each_key do |kmatch|
                linere = Regexp.new(kmatch.to_s)
                linematch = linere.match(line)
                if linematch
                    includes[kmatch] = includes[kmatch] + 1
                end
            end
        end
    end
end

includes.each_key do |key|
    puts "Include #{key} count: #{includes[key].to_s}"
end

#puts "Fixing..."

if not includes.has_value?(0)
    exit
end

puts "Making changes in #{$*}..."
puts

includes.delete_if {|key, value| value > 0}

thefile = File.readlines($*[0])

newfile = Array.new

thefile.each do |line|
    filere = Regexp.new('#include <((K|Q).*)>')
    filematch = filere.match(line)
    if filematch
        if not includes.has_key?(filematch[1])
            newfile.push(line)
        end
    else
        filere = Regexp.new('#include <.*/((K|Q).*)>')
        filematch = filere.match(line)
        if filematch
            if not includes.has_key?(filematch[1])
                newfile.push(line)
            end
        else
            newfile.push(line)
        end
    end
end

if not newfile.last == "\n"
    newfile.push("\n")
end

File.open($*[0], File::CREAT | File::RDWR | File::TRUNC) do |file|
    file.puts newfile
end


