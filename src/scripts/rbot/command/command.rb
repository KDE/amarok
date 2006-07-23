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
      code = @commands[m.message].untaint

      $SAFE = 3  #better safe than sorry
      eval( code )
      $SAFE = 0
    end
  end

  def cmd_command_add( m, params )
    cmd = params[:command]
    code = params[:code].join( " " )

    @commands[cmd] = code

    debug "added code: " + code 
    m.reply( "done" )
  end

  def cmd_command_list( m, params )
    if @commands.empty?
      m.reply( "No commands available." )
      return
    end
      
    txt = ""
    @commands.each_key { |cmd| txt << "#{cmd}, " }
    txt = txt[0, txt.length - 2] #Strip last comma

    m.reply( "Available commands:" )
    m.reply( txt )
  end
end
 

plugin = CommandPlugin.new
plugin.register( "command" )

plugin.map 'command add :command *code', :action => 'cmd_command_add', :auth => 'commandedit'
plugin.map 'command list', :action => 'cmd_command_list'



