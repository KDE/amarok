#!/usr/bin/env ruby
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

require 'Qt'
require "#{File.dirname( File.expand_path(__FILE__))}/amarok.rb"

#a simple method to call DCOP. If you use Korundum, you can use
#dR = KDE::DCOPRef.new("amarok", "player"); dR.artist instead
def dcop (val1, val2 = String.new)
	str =`dcop amarok player #{val1} #{val2}`
	str.chomp! if str != nil
	return str
end

#you can remove methods you don't need, they're all here just so that you can see

class ExampleActions < AmaroKSlots
	def initialize(app)
		@app = app
	end
	def configure
		puts "configure"
	end
	def stateChange_empty
		puts "empty"
	end	
	def stateChange_idle
		puts "idle"
	end
	def stateChange_paused
		puts "paused"
	end
	def stateChange_playing
		puts "playing"
	end
	def trackChange
		puts "track changed"
		puts "Now playing #{dcop('title')} by #{dcop('artist')}"
	end
	def exit
		@app.exit
	end
end
a = Qt::Application.new(ARGV)
e = ExampleActions.new(a)
AmarokCommunication = AmaroKLib.new(a, e)
a.exec()