# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
#
# Create mini plugins on IRC.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class CommandPlugin < Plugin
  def initialize
    super
    if @registry.has_key?(:commands)
      @commands = @registry[:commands]
    else
      @commands = Hash.new
    end
  end

  def save
    @registry[:commands] = @commands
  end

  def help( plugin, topic="" )
    "Create mini plugins on IRC. 'command add <name> <code>' => Create command named <name> with the code <source>. 'command list' => Show a list of all known commands. 'command show <name>' => Show the source code for <name>. 'command del <name>' => Delete command <name>."
  end

  def listen( m )
    cmd = m.message.split.first

    if m.address? and @commands.has_key?( cmd )
      code = @commands[cmd].dup.untaint

      Thread.start {
        $SAFE = 3
        begin
          eval( code )
        rescue => detail
          m.reply( "Command '#{cmd}' crapped out :(" )
          @bot.say( m.sourcenick, "Backtrace for command '#{cmd}':" )
          @bot.say( m.sourcenick, detail.backtrace.join("\n") )
        end
      }
    end
  end

  def cmd_command_add( m, params )
    cmd = params[:command]
    code = params[:code].join( " " )

    @commands[cmd] = code

    debug "added code: " + code
    m.reply( "done" )
  end

  def cmd_command_del( m, params )
    cmd = params[:command]
    unless @commands.has_key?( cmd )
      m.reply( "Command does not exist." )
      return
    end

    @commands.delete( cmd )
    m.reply( "done" )
  end

  def cmd_command_list( m, params )
    if @commands.length == 0
      m.reply( "No commands available." )
      return
    end

    m.reply "Available commands: #{@commands.keys.sort.join(', ')}" 
  end

  def cmd_command_show( m, params )
    cmd = params[:command]
    unless @commands.has_key?( cmd )
      m.reply( "Command does not exist." )
      return
    end

    m.reply( "Source code for command '#{cmd}':" )
    m.reply( @commands[cmd] )
  end
end


plugin = CommandPlugin.new
plugin.register( "command" )

plugin.map 'command add :command *code', :action => 'cmd_command_add', :auth => 'commandedit'
plugin.map 'command del :command', :action => 'cmd_command_del', :auth => 'commandedit'
plugin.map 'command list', :action => 'cmd_command_list'
plugin.map 'command show :command', :action => 'cmd_command_show'


