#!/usr/bin/env ruby
#
# Score2Rating 3.0 (for Amarok 1.4)
# ---------------------------------
#
# First of all:
# PLEASE MAKE A BACKUP OF YOUR DATABASE BEFORE RUNNING THIS SCRIPT!
# It might do bad things, although it shouldn't.
#
# This script will convert the scores of your (played) tracks to ratings,
# mostly to give you somewhat of a starting point for the new rating system.
#


if !system( "dcop amarok playlist popupMessage \"Score2Rating started. Click the 'Configure' button in the script manager (with Score2Rating selected) to start the conversion. It is recommended that you make a backup of your Amarok database BEFORE doing this.\" > /dev/null 2>&1" ) then #Info message, and running check combined
    print "ERROR: A suitable Amarok wasn't found running!\n"
    exit(1) #Exit with error
end

dialog = ""

trap( "SIGTERM" ) { system( "dcop #{dialog} close" ) if dialog.length > 0 }

###THRESHOLDS START###
thres = Array.new

#Awful (1)
thres << 10

#Barely Tolerable (1.5)
thres << 30

#Tolerable (2)
thres << 45

#Okay (2.5)
thres << 60

#Good (3, Tracks played full length once (75) will be here)
thres << 70

#Very good (3.5)
thres << 80

#Excellent (4, Tracks played full length twice (87) will be here)
thres << 85

#Amazing (4.5, Tracks played full length three times (91) will be here)
thres << 90

#Favourite (5)
thres << 95

###THRESHOLDS END###
thres << 100

command = ""
while command != "configure"
    command = /[A-Za-z]*/.match( gets().chomp() ).to_s() #Wait until user clicks on Configure
end

dialog = `kdialog --title Score2Rating --icon amarok --progressbar "Converting Scores to Ratings..." 9`.chomp()
dialog = dialog.gsub( /DCOPRef\((.*),(.*)\)/, '\1 \2') #Convert the DCOPRef, Ruby doesn't seem to like it.

for r in 2 .. 10

    system( "dcop amarok collection query \"UPDATE statistics SET rating='#{r}' WHERE percentage >= '#{thres[r - 2]}' AND percentage <= '#{thres[r - 1]}' AND rating < '#{r}'\" > /dev/null" )

    system( "dcop #{dialog} setProgress #{r - 1}" ) if dialog.length > 0

end

system( "dcop #{dialog} close" ) if dialog.length > 0
dialog = ""

system( "dcop amarok playlist popupMessage \"Score2Rating is done! All your tracks (that have a score) should now have ratings. You will have to reload your playlist to see them, though.\"" )
exit(0)
