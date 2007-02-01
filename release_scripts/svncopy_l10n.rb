list = `cat subdirs`
root = `pwd`.chomp
for it in Dir['*']
    it.chomp!
   begin
	Dir.chdir(it)
	pwd=`pwd`.chomp
	puts pwd
	for y in ['docmessages', 'docs', 'messages']
		begin
			Dir.chdir("#{y}/extragear-multimedia")
            current = `pwd`.chomp
            puts "now in #{current}"
            stableBranchDir = "/home/ian/work/l10n/stable_branch/#{current.gsub(/^#{root}\//,'').chomp}"
            if not File.exist?(stableBranchDir) then
                puts "oh noes, #{stableBranchDir} doesn't exist"
                `svn mkdir #{stableBranchDir}`
                puts "still doesn't exist" unless File.exist?(stableBranchDir)
            end
			Dir['*amarok*'].each { | file |
                `svn remove #{stableBranchDir}/#{file}`
                `svn copy #{file} #{stableBranchDir}`
            }
			Dir.chdir('../..')
		rescue StandardError
			puts $!
		end
	end
	Dir.chdir('..')
   rescue
	puts $!
   end
end
