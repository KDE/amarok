#!/usr/bin/env ruby
#
# Score2Rating 1.2 (for amaroK 1.4)
# -----------------------------------
#
# First of all:
# PLEASE MAKE A BACKUP OF YOUR DATABASE BEFORE RUNNING THIS SCRIPT!
# It might do bad things, although it shouldn't.
#
# This script will convert the scores of your (played) tracks to ratings,
# mostly to give you somewhat of a starting point for the new rating system.
# It will ignore tracks that already has ratings.
#


if !system( "dcop amarok playlist popupMessage \"Score2Rating started. Click the 'Configure' button in the script manager (with Score2Rating selected) to start the conversion. IT IS RECOMMENDED THAT YOU MAKE A BACKUP OF YOUR DATABASE BEFORE DOING THIS!\" > /dev/null 2>&1" ) then #Info message, and running check combined
  print "ERROR: A suitable amaroK wasn't found running!\n"
  exit(1) #Exit with error
end

loop do
  message = gets().chomp()
  command = /[A-Za-z]*/.match( message ).to_s()

  case command
  when "configure"
    result = `dcop amarok collection query "SELECT url FROM statistics WHERE rating=0"` #List tracks with no rating
    list = result.split( "\n" )

    success = true #Assume the procedure is successful

    list.each do |url|
      sqlurl = url.gsub( /[']/, '\\\\\'' ) #\Escape single 'quotes'
      percentage = Float( `dcop amarok collection query "SELECT percentage FROM statistics WHERE url='#{sqlurl}'"`.gsub( /\n/, '' ) )

      case percentage #The intervals are wrapped so that no scores will be left out.
      when 0...20
        rating = 1 #Crap
      when 20...60
        rating = 2 #Tolerable
      when 60...85
        rating = 3 #Good. Most tracks will end up here.
      when 85...95
        rating = 4 #Excellent
      when 95...100
        rating = 5 #Inconceivable!
      end

      if !system( "dcop amarok collection query \"UPDATE statistics SET rating='#{rating}' WHERE url='#{sqlurl}'\" > /dev/null 2>&1" ) then #If the command fails:
        success = false #The procedure wasn't successful
      end
    end

    if success == true then #If the procedure was sucessful
      system( "dcop amarok playlist popupMessage \"Done! All your tracks (that have been played at least once) should now have ratings. You will have to restart amaroK to see them.\" > /dev/null" )
      exit(0) #Exit without error
    else
      system( "dcop amarok playlist popupMessage \"Score2Rating has finished its conversion, but the process failed for at least one track. You might want to try running it again.\" > /dev/null" )
      exit(1) #Exit with error
    end
  end
end
