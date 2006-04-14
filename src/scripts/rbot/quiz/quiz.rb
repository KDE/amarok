# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class QuizPlugin < Plugin
    def initialize()
        super

        @quest = Array.new

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

        @quest = []
        data = data.split( "QUIZ DATA START" )[1]
        data = data.split( "QUIZ DATA END" )[0]

        entries = data.split( "</p><p><br />" )

        entries.each do |e|
            p = e.split( "</p><p>" )
            @quest << p[0].chomp
            @quest << p[1].chomp
        end
    end

    def help( plugin, topic="" )
        "Quiz game. Tell me 'ask' to start."
    end

    def privmsg( m )
        case m.message
            when "ask" then
                if @quest.empty? then fetch_data( m ) end

                i = rand( @quest.length / 2 ) * 2
                @current_question = @quest[ i ]
                @current_answer   = @quest[ i + 1 ]

                @bot.say( m.replyto, @current_question )

            when "answer" then
                @bot.say( m.replyto, "The correct answer is: #{@current_answer}" )

                @current_question = nil

            when "quiz_fetch" then
                fetch_data( m )
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


