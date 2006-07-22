# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
#
# Provides a key/value database for storing information on IRC.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class CalcPlugin < Plugin
  def initialize()
    super
  end

def privmsg( m )
end


plugin = QuizPlugin.new
plugin.register( "calc" )


