############################################################################
#    Copyright (C) 2005 by Ian Monroe                                      #
#    ian@monroe.nu                                                         #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             #
############################################################################

class QtIoListener < Qt::Object
	slots 'dataRecieved()'
	def initialize(app, iostream, lineHandler)
		super(app)
		@lineHandler = lineHandler
		@iostream = iostream
		streamListener = Qt::SocketNotifier.new(iostream.fileno, Qt::SocketNotifier.Read, app, 'stdinWatcher')
		Qt::Object.connect(streamListener, SIGNAL("activated(int)"), self, SLOT("dataRecieved()") )

	end
	def dataRecieved
		line = @iostream.gets
		line.chomp!
		@lineHandler.call(line)
	end
end

class AmaroKLib
#Either way, AmaroKLib depends of QtRuby by virtue of using the QtIoListener.
#Making a non-Qt alternative to QtIoListener shouldn't be too difficult,
#you'll just have to get your hands dirty with threads and then modify the definition of
#@listen in this class. And send me the class for inclusion in this template. :)
   def initialize(app, commandSlots)
		@listener = QtIoListener.new(app, STDIN, Proc.new { |line|
		#puts "Recieved command #{line}"
		case line
			when 'configure' then commandSlots.configure
			when 'trackChange' then commandSlots.trackChange
			when 'engineStateChange: empty' then commandSlots.stateChange_empty
			when 'engineStateChange: idle' then commandSlots.stateChange_idle
			when 'engineStateChange: paused' then commandSlots.stateChange_paused
			when 'engineStateChange: playing' then commandSlots.stateChange_playing
			#not a command given by Amarok, helpful when debugging
			when 'exit' then commandSlots.exit
			else $stderr.print "Illegal command: #{line}"
		end	
} )
   end
end

#make your own child class of AmaroKSlots to send to AmaroKLib.
class AmaroKSlots
	def configure
		nil
	end
	def stateChange_empty
		nil
	end	
	def stateChange_idle
		nil
	end
	def stateChange_paused
		nil
	end
	def stateChange_playing
		nil
	end
	def trackChange
		nil
	end
	#not a command given by Amarok, used when debugging
	def exit
		nil
	end
end
