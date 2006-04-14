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
        "Quiz game. Tell me 'ask' to start."
    end

    def privmsg( m )
        case m.message
            when "ask" then
                if @quest_orig.empty? then fetch_data( m ) end
                if @quest.empty? then shuffle end

                i = rand( @quest.length )
                @current_question = @quest[ i ].question
                @current_answer   = @quest[ i ].answer
                @quest.delete_at( i )

                @bot.say( m.replyto, @current_question )

            when "answer" then
                @bot.say( m.replyto, "The correct answer was: #{@current_answer}" )

                @current_question = nil

            when "quiz_fetch" then
                fetch_data( m )
                shuffle
        end
    end

    def listen( m )
        return if @current_question == nil

        if m.message.downcase == @current_answer.downcase
            @bot.say( m.replyto, "BINGO!! #{m.sourcenick} got it right. The answer was: #{@current_answer}" )
            @current_question = nil
        end
    end
end



plugin = QuizPlugin.new
plugin.register("ask")
plugin.register("answer")
plugin.register("quiz_fetch")


