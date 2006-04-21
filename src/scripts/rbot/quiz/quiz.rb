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

# One Quiz instance per channel, contains channel specific data
Quiz = Struct.new( "Quiz", :channel, :quest, :current_question, :current_answer,
                           :current_answer_core )


#######################################################################
# CLASS QuizPlugin
#######################################################################
class QuizPlugin < Plugin
    def initialize()
        super

        @quest_orig = Array.new
        @quizzes = Hash.new
    end


    def fetch_data( m )
        @bot.say( m.replyto, "Fetching questions from server.." )
        data = ""

        begin
            data = @bot.httputil.get( URI.parse( "http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz" ) )
            @bot.say( m.replyto, "done." )
        rescue
            @bot.say( m.replyto, "Failed to connect to the server. oioi." )
            return
        end

        @quest_orig = []
        data = data.split( "QUIZ DATA START" )[1]
        data = data.split( "QUIZ DATA END" )[0]

        entries = data.split( "</p><p><br />" )

        entries.each do |e|
            p = e.split( "</p><p>" )
            b = QuizBundle.new( p[0].chomp, p[1].chomp )
            @quest_orig << b
        end
    end


    def shuffle()
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        q.quest.clear
        temp = quest_orig.dup

        temp.length.times do
            i = rand( temp.length )
            q.quest << temp[i]
            temp.delete_at( i )
        end
    end


    def help( plugin, topic="" )
        "Quiz game. Tell me 'ask' to start. 'hint' for getting a hint and 'answer' for quick solving.\nYou can add new questions at http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz"
    end


    def listen( m )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        return if q.current_question == nil

        message = m.message.downcase.strip

        if message == q.current_answer.downcase or message == q.current_answer_core.downcase
            replies = []
            replies << "BINGO!! #{m.sourcenick} got it right. The answer was: #{q.current_answer}"
            replies << "OMG!! PONIES!! #{m.sourcenick} is the cutest. The answer was: #{q.current_answer}"
            replies << "HUZZAAAH! #{m.sourcenick} did it again. The answer was: #{q.current_answer}"
            replies << "YEEEHA! Cowboy #{m.sourcenick} scored again. The answer was: #{q.current_answer}"
            replies << "STRIKE! #{m.sourcenick} pwned you all. The answer was: #{q.current_answer}"
            replies << "YAY :)) #{m.sourcenick} is totally invited to my next sleepover. The answer was: #{q.current_answer}"
            replies << "And the crowd GOES WILD for #{m.sourcenick}.  The answer was: #{q.current_answer}"
            replies << "GOOOAAALLLL! That was one fine strike by #{m.sourcenick}. The answer was: #{q.current_answer}"
            replies << "#{m.sourcenick} deserves a medal! Only #{m.sourcenick} could have known the answer was: #{q.current_answer}"
            replies << "Okay, #{m.sourcenick} is officially a spermatologist! Answer was: #{q.current_answer}"
            replies << "I bet that #{m.sourcenick} knows where the word 'trivia' comes from too! Answer was: #{q.current_answer}"

            @bot.say( m.replyto, replies[rand( replies.length )] )

            stats = nil
            if @registry.has_key?( m.sourcenick )
                stats = @registry[m.sourcenick]
            else
                stats = PlayerStats.new( 0 )
                puts( "NEW PLAYER" )
            end

            stats["score"] = stats.score + 1
            @registry[m.sourcenick] = stats

            q.current_question = nil
    end

    #######################################################################
    # Command handling
    #######################################################################
    def cmd_quiz( m, params )
        fetch_data( m ) if @quest_orig.empty?

        unless @quizzes.has_key?( m.target )
            @quizzes[m.target] = Quiz.new( m.target, Arraw.new, nil, nil, nil )
        end
        q = @quizzes[m.target]

        unless q.current_question == nil
            @bot.say( m.replyto, "#{m.sourcenick}: Answer the current question first!" )
            return
        end

        shuffle if q.quest.empty?

        i = rand( @quest.length )
        q.current_question    = q.quest[i].question
        q.current_answer      = q.quest[i].answer.gsub( "#", "" )
        begin
            q.current_answer_core = /(#)(.*)(#)/.match( q.quest[i].answer )[2]
        rescue
            q.current_answer_core = nil
        end
        q.current_answer_core = q.current_answer.dup if q.current_answer_core == nil

        q.quest.delete_at( i )

        q.current_hint = ""
        (0..q.current_answer_core.length-1).each do |index|
            if q.current_answer_core[index, 1] == " "
                q.current_hint << " "
            else
                q.current_hint << "."
            end
        end

        # Generate array of unique random range
        q.current_hintrange = (0..q.current_answer_core.length-1).sort_by{rand}

        @bot.say( m.replyto, q.current_question )
    end


    def cmd_solve( m, params )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        @bot.say( m.replyto, "The correct answer was: #{q.current_answer}" )

        q.current_question = nil
    end


    def cmd_hint( m, params )
        return unless @quizzes.has_key?( m.target )
        q = @quizzes[m.target]

        if q.current_question == nil
            @bot.say( m.replyto, "Get a question first!" )
        else
            if q.hintrange.length >= 5
                # Reveal two characters
                2.times do
                    index = q.current_hintrange.pop
                    q.current_hint[index] = q.current_answer_core[index]
                end
                @bot.say( m.replyto, "Hint: #{q.current_hint}" )
            elsif q.current_hintrange.length >= 1
                # Reveal one character
                index = q.current_hintrange.pop
                q.current_hint[index] = q.current_answer_core[index]
                @bot.say( m.replyto, "Hint: #{q.current_hint}" )
            else
                # Bust
                @bot.say( m.replyto, "You lazy bum, what more can you want?" )
            end
        end
    end


    def cmd_fetch( m, params )
        fetch_data( m )
        shuffle
    end


    def cmd_stats( m, params )
        fetch_data( m ) if @quest_orig.empty?

        @bot.say( m.replyto, "* Total Number of Questions:" )
        @bot.say( m.replyto, "  #{@quest_orig.length}" )

        @bot.say( m.replyto, "* Top 5 Players:" )

        # Convert registry to array, then sort by score
        players = @registry.to_a.sort { |a,b| a[1].score<=>b[1].score }

        1.upto( 5 ) do |i|
            player = players.pop
            nick = player[0]
            score = player[1].score
            @bot.say( m.replyto, "  #{i}. #{nick} (#{score})" )
        end
    end


    def cmd_score( m, params )
        if @registry.has_key?( m.sourcenick )
            score = @registry[m.sourcenick].score
            @bot.say( m.replyto, "#{m.sourcenick}: Your score is: #{score}" )
        else
            @bot.say( m.replyto, "#{m.sourcenick}: You don't have a score yet, lamer." )
        end
    end
end



plugin = QuizPlugin.new

plugin.map 'quiz',       :action => 'cmd_quiz'
plugin.map 'quiz solve', :action => 'cmd_solve'
plugin.map 'quiz hint',  :action => 'cmd_hint'
plugin.map 'quiz score', :action => 'cmd_score'
plugin.map 'quiz fetch', :action => 'cmd_fetch'
plugin.map 'quiz stats', :action => 'cmd_stats'

