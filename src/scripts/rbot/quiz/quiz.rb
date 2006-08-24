# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
#
# A trivia quiz game. Fast paced, featureful and fun.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# (c) 2006 Jocke Andersson <ajocke@gmail.com>
# (c) 2006 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>
# Licensed under GPL V2.


# Class for storing question/answer pairs
QuizBundle = Struct.new( "QuizBundle", :question, :answer )

# Class for storing player stats
PlayerStats = Struct.new( "PlayerStats", :score, :jokers, :jokers_time )
# Why do we still need jokers_time? //Firetech

# Maximum number of jokers a player can gain
Max_Jokers = 3

# Control codes
Color = "\003"
Bold = "\002"


#######################################################################
# CLASS Quiz
# One Quiz instance per channel, contains channel specific data
#######################################################################
class Quiz
  attr_accessor :registry, :registry_conf, :questions, :question, :answer, :answer_core,
  :first_try, :hint, :hintrange, :rank_table, :hinted

  def initialize( channel, registry )
    @registry = registry.sub_registry( channel )
    @registry_conf = @registry.sub_registry( "config" )

    # Per-channel copy of the global questions table. Acts like a shuffled queue
    # from which questions are taken, until empty. Then we refill it with questions
    # from the global table.
    @registry_conf["questions"] = [] unless @registry_conf.has_key?( "questions" )

    # Autoask defaults to true
    @registry_conf["autoask"] = true unless @registry_conf.has_key?( "autoask" )

    @questions = @registry_conf["questions"]
    @question = nil
    @answer = nil
    @answer_core = nil
    @first_try = false
    @hint = nil
    @hintrange = nil
    @hinted = false

    # We keep this array of player stats for performance reasons. It's sorted by score
    # and always synced with the registry player stats hash. This way we can do fast
    # rank lookups, without extra sorting.
    @rank_table = @registry.to_a.sort { |a,b| b[1].score<=>a[1].score }

    # # Convert old PlayerStats to new. Can be removed later on
    # @registry.each_key do |player|
    #     begin
    #         j = @registry[player].joker
    #     rescue
    #         @registry[player] = PlayerStats.new( @registry[player].score, 0, 0 )
    #     end
    # end
  end
end


#######################################################################
# CLASS QuizPlugin
#######################################################################
class QuizPlugin < Plugin
  def initialize()
    super

    @questions = Array.new
    @quizzes = Hash.new
  end

  # Function that returns whether a char is a "separator", used for hints
  #
  def is_sep( ch )
    r = case ch
    when " " then true
    when "." then true
    when "," then true
    when "-" then true
    when "'" then true
    when "&" then true
    when "\"" then true
    else false
    end

    return r
  end


  # Fetches questions from a file on the server and from the wiki, then merges
  # and transforms the questions and fills the global question table.
  #
  def fetch_data( m )
    # TODO: Make this configurable, and add support for more than one file (there's a size limit in linux too ;) )
    path = "#{@bot.botclass}/quiz/quiz.rbot"
    debug "Fetching from #{path}"

    m.reply "Fetching questions from local database and wiki.."

    # Local data
    begin
      datafile = File.new( path, File::RDONLY )
      localdata = datafile.read
    rescue
      m.reply "Failed to read local database file. oioi."
      localdata = nil
    end

    # Wiki data
    begin
      serverdata = @bot.httputil.get( URI.parse( "http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz" ) )
      serverdata = serverdata.split( "QUIZ DATA START\n" )[1]
      serverdata = serverdata.split( "\nQUIZ DATA END" )[0]
      serverdata = serverdata.gsub( /&nbsp;/, " " ).gsub( /&amp;/, "&" ).gsub( /&quot;/, "\"" )
    rescue
      m.reply "Failed to download wiki questions. oioi."
      if localdata == nil
        m.reply "No questions loaded, aborting."
        return
      end
    end

    @questions = []

    # Fuse together and remove comments, then split
    data = "#{localdata}\n\n#{serverdata}".gsub( /^#.*$/, "" )
    entries = data.split( "\nQuestion: " )
    #First entry will be empty.
    entries.delete_at(0)

    entries.each do |e|
      p = e.split( "\n" )
      # We'll need at least two lines of data
      unless p.size < 2
        # Check if question isn't empty
        if p[0].length > 0
          while p[1].match( /^Answer: (.*)$/ ) == nil and p.size > 2
            # Delete all lines between the question and the answer
            p.delete_at(1)
          end
          p[1] = p[1].gsub( /Answer: /, "" ).strip
          # If the answer was found
          if p[1].length > 0
            # Add the data to the array
            b = QuizBundle.new( p[0], p[1] )
            @questions << b
          end
        end
      end
    end

    m.reply "done, #{@questions.length} questions loaded."
  end


  # Returns new Quiz instance for channel, or existing one
  #
  def create_quiz( channel )
    unless @quizzes.has_key?( channel )
      @quizzes[channel] = Quiz.new( channel, @registry )
    end

    return @quizzes[channel]
  end


  def say_score( m, nick )
    q = create_quiz( m.target.to_s )

    if q.registry.has_key?( nick )
      score = q.registry[nick].score
      jokers = q.registry[nick].jokers

      rank = 0
      q.rank_table.each_index { |rank| break if nick == q.rank_table[rank][0] }
      rank += 1

      m.reply "#{nick}'s score is: #{score}    Rank: #{rank}    Jokers: #{jokers}"
    else
      m.reply "#{nick} does not have a score yet. Lamer."
    end
  end


  def help( plugin, topic="" )
    if topic == "admin"
      "Quiz game aministration commands (requires authentication): 'quiz autoask <on/off>' => enable/disable autoask mode. 'quiz transfer <source> <dest> [score] [jokers]' => transfer [score] points and [jokers] jokers from <source> to <dest> (default is entire score and all jokers). 'quiz setscore <player> <score>' => set <player>'s score to <score>. 'quiz setjokers <player> <jokers>' => set <player>'s number of jokers to <jokers>. 'quiz deleteplayer <player>' => delete one player from the rank table (only works when score and jokers are set to 0)."
    else
      "A multiplayer trivia quiz. 'quiz' => ask a question. 'quiz hint' => get a hint. 'quiz solve' => solve this question. 'quiz skip' => skip to next question. 'quiz joker' => draw a joker to win this round. 'quiz score [player]' => show score for [player] (default is yourself). 'quiz top5' => show top 5 players. 'quiz top <number>' => show top <number> players (max 50). 'quiz stats' => show some statistics. 'quiz fetch' => refetch questions from databases.\nYou can add new questions at http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz"
    end
  end


  # Updates the per-channel rank table, which is kept for performance reasons
  #
  def calculate_ranks( m, q, nick )
    if q.registry.has_key?( nick )
      stats = q.registry[nick]

      # Find player in table
      i = 0
      q.rank_table.each_index do |i|
        break if nick == q.rank_table[i][0]
      end

      old_rank = i
      q.rank_table.delete_at( i )

      # Insert player at new position
      inserted = false
      q.rank_table.each_index do |i|
        if stats.score >= q.rank_table[i][1].score
          q.rank_table[i,0] = [[nick, stats]]
          inserted = true
          break
        end
      end

      # If less than all other players' scores, append at the end
      unless inserted
        q.rank_table << [nick, stats]
        i += 1
      end

      if i < old_rank
        m.reply "#{nick} ascends to rank #{i + 1}. Congratulations :)"
      elsif i > old_rank
        m.reply "#{nick} slides down to rank #{i + 1}. So Sorry! NOT. :p"
      end
    else
      q.rank_table << [[nick, PlayerStats.new( 1 )]]
    end
    debug q.rank_table.inspect
  end


  # Reimplemented from Plugin
  #
  def listen( m )
    return unless @quizzes.has_key?( m.target.to_s )
    q = @quizzes[m.target.to_s]

    return if q.question == nil

    message = m.message.downcase.strip

    if message == q.answer.downcase or message == q.answer_core.downcase
      replies = []

      points = 1
      if q.first_try
        points += 1
        replies << "WHOPEEE! #{m.sourcenick.to_s} got it on the first try! That's worth an extra point. Answer was: #{q.answer}"
      elsif q.rank_table.length >= 1 and m.sourcenick.to_s == q.rank_table[0][0]
        replies << "THE QUIZ CHAMPION defends his throne! Seems like #{m.sourcenick.to_s} is invicible! Answer was: #{q.answer}"
      elsif q.rank_table.length >= 2 and m.sourcenick.to_s == q.rank_table[1][0]
        replies << "THE SECOND CHAMPION is on the way up! Hurry up #{m.sourcenick.to_s}, you only need #{q.rank_table[0][1].score - q.rank_table[1][1].score - 1} points to beat the king! Answer was: #{q.answer}"
      elsif    q.rank_table.length >= 3 and m.sourcenick.to_s == q.rank_table[2][0]
        replies << "THE THIRD CHAMPION strikes again! Give it all #{m.sourcenick.to_s}, with #{q.rank_table[1][1].score - q.rank_table[2][1].score - 1} more points you'll reach the 2nd place! Answer was: #{q.answer}"
      else
        replies << "BINGO!! #{m.sourcenick.to_s} got it right. The answer was: #{q.answer}"
        replies << "OMG!! PONIES!! #{m.sourcenick.to_s} is the cutest. The answer was: #{q.answer}"
        replies << "HUZZAAAH! #{m.sourcenick.to_s} did it again. The answer was: #{q.answer}"
        replies << "YEEEHA! Cowboy #{m.sourcenick.to_s} scored again. The answer was: #{q.answer}"
        replies << "STRIKE! #{m.sourcenick.to_s} pwned you all. The answer was: #{q.answer}"
        replies << "YAY :)) #{m.sourcenick.to_s} is totally invited to my next sleepover. The answer was: #{q.answer}"
        replies << "And the crowd GOES WILD for #{m.sourcenick.to_s}. The answer was: #{q.answer}"
        replies << "GOOOAAALLLL! That was one fine strike by #{m.sourcenick.to_s}. The answer was: #{q.answer}"
        replies << "HOO-RAY, #{m.sourcenick.to_s} deserves a medal! Only #{m.sourcenick.to_s} could have known the answer: #{q.answer}"
        replies << "OKAY, #{m.sourcenick.to_s} is officially a spermatologist! Answer was: #{q.answer}"
        replies << "WOOO, I bet that #{m.sourcenick.to_s} knows where the word 'trivia' comes from too! Answer was: #{q.answer}"
      end

      m.reply replies[rand( replies.length )]

      player = nil
      if q.registry.has_key?( m.sourcenick.to_s )
        player = q.registry[m.sourcenick.to_s]
      else
        player = PlayerStats.new( 0, 0, 0 )
      end

      player.score = player.score + points

      # Reward player with a joker every X points
      if player.score % 15 == 0 and player.jokers < Max_Jokers
        player.jokers += 1
        m.reply "#{m.sourcenick.to_s} gains a new joker. Rejoice :)"
      end

      q.registry[m.sourcenick.to_s] = player
      calculate_ranks( m, q, m.sourcenick.to_s )

      q.question = nil
      cmd_quiz( m, nil ) if q.registry_conf["autoask"]
    else
      # First try is used, and it wasn't the answer.
      q.first_try = false
    end
  end


  # Stretches an IRC nick with dots, simply to make the client not trigger a hilight,
  # which is annoying for those not watching. Example: markey -> m.a.r.k.e.y
  #
  def unhilight_nick( nick )
    new_nick = ""

    0.upto( nick.length - 1 ) do |i|
      new_nick += nick[i, 1]
      new_nick += "." unless i == nick.length - 1
    end

    return new_nick
  end


  #######################################################################
  # Command handling
  #######################################################################
  def cmd_quiz( m, params )
    # if m.target.to_s == "#amarok"
    #     m.reply "Please join #amarok.gaming for quizzing! :)"
    #     return
    # end

    fetch_data( m ) if @questions.empty?

    q = create_quiz( m.target.to_s )

    if q.question
      m.reply "#{Bold}#{Color}03Current question: #{Color}#{Bold}#{q.question}"
      m.reply "Hint: #{q.hint}" if q.hinted
      return
    end

    # Fill per-channel questions buffer
    if q.questions.empty?
      temp = @questions.dup

      temp.length.times do
        i = rand( temp.length )
        q.questions << temp[i]
        temp.delete_at( i )
      end
    end

    i = rand( q.questions.length )
    q.question = q.questions[i].question
    q.answer     = q.questions[i].answer.gsub( "#", "" )

    begin
      q.answer_core = /(#)(.*)(#)/.match( q.questions[i].answer )[2]
    rescue
      q.answer_core = nil
    end
    q.answer_core = q.answer.dup if q.answer_core == nil

    # Check if core answer is numerical and tell the players so, if that's the case
    # The rather obscure statement is needed because to_i and to_f returns 99(.0) for "99 red balloons", and 0 for "balloon"
    q.question += "#{Color}07 (Numerical answer)#{Color}" if q.answer_core.to_i.to_s == q.answer_core or q.answer_core.to_f.to_s == q.answer_core

    q.questions.delete_at( i )

    q.first_try = true

    q.hint = ""
    (0..q.answer_core.length-1).each do |index|
      if is_sep(q.answer_core[index,1])
        q.hint << q.answer_core[index]
      else
        q.hint << "^"
      end
    end
    q.hinted = false

    # Generate array of unique random range
    q.hintrange = (0..q.answer_core.length-1).sort_by{rand}

    m.reply "#{Bold}#{Color}03Question: #{Color}#{Bold}" + q.question
  end


  def cmd_solve( m, params )
    return unless @quizzes.has_key?( m.target.to_s )
    q = @quizzes[m.target.to_s]

    m.reply "The correct answer was: #{q.answer}"

    q.question = nil

    cmd_quiz( m, nil ) if q.registry_conf["autoask"]
  end


  def cmd_hint( m, params )
    return unless @quizzes.has_key?( m.target.to_s )
    q = @quizzes[m.target.to_s]

    if q.question == nil
      m.reply "#{m.sourcenick.to_s}: Get a question first!"
    else
      num_chars = case q.hintrange.length    # Number of characters to reveal
      when 25..1000 then 7
      when 20..1000 then 6
      when 16..1000 then 5
      when 12..1000 then 4
      when  8..1000 then 3
      when  5..1000 then 2
      when  1..1000 then 1
      end

      num_chars.times do
        begin
          index = q.hintrange.pop
          # New hint char until the char isn't a "separator" (space etc.)
        end while is_sep(q.answer_core[index,1])
        q.hint[index] = q.answer_core[index]
      end
      m.reply "Hint: #{q.hint}"
      q.hinted = true

      if q.hint == q.answer_core
        m.reply "#{Bold}#{Color}04BUST!#{Color}#{Bold} This round is over. #{Color}04Minus one point for #{m.sourcenick.to_s}#{Color}."

        stats = nil
        if q.registry.has_key?( m.sourcenick.to_s )
          stats = q.registry[m.sourcenick.to_s]
        else
          stats = PlayerStats.new( 0, 0, 0 )
        end

        stats["score"] = stats.score -    1
        q.registry[m.sourcenick.to_s] = stats

        calculate_ranks( m, q, m.sourcenick.to_s )

        q.question = nil
        cmd_quiz( m, nil ) if q.registry_conf["autoask"]
      end
    end
  end


  def cmd_skip( m, params )
    return unless @quizzes.has_key?( m.target.to_s )
    q = @quizzes[m.target.to_s]

    q.question = nil
    cmd_quiz( m, params )
  end


  def cmd_joker( m, params )
    q = create_quiz( m.target.to_s )

    if q.question == nil
      m.reply "#{m.sourcenick.to_s}: There is no open question."
      return
    end

    if q.registry[m.sourcenick.to_s].jokers > 0
      player = q.registry[m.sourcenick.to_s]
      player.jokers -= 1
      player.score += 1
      q.registry[m.sourcenick.to_s] = player

      calculate_ranks( m, q, m.sourcenick.to_s )

      if player.jokers != 1
        jokers = "jokers"
      else
        jokers = "joker"
      end
      m.reply "#{Bold}#{Color}12JOKER!#{Color}#{Bold} #{m.sourcenick.to_s} draws a joker and wins this round. You have #{player.jokers} #{jokers} left."
      m.reply "The answer was: #{q.answer}."

      q.question = nil
      cmd_quiz( m, nil ) if q.registry_conf["autoask"]
    else
      m.reply "#{m.sourcenick.to_s}: You don't have any jokers left ;("
    end
  end


  def cmd_fetch( m, params )
    fetch_data( m )
  end


  def cmd_top5( m, params )
    q = create_quiz( m.target.to_s )

    debug q.rank_table.inspect

    m.reply "* Top 5 Players for #{m.target.to_s}:"

    [5, q.rank_table.length].min.times do |i|
      player = q.rank_table[i]
      nick = player[0]
      score = player[1].score
      m.reply "    #{i + 1}. #{unhilight_nick( nick )} (#{score})"
    end
  end


  def cmd_top_number( m, params )
    num = params[:number].to_i
    return unless 1..50 === num
    q = create_quiz( m.target.to_s )

    debug q.rank_table.inspect

    ar = []
    m.reply "* Top #{num} Players for #{m.target.to_s}:"
    n = [ num, q.rank_table.length ].min
    n.times do |i|
      player = q.rank_table[i]
      nick = player[0]
      score = player[1].score
      ar << "#{i + 1}. #{unhilight_nick( nick )} (#{score})"
    end
    str = ar.join(" | ")

    if str.empty?
      m.reply "Noone in #{m.target.to_s} has a score!"
    else
      m.reply str
    end
  end


  def cmd_stats( m, params )
    fetch_data( m ) if @questions.empty?

    m.reply "* Total Number of Questions:"
    m.reply "    #{@questions.length}"
  end


  def cmd_score( m, params )
    say_score( m, m.sourcenick.to_s )
  end


  def cmd_score_player( m, params )
    say_score( m, params[:player] )
  end


  def cmd_autoask( m, params )
    q = create_quiz( m.target.to_s )

    if params[:enable].downcase == "on"
      q.registry_conf["autoask"] = true
      m.reply "Enabled autoask mode."
      cmd_quiz( m, nil ) if q.question == nil
    elsif params[:enable].downcase == "off"
      q.registry_conf["autoask"] = false
      m.reply "Disabled autoask mode."
    else
      m.reply "Invalid autoask parameter. Use 'on' or 'off'."
    end
  end


  def cmd_transfer( m, params )
    q = create_quiz( m.target.to_s )

    debug q.rank_table.inspect

    source = params[:source]
    dest = params[:dest]
    transscore = params[:score].to_i
    transjokers = params[:jokers].to_i
    debug "Transferring #{transscore} points and #{transjokers} jokers from #{source} to #{dest}"

    if q.registry.has_key?(source)
      sourceplayer = q.registry[source]
      score = sourceplayer.score
      if transscore == -1
        transscore = score
      end
      if score < transscore
        m.reply "#{source} only has #{score} points!"
        return
      end
      jokers = sourceplayer.jokers
      if transjokers == -1
        transjokers = jokers
      end
      if jokers < transjokers
        m.reply "#{source} only has #{jokers} jokers!!"
        return
      end
      if q.registry.has_key?(dest)
        destplayer = q.registry[dest]
      else
        destplayer = PlayerStats.new(0,0,0)
      end

      sourceplayer.score -= transscore
      destplayer.score += transscore
      sourceplayer.jokers -= transjokers
      destplayer.jokers += transjokers

      q.registry[source] = sourceplayer
      calculate_ranks(m, q, source)

      q.registry[dest] = destplayer
      calculate_ranks(m, q, dest)

      m.reply "Transferred #{transscore} points and #{transjokers} jokers from #{source} to #{dest}"
    else
      m.reply "#{source} doesn't have any points!"
    end
  end


  def cmd_del_player( m, params )
    q = create_quiz( m.target.to_s )
    debug q.rank_table.inspect

    nick = params[:nick]
    if q.registry.has_key?(nick)
      player = q.registry[nick]
      score = player.score
      if score != 0
        m.reply "Can't delete player #{nick} with score #{score}."
        return
      end
      jokers = player.jokers
      if jokers != 0
        m.reply "Can't delete player #{nick} with #{jokers} jokers."
        return
      end
      q.registry.delete(nick)

      player_rank = nil
      q.rank_table.each_index { |rank|
        if nick == q.rank_table[rank][0]
          player_rank = rank
          break
        end
      }
      q.rank_table.delete_at(player_rank)

      m.reply "Player #{nick} deleted."
    else
      m.reply "Player #{nick} isn't even in the database."
    end
  end


  def cmd_set_score(m, params)
    q = create_quiz( m.target.to_s )
    debug q.rank_table.inspect

    nick = params[:nick]
    val = params[:score].to_i
    if q.registry.has_key?(nick)
      player = q.registry[nick]
      player.score = val
    else
      player = PlayerStats.new( val, 0, 0)
    end
    q.registry[nick] = player
    calculate_ranks(m, q, nick)
    m.reply "Score for player #{nick} set to #{val}."
  end


  def cmd_set_jokers(m, params)
    q = create_quiz( m.target.to_s )

    nick = params[:nick]
    val = [params[:jokers].to_i, Max_Jokers].min
    if q.registry.has_key?(nick)
      player = q.registry[nick]
      player.jokers = val
    else
      player = PlayerStats.new( 0, val, 0)
    end
    q.registry[nick] = player
    m.reply "Jokers for player #{nick} set to #{val}."
  end
end



plugin = QuizPlugin.new
plugin.default_auth( 'edit', false )

# Normal commands
plugin.map 'quiz',                  :action => 'cmd_quiz'
plugin.map 'quiz solve',            :action => 'cmd_solve'
plugin.map 'quiz hint',             :action => 'cmd_hint'
plugin.map 'quiz skip',             :action => 'cmd_skip'
plugin.map 'quiz joker',            :action => 'cmd_joker'
plugin.map 'quiz score',            :action => 'cmd_score'
plugin.map 'quiz score :player',    :action => 'cmd_score_player'
plugin.map 'quiz fetch',            :action => 'cmd_fetch'
plugin.map 'quiz top5',             :action => 'cmd_top5'
plugin.map 'quiz top :number',      :action => 'cmd_top_number'
plugin.map 'quiz stats',            :action => 'cmd_stats'

# Admin commands
plugin.map 'quiz autoask :enable',  :action => 'cmd_autoask', :auth_path => 'edit'
plugin.map 'quiz transfer :source :dest :score :jokers', :action => 'cmd_transfer', :auth_path => 'edit', :defaults => {:score => '-1', :jokers => '-1'}
plugin.map 'quiz deleteplayer :nick', :action => 'cmd_del_player', :auth_path => 'edit'
plugin.map 'quiz setscore :nick :score', :action => 'cmd_set_score', :auth_path => 'edit'
plugin.map 'quiz setjokers :nick :jokers', :action => 'cmd_set_jokers', :auth_path => 'edit'
