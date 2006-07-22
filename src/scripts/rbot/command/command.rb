# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
#
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class CommandPlugin < Plugin
  def initialize()
    super

    @registry["commands"] = Hash.new unless @registry.has_key?( "commands" )
    @commands = @registry["commands"]
  end

  def listen( m )
    if m.address? and @commands.has_key?( m.message )
      $SAFE = 4  #better safe than sorry
      eval( m.message )
      $SAFE = 0
    end
  end

  def cmd_command_add( m, params )
    cmd = params[:command]
    code = params[:code]

    @commands[cmd] = code

    debug "added code: " + code 
    m.reply( "done" )
  end
end


plugin = CommandPlugin.new
plugin.register( "command" )

plugin.map 'command add :command :code', :action => 'cmd_command_add', :auth => 'commandedit'


