#Released under GPL v2 or Later      
#Ian Monroe (C) 2005             

class ShakespeareInsultPlugin < Plugin
  def initialize
    super
#from http://www.pangloss.com/seidel/shake_rule.html
    @rowone =
["artless",      "impertinent",     "surly",
"bawdy",         "infectious",      "tottering",
"beslubbering",  "jarring",         "unmuzzled",
"bootless",      "loggerheaded",    "vain",
"churlish",      "lumpish",         "venomed",
"cockered",      "mammering",       "villainous",
"clouted",       "mangled",         "warped",
"craven",        "mewling",         "wayward",
"currish",       "paunchy",         "weedy",
"dankish",       "pribbling",       "yeasty",
"dissembling",   "puking",
"droning",       "puny",
"errant",        "qualling",
"fawning",       "rank",
"fobbing",       "reeky",
"froward",       "roguish",
"frothy",        "ruttish",
"gleeking",      "saucy",
"goatish",       "spleeny",
"gorbellied",    "spongy"]
    @rowtwo = 
["base-court",      "fly-bitten",            "pottle-deep", 
"bat-fowling",      "folly-fallen",          "pox-marked", 
"beef-witted",      "fool-born",             "reeling-ripe", 
"beetle-headed",    "full-gorged",           "rough-hewn", 
"boil-brained",     "guts-griping",          "rude-growing", 
"clapper-clawed",   "half-faced",            "rump-fed", 
"clay-brained",     "hasty-witted",          "shard-borne", 
"common-kissing",   "hedge-born",            "sheep-biting", 
"crook-pated",      "hell-hated",            "spur-galled", 
"dismal-dreaming",  "idle-headed",           "swag-bellied", 
"dizzy-eyed",       "ill-breeding",          "tardy-gaited", 
"doghearted",       "ill-nurtured",          "tickle-brained",
"dread-bolted",     "knotty-pated",          "toad-spotted", 
"earth-vexing",     "milk-livered",          "unchin-snouted",
"elf-skinned",      "motley-minded",         "weather-bitten", 
"fat-kidneyed",     "onion-eyed",      
"fen-sucked",       "plume-plucked",  
"flap-mouthed"]
    @rowthree=
["apple-john",       "flap-dragon",     "minnow",
"baggage",          "flax-wench",      "miscreant",
"barnacle",         "flirt-gill",      "moldwarp",
"bladder",          "foot-licker",     "mumble-news",
"boar-pig",         "fustilarian",     "nut-hook",
"bugbear",          "giglet",          "pigeon-egg",
"bum-bailey",       "gudgeon",         "pignut",
"canker-blossom",   "haggard",         "puttock",
"clack-dish",       "harpy",           "pumpion",
"clotpole",         "hedge-pig",       "ratsbane",
"coxcomb",          "horn-beast",      "scut",
"codpiece",         "hugger-mugger",   "skainsmate",
"death-token",      "joithead",        "strumpet",
"dewberry",         "lewdster",        "varlot",
                    "lout",            "vassal",
                    "maggot-pie",      "whey-face",
                    "malt-worm",       "wagtail",
                    "mammet", 
                    "measle"]
    end
  def listen(m)
    if(m.message =~ /#{@bot.nick}/i || m.message =~ /latex/i || m.message =~ /xmms/i || m.message =~ /itunes/i || m.message =~ /winamp/i || m.message =~ /windows media player/i || m.message =~ /rhythmbox/i || m.message =~ /muine/i)
        response = m.sourcenick + ": "
        response += "you are a "
        response += @rowone[rand(@rowone.size)] + ' ' + @rowtwo[rand(@rowtwo.size)] + ' '
        response += @rowthree[rand(@rowthree.size)] + '!'
        m.reply(response)
    end
  end
end          
plugin = ShakespeareInsultPlugin.new
plugin.register("shakespeare")
##############################################
#fin         
