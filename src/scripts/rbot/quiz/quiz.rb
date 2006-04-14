# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class QuizPlugin < Plugin
    def initialize()
        super

        @quest = Array.new
        @quest << "What, in population terms, is the largest Spanish speaking country in the world?"
        @quest << "Mexico"
        @quest << "In which country would you find both the rivers Oder and Vistula, which flow into the Baltic Sea?"
        @quest << "Poland"
        @quest << "What is the capital of Iran?"
        @quest << "Tehran"
        @quest << "Lusitania was the Roman name for which EU country?"
        @quest << "Portugal"
        @quest << "In which country is Timbuktu?"
        @quest << "Mali"

        @current_question = nil
        @current_answer  = nil
    end

    def help( plugin, topic="" )
        "Quiz game. Tell me 'ask' to start."
    end

    def privmsg( m )
        i = rand( @quest.length / 2 ) * 2
        @current_question = @quest[ i ]
        @current_answer   = @quest[ i + 1 ]

        @bot.say( m.replyto, @current_question )
    end

    def listen( m )
        return if @current_question == nil

        if m.message.downcase == @current_answer.downcase
            @bot.say( m.replyto, "BINGO!! Correct answer." )
            @current_question = nil
        end
    end
end



plugin = QuizPlugin.new
plugin.register("ask")

