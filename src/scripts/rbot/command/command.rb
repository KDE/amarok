# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
#
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class CommandPlugin < Plugin
  def initialize()
    super
  end

  def listen( m )
    if m.address? and @registry.has_key?( m.message )
      cmd = m.message
      code = @registry[cmd].untaint

      Thread.start do
        $SAFE = 3  #better safe than sorry
        begin
          eval( code )
        rescue => detail
          m.reply( "Command '#{cmd}' crapped out :(" )
          @bot.say( m.sourcenick, "Backtrace for command '#{cmd}':" )
          @bot.say( m.sourcenick, detail.backtrace.join("\n") )
        end
      end
    end
  end

  def cmd_command_add( m, params )
    cmd = params[:command]
    code = params[:code].join( " " )

    @registry[cmd] = code

    debug "added code: " + code
    m.reply( "done" )
  end

  def cmd_command_list( m, params )
    if @registry.length == 0
      m.reply( "No commands available." )
      return
    end

    txt = ""
    @registry.each_key { |cmd| txt << "#{cmd}, " }
    txt = txt[0, txt.length - 2] #Strip last comma

    m.reply( "Available commands:" )
    m.reply( txt )
  end
end


plugin = CommandPlugin.new
plugin.register( "command" )

plugin.map 'command add :command *code', :action => 'cmd_command_add', :auth => 'commandedit'
plugin.map 'command list', :action => 'cmd_command_list'



