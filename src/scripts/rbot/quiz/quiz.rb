# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
#
# A trivia quiz game.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
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
    attr_accessor :registry, :questions, :question, :answer, :answer_core, :hint, :hintrange

    def initialize( channel, registry )
        @registry = registry.sub_registry( channel )

        @questions = Array.new
        @question = nil
        @answer = nil
        @answer_core = nil
        @hint = nil
        @hintrange = nil
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
        # TODO: Make this configurable, and add support for more than one file (there's a limit in linux too ;) )
        path = "rbot.quiz"

        @bot.say( m.replyto, "Fetching questions from the local database and the server.." )
        datafile  = File.new( path,  File::RDONLY )
        begin
            localdata = datafile.read
        rescue
            @bot.say( m.replyto, "Failed to read local database. oioi." )
        end

        begin
            serverdata = @bot.httputil.get( URI.parse( "http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz" ) )
            serverdata = serverdata.split( "QUIZ DATA START" )[1]
            serverdata = serverdata.split( "QUIZ DATA END" )[0]
            @bot.say( m.replyto, "done." )
        rescue
            @bot.say( m.replyto, "Failed to connect to the server. oioi." )
            if localdata == nil
              return
            end
        end

        @questions = []

        if localdata != nil
            serverdata = "\n\n#{serverdata}"
        end

        data = "#{localdata}#{serverdata}".gsub( /^#.*$/, "" ) # fuse together and remove comments

        entries = data.split( "\n\n" )

        entries.each do |e|
            if p[0].index( "Category: " ) != nil
                p.delete_at(0)
            end
            p = e.split( "\n" )
            p[0] = p[0].split( "Question: " )[0].chomp
            p[1] = p[1].split( "Answer: " )[0].chomp
            b = QuizBundle.new( p[0], p[1] )
            @questions << b
        end
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

            # Convert registry to array, then sort by score
            players = q.registry.to_a.sort { |a,b| b[1].score<=>a[1].score }

            rank = 0
            while rank < players.length
                if players[rank][0] == nick then break end
                rank += 1
            end

            @bot.say( m.replyto, "#{nick}'s score is: #{score}  Rank: #{rank + 1}" )
        else
            @bot.say( m.replyto, "#{nick} does not have a score yet. Lamer." )
        end
    end


    def help( plugin, topic="" )
        "Quiz game. 'quiz' => ask a question. 'quiz hint' => get a hint. 'quiz solve' => solve this question. 'quiz skip' => skip to next question. 'quiz repeat' => repeat the current question. 'quiz score <player>' => show score from <player>. 'quiz top5' => show top 5 players. 'quiz top10' => show top 10 players. 'quiz stats' => show some statistics. 'quiz fetch' => fetch new questions from server.\nYou can add new questions at http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz"
    end


    def listen( m )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        return if q.question == nil

        message = m.message.downcase.strip

        if message == q.answer.downcase or message == q.answer_core.downcase
            replies = []

            # Convert registry to array, then sort by score
            players = q.registry.to_a.sort { |a,b| b[1].score<=>a[1].score }

            if q.registry.length >= 1 and m.sourcenick == players[0][0]
                replies << "THE QUIZ CHAMPION defends his throne! Seems like #{m.sourcenick} is invicible! Answer was: #{q.answer}"
            elsif q.registry.length >= 2 and m.sourcenick == players[1][0]
                replies << "THE SECOND CHAMPION is on the way up! Hurry up #{m.sourcenick}, you only need #{players[0][1].score - players[1][1].score} points to beat the king! Answer was: #{q.answer}"
            elsif  q.registry.length >= 3 and m.sourcenick == players[2][0]
                replies << "THE THIRD CHAMPION strikes again! Give it all #{m.sourcenick}, with #{players[1][1].score - players[2][1].score} more points you'll reach the 2nd place! Answer was: #{q.answer}"
            else
                replies << "BINGO!! #{m.sourcenick} got it right. The answer was: #{q.answer}"
                replies << "OMG!! PONIES!! #{m.sourcenick} is the cutest. The answer was: #{q.answer}"
                replies << "HUZZAAAH! #{m.sourcenick} did it again. The answer was: #{q.answer}"
                replies << "YEEEHA! Cowboy #{m.sourcenick} scored again. The answer was: #{q.answer}"
                replies << "STRIKE! #{m.sourcenick} pwned you all. The answer was: #{q.answer}"
                replies << "YAY :)) #{m.sourcenick} is totally invited to my next sleepover. The answer was: #{q.answer}"
                replies << "And the crowd GOES WILD for #{m.sourcenick}. The answer was: #{q.answer}"
                replies << "GOOOAAALLLL! That was one fine strike by #{m.sourcenick}. The answer was: #{q.answer}"
                replies << "HOO-RAY, #{m.sourcenick} deserves a medal! Only #{m.sourcenick} could have known the answer was: #{q.answer}"
                replies << "OKAY, #{m.sourcenick} is officially a spermatologist! Answer was: #{q.answer}"
                replies << "WOO, I bet that #{m.sourcenick} knows where the word 'trivia' comes from too! Answer was: #{q.answer}"
            end

            @bot.say( m.replyto, replies[rand( replies.length )] )

            stats = nil
            if q.registry.has_key?( m.sourcenick )
                stats = q.registry[m.sourcenick]
            else
                stats = PlayerStats.new( 0 )
            end

            stats["score"] = stats.score + 1
            q.registry[m.sourcenick] = stats

            q.question = nil
        end
    end

    #######################################################################
    # Command handling
    #######################################################################
    def cmd_quiz( m, params )
        fetch_data( m ) if @questions.empty?

        q = create_quiz( m.target )

        unless q.question == nil
            @bot.say( m.replyto, "#{m.sourcenick}: Answer the current question first!" )
            return
        end

        shuffle( m ) if q.questions.empty?

        i = rand( q.questions.length )
        q.question = q.questions[i].question.gsub( "&nbsp;", "" )
        q.answer   = q.questions[i].answer.gsub( "#", "" )
        begin
            q.answer_core = /(#)(.*)(#)/.match( q.questions[i].answer )[2]
        rescue
            q.answer_core = nil
        end
        q.answer_core = q.answer.dup if q.answer_core == nil

        q.questions.delete_at( i )

        q.hint = ""
        (0..q.answer_core.length-1).each do |index|
            if q.answer_core[index, 1] == " "
                q.hint << " "
            else
                q.hint << "."
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
                index = q.hintrange.pop
                q.hint[index] = q.answer_core[index]
            end
            @bot.say( m.replyto, "Hint: #{q.hint}" )

            if q.hintrange.length == 0
                @bot.say( m.replyto, "BUST! This round is over." )
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

        # Convert registry to array, then sort by score
        players = q.registry.to_a.sort { |a,b| a[1].score<=>b[1].score }

        1.upto( [5, players.length].min ) do |i|
            player = players.pop
            nick = player[0]
            score = player[1].score
            @bot.say( m.replyto, "  #{i}. #{nick} (#{score})" )
        end
    end


    def cmd_top10( m, params )
        q = create_quiz( m.target )

        @bot.say( m.replyto, "* Top 10 Players for #{m.target}:" )

        # Convert registry to array, then sort by score
        players = q.registry.to_a.sort { |a,b| a[1].score<=>b[1].score }

        str = ""
        1.upto( [10, players.length].min ) do |i|
            player = players.pop
            nick = player[0]
            score = player[1].score
            str << "#{i}. #{nick} (#{score}) | "
        end

        @bot.say( m.replyto, str )
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
end



plugin = QuizPlugin.new

plugin.map 'quiz',               :action => 'cmd_quiz'
plugin.map 'quiz solve',         :action => 'cmd_solve'
plugin.map 'quiz hint',          :action => 'cmd_hint'
plugin.map 'quiz skip',          :action => 'cmd_skip'
plugin.map 'quiz repeat',        :action => 'cmd_repeat'
plugin.map 'quiz score',         :action => 'cmd_score'
plugin.map 'quiz score :player', :action => 'cmd_score_player'
plugin.map 'quiz fetch',         :action => 'cmd_fetch'
plugin.map 'quiz top5',          :action => 'cmd_top5'
plugin.map 'quiz top10',         :action => 'cmd_top10'
plugin.map 'quiz stats',         :action => 'cmd_stats'

