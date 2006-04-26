# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
#
# A trivia quiz game.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# (c) 2006 Jocke Andersson <ajocke@gmail.com>
# Licensed under GPL V2.


# Class for storing question/answer pairs
QuizBundle = Struct.new( "QuizBundle", :question, :answer )

# Class for storing player stats
PlayerStats = Struct.new( "PlayerStats", :score )


#######################################################################
# CLASS Quiz
# One Quiz instance per channel, contains channel specific data
#######################################################################
class Quiz
    attr_accessor :registry, :registry_conf, :questions, :question, :answer, :answer_core,
                  :first_try, :hint, :hintrange, :rank_table

    def initialize( channel, registry )
        @registry = registry.sub_registry( channel )
        @registry_conf = @registry.sub_registry( "config" )

        @questions = Array.new
        @question = nil
        @answer = nil
        @answer_core = nil
        @first_try = false
        @hint = nil
        @hintrange = nil

        # We keep this array of player stats for performance reasons. It's sorted by score
        # and always synced with the registry player stats hash. This way we can do fast
        # rank lookups, without extra sorting.
        @rank_table = @registry.to_a.sort { |a,b| b[1].score<=>a[1].score }
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


    def fetch_data( m )
        # TODO: Make this configurable, and add support for more than one file (there's a size limit in linux too ;) )
        path = "/home/eean/.rbot/quiz.rbot"

        @bot.say( m.replyto, "Fetching questions from local database and wiki.." )

        # Local data
        begin
            datafile  = File.new( path,  File::RDONLY )
            localdata = datafile.read
        rescue
            @bot.say( m.replyto, "Failed to read local database file. oioi." )
            localdata = nil
        end

        # Wiki data
        begin
            serverdata = @bot.httputil.get( URI.parse( "http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz" ) )
            serverdata = serverdata.split( "QUIZ DATA START\n" )[1]
            serverdata = serverdata.split( "\nQUIZ DATA END" )[0]
            serverdata = serverdata.gsub( /&nbsp;/, " " ).gsub( /&amp;/, "&" ).gsub( /&quot;/, "\"" )
        rescue
            @bot.say( m.replyto, "Failed to download wiki questions. oioi." )
            if localdata == nil
              @bot.say( m.replyto, "No questions loaded, aborting." )
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

        @bot.say( m.replyto, "done." )
    end


    def shuffle( m )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        q.questions.clear
        temp = @questions.dup

        temp.length.times do
            i = rand( temp.length )
            q.questions << temp[i]
            temp.delete_at( i )
        end
    end


    # Return new Quiz instance for channel, or existing one
    #
    def create_quiz( channel )
        unless @quizzes.has_key?( channel )
            @quizzes[channel] = Quiz.new( channel, @registry )
        end

        return @quizzes[channel]
    end


    def say_score( m, nick )
        q = create_quiz( m.target )

        if q.registry.has_key?( nick )
            score = q.registry[nick].score

            rank = 0
            q.rank_table.length.times do |rank|
                break if nick == q.rank_table[rank][0]
            end

            @bot.say( m.replyto, "#{nick}'s score is: #{score}  Rank: #{rank + 1}" )
        else
            @bot.say( m.replyto, "#{nick} does not have a score yet. Lamer." )
        end
    end


    def help( plugin, topic="" )
        "Quiz game. 'quiz' => ask a question. 'quiz hint' => get a hint. 'quiz solve' => solve this question. 'quiz skip' => skip to next question. 'quiz repeat' => repeat the current question. 'quiz score <player>' => show score from <player>. 'quiz top5' => show top 5 players. 'quiz top <number>' => show top <number> players (max 50). 'quiz stats' => show some statistics. 'quiz fetch' => fetch new questions from server. 'quiz autoask <on/off>' => Enable/disable autoask mode.\nYou can add new questions at http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz"
    end


    def calculate_ranks( m, q )
        if q.registry.has_key?( m.sourcenick )
            stats = q.registry[m.sourcenick]

            # Find player in table
            i = 0
            q.rank_table.length.times do |i|
                break if m.sourcenick == q.rank_table[i][0]
            end

            old_rank = i
            q.rank_table.delete_at( i )

            # Insert player at new position
            inserted = false
            q.rank_table.length.times do |i|
                if stats.score >= q.rank_table[i][1].score
                    q.rank_table[i,0] = [[m.sourcenick, stats]]
                    inserted = true
                    break
                end
            end

            # If less than all other players' scores, append at the end
            unless inserted
                q.rank_table << [[m.sourcenick, stats]]
            end

            if i < old_rank
                @bot.say( m.replyto, "#{m.sourcenick} ascends to rank #{i + 1}. Congratulations :)" )
            elsif i > old_rank
                @bot.say( m.replyto, "#{m.sourcenick} slides down to rank #{i + 1}. So Sorry! NOT. :p" )
            end
        else
            q.rank_table << [[m.sourcenick, PlayerStats.new( 1 )]]
        end
    end


    def listen( m )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        return if q.question == nil

        message = m.message.downcase.strip

        if message == q.answer.downcase or message == q.answer_core.downcase
            replies = []

            points = 1
            if q.first_try
                points += 1
                replies << "WHOPEEE! #{m.sourcenick} got it on the first try! That's worth an extra point. Answer was: #{q.answer}"
            elsif q.rank_table.length >= 1 and m.sourcenick == q.rank_table[0][0]
                replies << "THE QUIZ CHAMPION defends his throne! Seems like #{m.sourcenick} is invicible! Answer was: #{q.answer}"
            elsif q.rank_table.length >= 2 and m.sourcenick == q.rank_table[1][0]
                replies << "THE SECOND CHAMPION is on the way up! Hurry up #{m.sourcenick}, you only need #{q.rank_table[0][1].score - q.rank_table[1][1].score - 1} points to beat the king! Answer was: #{q.answer}"
            elsif  q.rank_table.length >= 3 and m.sourcenick == q.rank_table[2][0]
                replies << "THE THIRD CHAMPION strikes again! Give it all #{m.sourcenick}, with #{q.rank_table[1][1].score - q.rank_table[2][1].score - 1} more points you'll reach the 2nd place! Answer was: #{q.answer}"
            else
                replies << "BINGO!! #{m.sourcenick} got it right. The answer was: #{q.answer}"
                replies << "OMG!! PONIES!! #{m.sourcenick} is the cutest. The answer was: #{q.answer}"
                replies << "HUZZAAAH! #{m.sourcenick} did it again. The answer was: #{q.answer}"
                replies << "YEEEHA! Cowboy #{m.sourcenick} scored again. The answer was: #{q.answer}"
                replies << "STRIKE! #{m.sourcenick} pwned you all. The answer was: #{q.answer}"
                replies << "YAY :)) #{m.sourcenick} is totally invited to my next sleepover. The answer was: #{q.answer}"
                replies << "And the crowd GOES WILD for #{m.sourcenick}. The answer was: #{q.answer}"
                replies << "GOOOAAALLLL! That was one fine strike by #{m.sourcenick}. The answer was: #{q.answer}"
                replies << "HOO-RAY, #{m.sourcenick} deserves a medal! Only #{m.sourcenick} could have known the answer: #{q.answer}"
                replies << "OKAY, #{m.sourcenick} is officially a spermatologist! Answer was: #{q.answer}"
                replies << "WOOO, I bet that #{m.sourcenick} knows where the word 'trivia' comes from too! Answer was: #{q.answer}"
            end

            @bot.say( m.replyto, replies[rand( replies.length )] )

            stats = nil
            if q.registry.has_key?( m.sourcenick )
                stats = q.registry[m.sourcenick]
            else
                stats = PlayerStats.new( 0 )
            end

            stats["score"] = stats.score + points
            q.registry[m.sourcenick] = stats

            calculate_ranks( m, q )

            q.question = nil
            cmd_quiz( m, nil ) if q.registry_conf["autoask"]
        else
            # First try is used, and it wasn't the answer.
            q.first_try = false
        end
    end

    #######################################################################
    # Command handling
    #######################################################################
    def cmd_quiz( m, params )
        if m.target == "#amarok"
            @bot.say( m.replyto, "Please join #amarok.gaming for quizzing! :)" )
            return
        end

        fetch_data( m ) if @questions.empty?

        q = create_quiz( m.target )

        unless q.question == nil
            @bot.say( m.replyto, "#{m.sourcenick}: Answer the current question first!" )
            return
        end

        shuffle( m ) if q.questions.empty?

        i = rand( q.questions.length )
        q.question = q.questions[i].question
        q.answer   = q.questions[i].answer.gsub( "#", "" )

        begin
            q.answer_core = /(#)(.*)(#)/.match( q.questions[i].answer )[2]
        rescue
            q.answer_core = nil
        end
        q.answer_core = q.answer.dup if q.answer_core == nil

        # Check if core answer is numerical and tell the players so, if that's the case
        # The rather obscure statement is needed because to_i returns 99 for "99 red balloons", and 0 for "balloon"
        q.question += " (Numerical answer)" if q.answer_core.to_i.to_s == q.answer_core

        q.questions.delete_at( i )

        q.first_try = true

        q.hint = ""
        (0..q.answer_core.length-1).each do |index|
            if q.answer_core[index, 1] == " "
                q.hint << " "
            else
                q.hint << "^"
            end
        end

        # Generate array of unique random range
        q.hintrange = (0..q.answer_core.length-1).sort_by{rand}

        @bot.say( m.replyto, q.question )
    end


    def cmd_solve( m, params )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        @bot.say( m.replyto, "The correct answer was: #{q.answer}" )

        q.question = nil

        cmd_quiz( m, nil ) if q.registry_conf["autoask"]
    end


    def cmd_hint( m, params )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        if q.question == nil
            @bot.say( m.replyto, "#{m.sourcenick}: Get a question first!" )
        else
            num_chars = case q.hintrange.length  # Number of characters to reveal
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
                # New hint char until the char isn't a space
                end until q.answer_core[index, 1] != " "
                q.hint[index] = q.answer_core[index]
            end
            @bot.say( m.replyto, "Hint: #{q.hint}" )

            if q.hintrange.length == 0
                @bot.say( m.replyto, "BUST! This round is over. Minus one point for #{m.sourcenick}." )

                stats = nil
                if q.registry.has_key?( m.sourcenick )
                    stats = q.registry[m.sourcenick]
                else
                    stats = PlayerStats.new( 0 )
                end

                stats["score"] = stats.score -  1
                q.registry[m.sourcenick] = stats

                calculate_ranks( m, q )

                q.question = nil
            end
        end
    end


    def cmd_skip( m, params )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        q.question = nil
        cmd_quiz( m, params )
    end


    def cmd_repeat( m, params )
        q = @quizzes[m.target]

        if q.question == nil
            @bot.say( m.replyto, "#{m.sourcenick}: There's currently no open question!" )
        else
            @bot.say( m.replyto, "Current question: #{q.question}" )
        end
    end


    def cmd_fetch( m, params )
        fetch_data( m )
        shuffle( m )
    end


    def cmd_top5( m, params )
        q = create_quiz( m.target )

        @bot.say( m.replyto, "* Top 5 Players for #{m.target}:" )

        [5, q.rank_table.length].min.times do |i|
            player = q.rank_table[i]
            nick = player[0]
            score = player[1].score
            @bot.say( m.replyto, "  #{i + 1}. #{nick} (#{score})" )
        end
    end


    def cmd_top_number( m, params )
        return if params[:number] == "0"
        q = create_quiz( m.target )

        str = ""
        @bot.say( m.replyto, "* Top #{[ params[:number].to_i, 50].min} Players for #{m.target}:" )
        n = [ params[:number].to_i, 50, q.rank_table.length ].min
        n.times do |i|
            player = q.rank_table[i]
            nick = player[0]
            score = player[1].score
            str << "#{i + 1}. #{nick} (#{score})"
            str << " | " unless i == n - 1
        end

        if str != ""
            @bot.say( m.replyto, str )
        else
            @bot.say( m.replyto, "Noone in #{m.target} has a score!" )
        end
    end


    def cmd_stats( m, params )
        fetch_data( m ) if @questions.empty?

        @bot.say( m.replyto, "* Total Number of Questions:" )
        @bot.say( m.replyto, "  #{@questions.length}" )
    end


    def cmd_score( m, params )
        say_score( m, m.sourcenick )
    end


    def cmd_score_player( m, params )
        say_score( m, params[:player] )
    end


    def cmd_autoask( m, params )
        q = create_quiz( m.target )

        if params[:enable].downcase == "on"
            q.registry_conf["autoask"] = true
            @bot.say( m.replyto, "Enabled autoask mode." )
            cmd_quiz( m, nil ) if q.registry_conf["autoask"]
        elsif params[:enable].downcase == "off"
            q.registry_conf["autoask"] = false
            @bot.say( m.replyto, "Disabled autoask mode." )
        else
            @bot.say( m.replyto, "Invalid autoask parameter. Use 'on' or 'off'." )
        end
    end
end



plugin = QuizPlugin.new

plugin.map 'quiz',                 :action => 'cmd_quiz'
plugin.map 'quiz solve',           :action => 'cmd_solve'
plugin.map 'quiz hint',            :action => 'cmd_hint'
plugin.map 'quiz skip',            :action => 'cmd_skip'
plugin.map 'quiz repeat',          :action => 'cmd_repeat'
plugin.map 'quiz score',           :action => 'cmd_score'
plugin.map 'quiz score :player',   :action => 'cmd_score_player'
plugin.map 'quiz fetch',           :action => 'cmd_fetch'
plugin.map 'quiz top5',            :action => 'cmd_top5'
plugin.map 'quiz top :number',     :action => 'cmd_top_number'
plugin.map 'quiz stats',           :action => 'cmd_stats'
plugin.map 'quiz autoask :enable', :action => 'cmd_autoask'

