#!/usr/bin/env ruby
#
# Score2Rating 2.5 (for Amarok 1.4)
# ---------------------------------
#
# First of all:
# PLEASE MAKE A BACKUP OF YOUR DATABASE BEFORE RUNNING THIS SCRIPT!
# It might do bad things, although it shouldn't.
#
# This script will convert the scores of your (played) tracks to ratings,
# mostly to give you somewhat of a starting point for the new rating system.
#


if !system( "dcop amarok playlist popupMessage \"Score2Rating started. Click the 'Configure' button in the script manager (with Score2Rating selected) to start the conversion. IT IS RECOMMENDED THAT YOU MAKE A BACKUP OF YOUR DATABASE BEFORE DOING THIS!\" > /dev/null 2>&1" ) then #Info message, and running check combined
  print "ERROR: A suitable Amarok wasn't found running!\n"
  exit(1) #Exit with error
end

dialog = ""

trap( "SIGTERM" ) { system( "dcop #{dialog} close" ) if dialog.length > 0 }

loop do
  message = gets().chomp()
  command = /[A-Za-z]*/.match( message ).to_s()

  case command
  when "configure"
    result = `dcop amarok collection query "SELECT url FROM statistics WHERE percentage > 0 ORDER BY url"`.chomp() #List tracks that have a score
    list = result.split( "\n" )

    success = true #Assume the procedure is successful
    failList = Array.new()

    tracknum = 0

    dialog = `kdialog --title Score2Rating --icon amarok --progressbar "Converting Scores to Ratings..." #{list.length}`.chomp()
    dialog = dialog.gsub( /DCOPRef\((.*),(.*)\)/, '\1 \2') #Convert the DCOPRef, Ruby doesn't seem to like it.

    list.each do |url|
      tracknum += 1
      system( "dcop #{dialog} setProgress #{tracknum}" ) if dialog.length > 0

      sqlurl = url.gsub( /(['"`])/, '\\\\\\1' ) #\Escape 'single' and "double" quotes and `backticks`

      begin
        percentage = Float( `dcop amarok collection query "SELECT percentage FROM statistics WHERE url='#{sqlurl}'"`.chomp() )

        rating = Integer( `dcop amarok collection query "SELECT rating FROM statistics WHERE url='#{sqlurl}'"`.chomp() )
        oldrating = rating

        case percentage #The intervals are wrapped so that no scores will be left out. No existing ratings will be downgraded either.
        when 0..30
            rating = [ rating, 2 ].max #Awful
        when 30..45
            rating = [ rating, 3 ].max #Barely Tolerable
        when 45..60
            rating = [ rating, 4 ].max #Tolerable
        when 60..70
            rating = [ rating, 5 ].max #Okay
        when 70..80
            rating = [ rating, 6 ].max #Good (Tracks played full length once (75) will be here)
        when 80..85
            rating = [ rating, 7 ].max #Very good
        when 85..90
            rating = [ rating, 8 ].max #Excellent (Tracks played full length twice (87) will be here)
        when 90..95
            rating = [ rating, 9 ].max #Amazing (Tracks played full length three times (91) will be here)
        when 95..100
            rating = [ rating, 10 ].max #Favourite
        end

        if rating != oldrating then #If a new rating was calculated
          if !system( "dcop amarok collection query \"UPDATE statistics SET rating='#{rating}' WHERE url='#{sqlurl}'\" > /dev/null 2>&1" ) then #If the command fails:
            success = false #The procedure wasn't successful
            failList << "DCOP: " + url
          end
        end
      rescue
        success = false
        failList << "SEVERE: " + url
      end
    end

    system( "dcop #{dialog} close" ) if dialog.length > 0

    if success == true then #If the procedure was sucessful
      system( "kdialog --msgbox \"Done! All your tracks (that have a score) should now have ratings. You will have to reload your playlist to see them, though.\"" )
    else
      failCount = failList.length
      STDERR.puts( "FAILED URL LIST START\n" + failList.join("\n") + "\nFAILED URL LIST END\nPlease e-mail the entire contents of this error report to ajocke (at) gmail (dot) com to help further development of Score2Rating.")

      system( "kdialog --sorry \"Score2Rating has finished its conversion, but the process failed for #{failCount} track(s).\nA list of the failed URL(s) will be given to you if you click Details in Amarok's error dialog which will appear after you dismiss this dialog.\n(To see the new ratings, you will have to reload your playlist.)\"" )
      exit(38) #Exit with error to give the user Amarok's error box
    end
  end
end