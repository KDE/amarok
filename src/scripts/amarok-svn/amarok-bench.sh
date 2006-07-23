#!/bin/bash

# # # # # # # # # # # # # # # # # # # # # #   # # # # #   # # #   # #   # #   # #   #   #
# Amarok-bench
# ==================
# Measures the time it takes to compile Amarok, meant to be run in a directory controlled by
# Amarok-svn (also by me).
#
# If you run it with a number as parameter (I.E. `./amarok-bench.sh 2`), that number will be
# passed to unsermake's -j option, this is useful for SMP systems.
# # # # # # # # # # # # # # # # # # # # # #   # # # # #   # # #   # #   # #   # #   #   #

echo
echo "Amarok-bench (Version 1.0)"
echo "============================"
echo

## Define functions
function Error {
  echo
  echo -e "ERROR: $1"
  exit 1 #Exit with error
}

function Clean {
  unsermake clean > /dev/null
  if [ "$?" != "0" ]; then #If the command didn't finish successfully
    Error "Failed to clean the source tree!"
  else
    echo "Done."
  fi
}

function Compile {
  COMP_START=`date +%s`
  #Run unsermake twice because of some files that gets forgotten sometimes
  #(We don't want to compile as root in unsermake install, and it doesn't hurt)
  unsermake -j $1 && unsermake -j $1
  if [ "$?" = "0" ]; then #If the command did finish successfully
    #stopwatch.
    let COMP_TIME=`date +%s`-COMP_START
    let COMP_M=COMP_TIME/60
    let COMP_S=COMP_TIME%60
    echo
    echo "FINISHED! Timer stopped at $COMP_M minute(s) and $COMP_S second(s)."
  else
    Error "Compilation failed! Amarok was NOT compiled."
  fi
}

JOBS=$1
[ "0$JOBS" -lt "1" ] && JOBS=1

echo "# Cleaning source tree"
Clean

echo
echo "# Compilation"
echo "Ready..."
sleep 1

echo "Set..."
sleep 1

echo "COMPILE!"

echo
Compile $JOBS

echo
echo "Now get something real to do..."

