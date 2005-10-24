command="for x in "
Dir.foreach(".") { |lang|
 for part in ['docmessages','docs', 'messages'] 
	if(File.exists?("#{lang}/#{part}/extragear-multimedia/"))
		command="#{command} #{lang}/#{part}/extragear-multimedia/"
	end
 end
}
command=command+"; do svn ci -m 'Copying over the po and doc files to the stable amaroK branch' $x; "
command=command+"done;"
puts command
