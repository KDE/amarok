# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class Bundle
    attr_reader :question, :answer

    def initialize( question, answer )
        @question = question
        @answer = answer
    end
end


class QuizPlugin < Plugin
    def initialize()
        super

        @quest = Array.new
        @quest_orig = Array.new

        @current_question = nil
        @current_answer  = nil
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

    def shuffle()
        @quest.clear
        temp = @quest_orig.dup

        temp.length.times do
            i = rand( temp.length )
            @quest << temp[i]
            temp.delete_at( i )
        end
    end

    def help( plugin, topic="" )
        "Quiz game. Tell me 'ask' to start. You can add new questions at http://amarok.kde.org/amarokwiki/index.php/Rbot_Quiz"
    end

    def privmsg( m )
        case m.message
            when "ask"
                fetch_data( m ) if @quest_orig.empty?
                shuffle if @quest.empty?

                i = rand( @quest.length )
                @current_question = @quest[ i ].question
                @current_answer   = @quest[ i ].answer
                @quest.delete_at( i )

                @bot.say( m.replyto, @current_question )

            when "answer"
                @bot.say( m.replyto, "The correct answer was: #{@current_answer}" )

                @current_question = nil

            when "quiz_fetch"
                fetch_data( m )
                shuffle

            when "hint"
                if @current_question == nil
                    @bot.say( m.replyto, "Get a question first!" )
                else
                    s = ""
                    @current_answer.length.times do
                        s << "."
                    end

                    index = rand( s.length )
                    s[index] = @current_answer[index]

                    @bot.say( m.replyto, "Hint: #{s}" )
                end

            when "quiz_stats"
                fetch_data( m ) if @quest_orig.empty?

                @bot.say( m.replyto, "Questions in database: #{@quest_orig.length}" )
        end
    end

    def listen( m )
        return if @current_question == nil

        if m.message.downcase == @current_answer.downcase
            replies = []
            replies << "BINGO!! #{m.sourcenick} got it right. The answer was: #{@current_answer}"
            replies << "OMG!! PONIES!!! #{m.sourcenick} is the cutest. The answer was: #{@current_answer}"
            replies << "HUZZAAAH! #{m.sourcenick} did it again. The answer was: #{@current_answer}"
            replies << "YEEEHA! Cowboy #{m.sourcenick} scored again. The answer was: #{@current_answer}"
            replies << "STRIKE! #{m.sourcenick} pwned you all. The answer was: #{@current_answer}"

            @bot.say( m.replyto, replies[rand( replies.length )] )

            @current_question = nil
        end
    end
end



plugin = QuizPlugin.new
plugin.register("ask")
plugin.register("answer")
plugin.register("hint")
plugin.register("quiz_fetch")
plugin.register("quiz_stats")


