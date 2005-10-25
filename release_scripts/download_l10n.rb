list = `cat subdirs`
for it in list
    it.chomp!
   `svn up -N #{it}`
   begin
	Dir.chdir(it)
	pwd=`pwd #{it}`
	print  pwd + it
	for y in ['docmessages', 'docs', 'messages']
		begin
			`svn up -N #{y}`
			Dir.chdir(y)
			`svn up extragear-multimedia`
			Dir.chdir('..')
		rescue StandardError
			puts $!
		end
	end
	Dir.chdir('..')
   rescue
	puts $!
   end
end
