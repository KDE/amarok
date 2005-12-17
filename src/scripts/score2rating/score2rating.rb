#!/usr/bin/env ruby
#
# Score2Rating for amaroK 1.4
# -----------------------------
#
# First of all:
# PLEASE MAKE A BACKUP OF YOUR DATABASE BEFORE RUNNING THIS SCRIPT!
# It might do bad things, although it shouldn't.
#
# This script will convert the scores of your (played) tracks to ratings,
# mostly to give you somewhat of a starting point for the new rating system.
# It will ignore tracks that already has ratings.
#


if !system( "dcop amarok > /dev/null 2>&1" ) then #Simple running check
  print "ERROR: amaroK isn't running!\n"
  exit(1)
end

result = `dcop amarok collection query "SELECT url FROM statistics WHERE rating=0"` #List tracks with no rating
list = result.split( "\n" )

print "Converting track scores to ratings...\n"

list.each do |url|
  sqlurl = url.gsub( /[']/, '\\\\\'' ) #Escape single quotes
  percentage = Float( `dcop amarok collection query "SELECT percentage FROM statistics WHERE url='#{sqlurl}'"`.gsub( /\n/, '' ) )

  case percentage
  when 0...20
    rating = 1 #Crap
  when 20...60
    rating = 2 #Tolerable
  when 60...85
    rating = 3 #Good
  when 85...95
    rating = 4 #Excellent
  when 95...100
    rating = 5 #Inconceivable!
  end

  print url, " - Score: ", percentage, ", Rating: ", rating, "... "

  if !system( "dcop amarok collection query \"UPDATE statistics SET rating='#{rating}' WHERE url='#{sqlurl}'\" > /dev/null 2>&1" ) then
    #if the dcop command fails, amaroK probably has quitted, so we won't show any popup.
    print "Fail!\nProbably lost contact with amaroK.\n"
    exit(1)
  else
    print "OK\n"
  end
end

print "Done.\n"

system( "dcop amarok playlist popupMessage \"Done! All your tracks (that have been played at least once) should now have ratings. You will have to restart amaroK to see them.\" > /dev/null" )

exit(0)