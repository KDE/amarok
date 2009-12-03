#!/bin/bash

# # # # # # # # # # # # # # # # # # # # # #   # # # # #   # # #   # #   # #   # #   #   #
# Amarok-svn
# ============
# This script installs the current development (SVN) version of Amarok on your computer.
# If you've run it once, and then run it again, it will update your version of Amarok and only compile the new files.
#
# Made by Jocke "Firetech" Andersson.
# Idea and inspiration from a small script by Greg "oggb4mp3" Meyer.
# # # # # # # # # # # # # # # # # # # # # #   # # # # #   # # #   # #   # #   # #   #   #

echo
echo "Amarok-svn (Version 3.2-SVN)"
echo "=============================="
echo

## Define global variables
LANG="C" #Make outputs in English, like the script itself.
RCFILE="amarok-svnrc" #Settings file, will end up in '`kde-config --localprefix`/share/config/'.
C_STEP="1" #The current step of the installation process
S_STEPS="11" #Number of steps in the installation process
TMP_FILES="" #Will be filled with URLs to temporary files

## Define functions
function Dialog {
  kdialog --icon "amarok" --title "Amarok-svn$KDTITLE" "$@"
}

function RemoveTemp {
  rm -f $TMP_FILES
}

function Error {
  RemoveTemp
  echo
  echo -e "ERROR: $1"
  if [ "$2" != "--no-dialog" ]; then
    Dialog --error "$1"
  fi
  exit 1 #Exit with error
}

function ReadConfig {
  kreadconfig --file "$RCFILE" --group Settings --key "$1"
}

function WriteConfig {
  kwriteconfig --file "$RCFILE" --group Settings --key "$1" "$2"
}

function CheckBinary {
  if [ ! -x "`which $1`" ]; then #check if $1 is in $PATH and can be executed
    ERROR_TEXT="Amarok-svn requires $1, which wasn't found in your \$PATH!"
    if [ "$2" ]; then
      ERROR_TEXT="$ERROR_TEXT ($2)"
    fi 
    Error "$ERROR_TEXT" $3
  fi
}

function FlagUsage {
  echo "Usage: $0 [options] [builddir]"
  echo
  echo "Amarok-svn installs the current development (SVN) version of Amarok on your computer."
  echo
  echo "Options:"
  echo -e "  -r, --reset\t\tAsk for settings again."
  echo -e "  -s, --select-server\tAsk which SVN server to use. (Only needed if you want to use your SVN account.)"
  echo -e "  -c, --clean\t\tClean the source tree before compiling Amarok."
  echo -e "  -h, --help\t\tShow this message."
  echo
  echo "Arguments:"
  echo -e "  builddir\t\tDownload and build Amarok in this directory. Default is '[your current dir]/amarok-svn'."
  echo
  echo "Notes:"
  echo "  * Options can NOT be fused together! (I.E. You can't use '-cr', you have to use '-c -r'.)"
  echo "  * Amarok-svn will download the Amarok sources directly into the folder you choose, without creating any subdirectory!"
  echo
  exit 1
}

function Clean {
  unsermake clean
  if [ "$?" != "0" ]; then #If the command didn't finish successfully
    Error "Failed to clean the source tree!"
  fi
}

function Compile {
  COMP_START=`date +%s`
  #Run unsermake twice because of some files that gets forgotten sometimes
  #(We don't want to compile as root in unsermake install, and it doesn't hurt)
  unsermake && unsermake
  if [ "$?" = "0" ]; then #If the command did finish successfully
    echo
    #stopwatch.
    let COMP_TIME=`date +%s`-COMP_START
    let COMP_M=COMP_TIME/60
    let COMP_S=COMP_TIME%60
    echo "Compilation successful after $COMP_M minute(s) and $COMP_S second(s)."
  else
    echo
    if [ "$CLEAN_BUILD" = "0" ]; then
      echo "Compilation Failed!"
      Dialog --warningyesno "Compilation failed!\nSometimes, a clean rebuild can fix this.\nDo you want to retry compiling with a clean source tree?\nIf you answer No, Amarok-svn will exit."
      if [ "$?" = "0" ]; then #If user said yes
        echo "Retrying with a clean source tree."
        CLEAN_BUILD="1"
        Clean
        Compile
      else
        Error "Compilation failed! Amarok was NOT installed/updated."
      fi
    else
      Error "Compilation failed! Amarok was NOT installed/updated."
    fi
  fi
}

## Handle --flags
# set default values
RESET_CONF="0"
CLEAN_BUILD="0"
SELECT_SERVER="0"
BUILD_DIR="`pwd`/amarok-svn"
# read flags values
BUILD_DIR_SET="0"
for flag; do
  case "$flag" in
    -r|--reset)
      RESET_CONF="1"
      ;;
    -c|--clean)
      CLEAN_BUILD="1"
      let S_STEPS=S_STEPS+1
      ;;
    -s|--select-server)
      SELECT_SERVER="1"
      ;;
    -h|--help)
      FlagUsage
      ;;
    -*)
      echo "Unknown option '$flag'."
      echo
      echo "---------------------- ---- --- -- -- -- - -"
      echo
      FlagUsage
      ;;
    *)
      if [ "$BUILD_DIR_SET" = "0" ]; then
        BUILD_DIR="$flag"
        BUILD_DIR_SET="1"
      else
        echo "Excessive argument: '$flag'."
        echo
        echo "---------------------- ---- --- -- -- -- - -"
        echo
        FlagUsage
      fi
      ;;
  esac
done

## Check requirements
CheckBinary kdialog "" --no-dialog
CheckBinary svn "Version 1.1 or newer is needed."
CheckBinary kde-config "kde-config sometimes falls out of the \$PATH for some reason."
CheckBinary kreadconfig
CheckBinary kwriteconfig

## Check the build directory
if [ -e "$BUILD_DIR" -a ! -f "$BUILD_DIR/.amarok-svn-dir" ]; then #if directory exist and isn't watermarked
  if [ -d "$BUILD_DIR" ]; then
    Dialog --warningyesno "The directory you choosed to build in ($BUILD_DIR) already exists, and it wasn't detected as an Amarok-svn directory.\nFiles in this directory can possibly be overwritten by the Amarok-svn procedures.\nDo you want to use this directory anyway?"
    if [ "$?" != "0" ]; then #If the user said no, exit.
      exit 1
    fi
  else
    Error "The build directory you have chosen exists, but it's not a directory!"
  fi
fi

## Check if user is root
if [ "`id -u`" = "0" ]; then #if user is root
  Dialog --warningcontinuecancel "You are running Amarok-svn as root! This is not required, and generally not a good idea.\n(Amarok-svn will get root privileges by itself when needed, see the settings for details.)\nAre you sure you want to continue anyway?" --dontagain $RCFILE:root_warning
  if [ "$?" != "0" ]; then #If the user said cancel.
    exit 1
  fi
fi


## Settings
if [ -s "`kde-config --localprefix`/share/config/$RCFILE" -a "$RESET_CONF" != "1" ]; then #If the settings exists and the user doesn't want to change them.

  GET_LANG="`ReadConfig get_lang`"
  if  [ -z "$GET_LANG" ]; then #Save default value if empty
    GET_LANG="en_US"
    WriteConfig get_lang "$GET_LANG"
  fi
  CONF_FLAGS_RAW="`ReadConfig conf_flags`"
  CONF_FLAGS=""
  for flag in $CONF_FLAGS_RAW; do
    CONF_FLAGS="$CONF_FLAGS `echo $flag | sed -e \"s/__/--/\"`"
  done
  HOW_ROOT="`ReadConfig how_root`"
  if  [ -z "$HOW_ROOT" ]; then #Save default value if empty
    HOW_ROOT="kdesu"
    WriteConfig how_root "$HOW_ROOT"
  fi
  USE_ID="`ReadConfig use_id`"
  if  [ -z "$USE_ID" ]; then #Save default value if empty
    USE_ID="0"
    WriteConfig use_id "$USE_ID"
  fi

else

  ## Language
  AUTO_LANG="`kreadconfig --group Locale --key Language | sed -re \"s/:.+//\"`"
  if [ "$AUTO_LANG" != "" ]; then #Generally, if the user is running KDE
    Dialog --yesno "I detected that you are running KDE with language '$AUTO_LANG'.\nIf this is correct (and you want it that way), I will download localization and documentation for Amarok in that language.\nDo you want this language?"
  fi
  if [ "$?" = "0" -a "$AUTO_LANG" != "" ]; then #If the user said yes, and is running KDE...
    GET_LANG="$AUTO_LANG"
  else
    Dialog --msgbox "Which language do you want to download localization and documentation for?\nA list of available languages is available at http://websvn.kde.org/trunk/l10n/ (It is CaSe sensitive!)\nIf you want to use the default language (American English), either leave this empty or set it to 'en_US' (it's not in the list above).\n(Click Ok to get to the input box.)"
    GET_LANG="`Dialog --inputbox \"Specify language to download localization and documentation for\"`"
  fi
  if  [ -z "$GET_LANG" ]; then
    GET_LANG="en_US"
  fi
  WriteConfig get_lang "$GET_LANG"

  ## ./configure flags
  Dialog --yesno "Do you want to use any extra configuration options (in addition to '--prefix=`kde-config --prefix` --enable-debug=full')?\nNo extra options is the default, and that works fine.\n(For a list of available flags, say yes and enter 'help' (CaSe insensitive) in the box, then wait for Amarok-svn to get to the configuration step (step 8).)"
  if [ "$?" = "0" ]; then #If the user said yes
    CONF_FLAGS_RAW="`Dialog --inputbox \"Specify extra configuration options to use\"`"
    if [ "`echo $CONF_FLAGS_RAW | tr A-Z a-z`" != "help" ]; then
      CONF_FLAGS=""
      CONF_FLAGS_SAVE=""
      for flag in $CONF_FLAGS_RAW; do
        if [ "$flag" != "--help" ]; then
          CONF_FLAGS="$CONF_FLAGS $flag"
          CONF_FLAGS_SAVE="$CONF_FLAGS_SAVE `echo $flag | sed -e \"s/--/__/\"`"
        fi
      done
      WriteConfig conf_flags "$CONF_FLAGS_SAVE"
    else
      CONF_HELP="true"
    fi
  fi

  ## What to use to get root privileges for installation?
  HOW_ROOT="`kdialog --radiolist \"How do you want Amarok-svn to get root privileges for the install/uninstall commands?\" kdesu \"With  'kdesu' (default, choose this if unsure)\" on sudo \"With 'sudo'\" off \"su -c\" \"With 'su -c'\" off`"
  if [ -z "$HOW_ROOT" ]; then #Fallback if the user pressed cancel
    HOW_ROOT="kdesu"
  fi
  WriteConfig how_root "$HOW_ROOT"

  USE_ID="0" #Assume default answer
  Dialog --yesno "Do you want to use build ID?\nThis feature is generally not needed, and often makes the compiling time longer.\nIf you use it, you can tell from the About box in Amarok when your revision was compiled.\nDefault answer is No, you can get this information from other places."
  if [ "$?" = "0" ]; then #If the user said yes
    USE_ID="1"
  fi
  WriteConfig use_id "$USE_ID"
fi

if [ "$SELECT_SERVER" = "1" ]; then #SVN server selection
  WriteConfig svn_server "`kdialog --title \"$KD_TITLE\" --inputbox \"Specify which SVN server you want to use. Default is 'svn://anonsvn.kde.org'.\"`"
fi

SVN_SERVER="`ReadConfig svn_server`"
if  [ -z "$SVN_SERVER" ]; then #Save default value if empty
  SVN_SERVER="svn://anonsvn.kde.org"
  WriteConfig svn_server "$SVN_SERVER"
fi

## Start the installation process and show the settings
INST_START=`date +%s`
echo "Used configuration"
echo "--------------------"
echo  "(Use --help to get information on how to change it.)"
echo 

echo -e "SVN server:\t\t\t\t\t$SVN_SERVER"
echo -e "Language for localization and documentation:\t$GET_LANG"
if [ "$CONF_HELP" != "true" -a "`echo $CONF_FLAGS`" ]; then
  echo -e "Extra configuration options:\t\t\t`echo $CONF_FLAGS`" #`echo ...` strips the preceding space.
fi
echo -e "Command for getting root privileges:\t\t$HOW_ROOT"
echo -en "Build ID:\t\t\t\t\t"
if [ "$USE_ID" = "1" ]; then
  echo "On"
else
  echo "Off"
fi
echo -e "Build directory:\t\t\t\t$BUILD_DIR"
echo -ne "Clean source tree:\t\t\t\t"
if [ "$CLEAN_BUILD" = "1" ]; then
  echo "On"
else
  echo "Off"
fi

echo
echo "###################### #### ### ## ## ## # #"

## Base checkout
echo
echo "# $C_STEP/$S_STEPS - Checking out base files."
svn co -N $SVN_SERVER/home/kde/trunk/extragear/multimedia $BUILD_DIR
if [ "$?" != "0" ]; then #If the command didn't finish successfully
  Error "The SVN transfer didn't finish successfully."
fi
cd $BUILD_DIR
touch .amarok-svn-dir #Watermark the directory

## Get unsermake, if not installed already
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Getting unsermake."
if [ -x "`which unsermake`" ]; then #unsermake installed system wide?
  echo "Unsermake is already installed system wide."
else
  PATH="$BUILD_DIR/unsermake:$PATH"
  if [ -x "`which unsermake`" ]; then #Is unsermake installed and in the $PATH now?
    echo "Unsermake downloaded by this script. Checking for update."
  else
    echo "Unsermake wasn't found. Downloading it to '$BUILD_DIR/unsermake'."
  fi
  svn co -N $SVN_SERVER/home/kde/trunk/kdenonbeta/unsermake
  if [ "$?" != "0" ]; then #If the command didn't finish successfully
    Error "The SVN transfer failed."
  fi
fi

## Store the old uninstall commands
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Saving uninstall commands for your current revision."
if [ ! -f "Makefile" ]; then
  echo "No current revision was found."
else
  TMP_OLD_UNINFO="`mktemp`"
  TEMP_FILES="$TMP_OLD_UNINFO"
  unsermake -n uninstall > $TMP_OLD_UNINFO #Stored in a file so we can run it later, in a simple manner.
  if [ "$?" != "0" ]; then #If the command didn't finish successfully
    Error "Couldn't get uninstall commands for your current revision."
  else
    echo "Done."
  fi
fi

## Continue checkout
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Checking out common SVN files."
svn co $SVN_SERVER/home/kde/branches/KDE/3.5/kde-common/admin #URL changed since KDE 3.5 branching
if [ "$?" != "0" ]; then #If the command didn't finish successfully
  RemoveTemp
  Error "The SVN transfer failed."
fi
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Updating Amarok files."
svn up amarok
if [ "$?" != "0" ]; then #If the command didn't finish successfully
  RemoveTemp
  Error "The SVN transfer failed.\nIf the message from svn (in console) is something like 'amarok is not under version control', you need a never version of svn.\nAt least version 1.1 is needed."
fi
if [ "$USE_ID" = "1" ]; then
  #Append build ID (date and time with no punctuation) to version
  sed -re "s/^#define APP_VERSION \"(.*)-SVN.*\"/#define APP_VERSION \"\1-SVN-`date +%y%m%d%H%M`\"/" -i amarok/src/amarok.h
  echo "Appended build ID to version number."
fi

echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Getting localization and documentation:"
ENGD_STEP=""

if [ "$GET_LANG" != "en_US" ]; then #If a language (not en_US) is selected

  ## Localization
  echo "- # 1/4 - Checking if localization for selected language exists."
  svn ls $SVN_SERVER/home/kde/trunk/l10n/$GET_LANG/messages/extragear-multimedia/amarok.po > /dev/null 2>&1 #Check if language exists
  if [ "$?" != "0" ]; then #If the localization wasn't found
    GET_LANG="en_US"
    echo "WARNING:  Localization for selected language was not found. Reverting to 'en_US'."
  else #If localization exists
    echo "- # 2/4 - Updating localization file."
    if [ ! -d "po" ]; then
      mkdir po
    fi
    cd po
    echo "SUBDIRS = $GET_LANG" > Makefile.am
    if [ ! -d $GET_LANG ]; then
      mkdir $GET_LANG
    fi
    cd $GET_LANG
    echo "KDE_LANG = $GET_LANG" > Makefile.am
    echo "SUBDIRS  = \$(AUTODIRS)" >> Makefile.am
    echo "POFILES  = AUTO" >> Makefile.am
    echo "Downloading current localization file from server."
    TMP_L10N="`mktemp`"
    TMP_FILES="$TMP_FILES $TMP_L10N"
    svn cat $SVN_SERVER/home/kde/trunk/l10n/$GET_LANG/messages/extragear-multimedia/amarok.po 2> /dev/null | tee $TMP_L10N > /dev/null
    if [ "$?" != "0" ]; then #If the command didn't finish successfully
      Error "The SVN transfer didn't finish successfully."
    fi
    if [ ! -s "$TMP_L10N" ]; then
      echo "WARNING: The downloaded file was empty!"
      echo "Some error must have occured."
      rm -f Makefile.am
      rm -f ../Makefile.am
      echo "No localization downloaded, so no localization will be installed!"
    elif [ -f "amarok.po" ]; then
      if [ -z "`diff -q $TMP_L10N amarok.po 2>&1`" ]; then
        echo "You already have the current version of the localization file."
      else
        echo "New localization file downloaded."
        rm -f amarok.po
        mv -f $TMP_L10N amarok.po
        echo "Replaced the old copy with the downloaded version."
      fi
    else
      mv -f $TMP_L10N amarok.po
      echo "Localization file downloaded."
    fi
    cd ../..

    ## Localized documentation
    echo "- # 3/4 - Checking if localized documentation for selected language exists."
    svn ls $SVN_SERVER/home/kde/trunk/l10n/$GET_LANG/docs/extragear-multimedia/amarok > /dev/null 2>&1 #Check if localized documentation exists
    if [ "$?" != "0" ]; then #If the localized documentation wasn't found
      GET_LANG="en_US"
      echo "WARNING: Localized documentation for selected language was not found. Reverting to default (American English) documentation."
      ENGD_STEP=" 4/4 -"
    else #If a localized documentation exists
      echo "- # 4/4 - Checking out localized documentation."
      if [ ! -d "doc" ]; then
        mkdir doc
      fi
      cd doc
      echo "SUBDIRS = $GET_LANG" > Makefile.am
      svn co $SVN_SERVER/home/kde/trunk/l10n/$GET_LANG/docs/extragear-multimedia/amarok $GET_LANG
      if [ "$?" != "0" ]; then #If the command didn't finish successfully
        Error "The SVN transfer failed."
      fi
      cd $GET_LANG
      echo "KDE_LANG = $GET_LANG" > Makefile.am
      echo "KDE_DOCS = amarok" >> Makefile.am
      cd ../..

    fi
  fi
fi

if [ "$GET_LANG" = "en_US" ]; then #If no language (en_US) is selected. This is a stand alone if-statement to enable fallbacks when selected language isn't found.

  ## Default (American English) documentation
  echo "- #$ENGD_STEP Checking out default (American English) documentation."
  if [ ! -d "doc" ]; then
    mkdir doc
  fi
  cd doc
  echo "SUBDIRS = en" > Makefile.am
  svn co $SVN_SERVER/home/kde/trunk/extragear/multimedia/doc/amarok en
  if [ "$?" != "0" ]; then #If the command didn't finish successfully
    Error "The SVN transfer failed."
  fi
  cd en
  echo "KDE_LANG = en" > Makefile.am
  echo "KDE_DOCS = amarok" >> Makefile.am
  cd ../..

fi

## Preparation
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Preparing for configuration. (This will take a while.)"
WANT_AUTOCONF="2.5" unsermake -f Makefile.cvs
if [ "$?" != "0" ]; then #If the command didn't finish successfully
  Error "Preparation failed.\nProblems at this step are quite certainly problems with either unsermake or your automake configuration."
fi
echo -n "*** Patching the configure script to show when it fails... "
echo -e "\nif test \"\$all_tests\" = \"bad\"; then\n  exit 1\nfi" >> configure #Does its job, albeit a little bit ugly...
if [ "$?" != "0" ]; then #If the command didn't finish successfully
  echo "Fail. Amarok will compile even if some dependencies aren't met!"
else
  echo "Done."
fi

## Configuration help
if [ "$CONF_HELP" = "true" ]; then
  TMP_CONF="`mktemp`"
  TMP_FILES="$TMP_FILES $TMP_CONF"
  echo -e "<big><b><u>Configuration options</u></b></big>\n" > $TMP_CONF
  ./configure --help >> $TMP_CONF
  KDTITLE=" :: Configuration options" Dialog --textbox $TMP_CONF 600 800 & #800x600 should be enough. Put it in the background (&) to make the user able to watch it while typing the selected options into the input box.
  Dialog --yesno "Do you want to use any extra configuration options (in addition to '--prefix=`kde-config --prefix` --enable-debug=full')?\nNo extra options is the default, and that works fine.\n(Available options are displayed in another window right now.)"
  if [ "$?" = "0" ]; then #If the user said yes
    CONF_FLAGS_RAW="`kdialog --title \"$KD_TITLE\" --inputbox \"Specify extra configuration options to use\"`"
    CONF_FLAGS=""
    CONF_FLAGS_SAVE=""
    for flag in $CONF_FLAGS_RAW; do
      if [ "$flag" != "--help" ]; then
        CONF_FLAGS="$CONF_FLAGS $flag"
        CONF_FLAGS_SAVE="$CONF_FLAGS_SAVE `echo $flag | sed -e \"s/--/__/\"`"
      fi
    done
    WriteConfig conf_flags "$CONF_FLAGS_SAVE"
    if [ "`echo $CONF_FLAGS`" ]; then
      echo -e "Extra configuration options:\t'`echo $CONF_FLAGS`'"
    fi
  fi
fi

## Configuration
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Configuring. (This will also take a while.)"
./configure --prefix=`kde-config --prefix` --enable-debug=full$CONF_FLAGS
if [ "$?" != "0" ]; then #If the command didn't finish successfully
  Error "Configuration failed. Amarok was NOT installed/upgraded."
fi

## Clean build dir if user wanted to.
if [ "$CLEAN_BUILD" = "1" ]; then
  let C_STEP=C_STEP+1
  echo "# $C_STEP/$S_STEPS - Cleaning the source tree."
  Clean
fi

## Compilation
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Compiling. (The time of this step depends on the number of new source files that were downloaded.)"
Compile

## Compare uninstall commands and , if they differ, uninstall the old revision.
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Comparing uninstall commands."
if [ ! -f "$TMP_OLD_UNINFO" ]; then
  echo "No older revision was found."
else
  TMP_NEW_UNINFO="`mktemp`"
  TMP_FILES="$TMP_FILES $TMP_NEW_UNINFO"
  unsermake -n uninstall > $TMP_NEW_UNINFO
  if [ "$?" != "0" ]; then #If the command didn't finish successfully.
    Error "Couldn't get uninstall commands for the new revision."
  fi
  UN_DIFF="`diff -q $TMP_OLD_UNINFO $TMP_NEW_UNINFO 2>&1`"
  if [ "$?" != "0" -a "$?" != "1" ]; then #If the command didn't finish successfully. diff strangely (?) returns 1 when there are differences.
    Error "Couldn't compare the uninstall commands."
  elif [ -z "$UN_DIFF" ]; then #If diff was quiet
    echo "No differences between your current revision and the new one were found."
  else
    echo "Differences in the uninstall commands were found, uninstalling your current revision."
    echo "Executing '$HOW_ROOT' to get root privileges for uninstallation."
    if [ "$HOW_ROOT" = "sudo" ]; then
      echo "(You might need to enter your password now.)"
      sudo bash $TMP_OLD_UNINFO
    elif [ "$HOW_ROOT" = "su -c" ]; then
      echo "(You probably have to enter the root password now.)"
      su -c "bash $TMP_OLD_UNINFO"
    else
      kdesu -t bash $TMP_OLD_UNINFO
    fi
    #No error check here because it seems to trigger even when there is no error, I can't figure out why...
    #It doesn't matter much if this step fails anyway...
  fi 
fi
RemoveTemp #From here on, no temp files are needed.

## Installation
echo
let C_STEP=C_STEP+1
echo "# $C_STEP/$S_STEPS - Installing files."
echo "Executing '$HOW_ROOT' to get root privileges for installation."
if [ "$HOW_ROOT" = "sudo" ]; then
  echo "(You might need to enter your password now.)"
  sudo `which unsermake` install #"which" here (and below) is needed because root doesn't use the user's $PATH
elif [ "$HOW_ROOT" = "su -c" ]; then
  echo "(You probably have to enter the root password now.)"
  su -c "`which unsermake` install"
else
  kdesu -t `which unsermake` install
fi
if [ "$?" = "0" ]; then #If the command did finish successfully
  #stopwatch.
  let INST_TIME=`date +%s`-INST_START
  let INST_M=INST_TIME/60
  let INST_S=INST_TIME%60
  echo
  echo -e "# DONE - Amarok was successfully installed/updated after $INST_M minute(s) and $INST_S second(s)."
  Dialog --msgbox "Done!\nAmarok was successfully installed/updated after $INST_M minute(s) and $INST_S second(s).\nCompilation took $COMP_M minute(s) and $COMP_S second(s).\nStart Amarok from your menu or by running the command \"amarok\"."
  exit 0 #Exit succsessfully
else
  echo
  Error "Amarok was compiled but NOT installed.\nIf the errors above seem to be permission errors, you could try installing Amarok manually.\n(You also might want to have a look at the settings for this script, use --help to see how.)\nTo install manually, get root privileges in some way and then run 'unsermake install' in the '$BUILD_DIR' directory."
fi
