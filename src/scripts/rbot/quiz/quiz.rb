# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
#
# A quiz game.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


#######################################################################
# CLASS Bundle
#######################################################################
class Bundle
    attr_reader :question, :answer

    def initialize( question, answer )
        @question = question
        @answer = answer
    end
end


#######################################################################
# CLASS Quiz
# One Quiz instance per channel, handles all channel specific stuff
#######################################################################
class Quiz
    def initialize( channel, plugin )
        @channel = channel
        @plugin = plugin

        @quest = Array.new

        @current_question = nil
        @current_answer  = nil
    end

    def shuffle()
        @quest.clear
        temp = @plugin.quest_orig.dup

        temp.length.times do
            i = rand( temp.length )
            @quest << temp[i]
            temp.delete_at( i )
        end
    end

    def privmsg( m )
        case m.message
            when "ask"
                shuffle if @quest.empty?

                i = rand( @quest.length )
                @current_question = @quest[ i ].question
                @current_answer   = @quest[ i ].answer
                @quest.delete_at( i )

                @plugin.bot.say( m.replyto, @current_question )

            when "answer"
                @plugin.bot.say( m.replyto, "The correct answer was: #{@current_answer}" )

                @current_question = nil

            when "quiz_fetch"
                @plugin.fetch_data( m )
                shuffle

            when "hint"
                if @current_question == nil
                    @plugin.bot.say( m.replyto, "Get a question first!" )
                else
                    s = ""
                    (0..@current_answer.length-1).each do |index|
                        if @current_answer[index, 1] == " "
                            s << " "
                        else
                            s << "."
                        end
                    end

                    index = rand( s.length )
                    s[index] = @current_answer[index]

                    @plugin.bot.say( m.replyto, "Hint: #{s}" )
                end

            when "quiz_stats"
                fetch_data( m ) if @plugin.quest_orig.empty?

                @plugin.bot.say( m.replyto, "Questions in database: #{@plugin.quest_orig.length}" )
        end
    end

    def listen( m )
        if m.message.downcase == @current_answer.downcase
            replies = []
            replies << "BINGO!! #{m.sourcenick} got it right. The answer was: #{@current_answer}"
            replies << "OMG!! PONIES!! #{m.sourcenick} is the cutest. The answer was: #{@current_answer}"
            replies << "HUZZAAAH! #{m.sourcenick} did it again. The answer was: #{@current_answer}"
            replies << "YEEEHA! Cowboy #{m.sourcenick} scored again. The answer was: #{@current_answer}"
            replies << "STRIKE! #{m.sourcenick} pwned you all. The answer was: #{@current_answer}"
            replies << "YAY :)) #{m.sourcenick} is totally invited to my next sleepover. The answer was: #{@current_answer}"

            @plugin.bot.say( m.replyto, replies[rand( replies.length )] )

            @current_question = nil
        end
    end
end


#######################################################################
# CLASS QuizPlugin
#######################################################################
class QuizPlugin < Plugin
    attr_accessor :bot
    attr_reader   :quest_orig

    def initialize()
        super

        @quest_orig = Array.new
        @quizzes = Hash.new
    end

    def fetch_data( m )
        @bot.say( m.replyto, "Fetching questions from server.." )
        data = ""

        begin
            data = bot.httputil.get( URI.parse( "http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz" ) )
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
            b = Bundle.new( p[0].chomp, p[1].chomp )
            @quest_orig << b
        end
    end

    def help( plugin, topic="" )
        "Quiz game. Tell me 'ask' to start. 'hint' for getting a hint and 'answer' for quick solving.\nYou can add new questions at http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz"
    end

    def privmsg( m )
        fetch_data( m ) if @quest_orig.empty?

        unless @quizzes.has_key?( m.target )
            @quizzes[m.target] = Quiz.new( m.target, self )
        end

        @quizzes[m.target].privmsg( m )
    end

    def listen( m )
        if @quizzes.has_key?( m.target )
            @quizzes[m.target].listen( m )
        end
    end
end



plugin = QuizPlugin.new
plugin.register("ask")
plugin.register("answer")
plugin.register("hint")
plugin.register("quiz_fetch")
plugin.register("quiz_stats")


