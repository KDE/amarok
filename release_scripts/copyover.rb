list = `cat subdirs`
for it in list
	it.chomp!
	puts "#{it} begins..."
	begin
		Dir.chdir("/tmp/work/trunk/l10n/#{it}/")
	rescue StandardError
		puts $!
	end
	for y in ['docmessages', 'docs', 'messages']
		begin
			Dir.chdir(y)
			Dir.chdir("extragear-multimedia")
			Dir.new('.').each { |file|
				if file =~ /amarok/
					if not File.exists?("/tmp/l10n/#{it}/#{y}/extragear-multimedia/#{file}")
						if not File.exists?("/tmp/l10n/#{it}/#{y}/extragear-multimedia/")
							`svn mkdir /tmp/l10n/#{it}/#{y}/extragear-multimedia/`
						end
						print "copying #{it}/#{y}/#{file}... "
						`svn copy #{file} /tmp/l10n/#{it}/#{y}/extragear-multimedia/#{file}/`
						puts "done"					
					else 
						puts "#{it}/#{y}/#{file} is already there."
					end
				end
			}
			Dir.chdir("..")
			Dir.chdir("..")
		rescue StandardError
			puts $!
		end
	end
end