#!/bin/bash

# # # # # # # # # # # # # # # # # # # # # #   # # # # #   # # #   # #   # #   # #   #   #
# amaroK-svn
# ============
# This script installs the current development (SVN) version of amaroK on your computer.
# If you've run it once, and then run it again, it will update your version of amaroK and only compile the new files.
#
# Made by Jocke "Firetech" Andersson.
# Based on a script by Greg "oggb4mp3" Meyer.
# # # # # # # # # # # # # # # # # # # # # #   # # # # #   # # #   # #   # #   # #   #   #

echo
echo "amaroK-svn (Version 2.97) by Jocke \"Firetech\" Andersson"
echo "========================================================="
echo

#define global variables
LANG="C" # Make outputs in English, like the script.
RCFILE="`pwd`/.amarok-svnrc" # Settings file
KD_TITLE="amaroK-svn" # Title for kdialog windows

#define functions
function Error {
  echo
  echo -e "ERROR: $1"
  if [ "$2" != "--no-dialog" ]; then
    kdialog --title "$KD_TITLE" --error "$1"
  fi
  exit 1 # Exit with error
}

function ReadConfig {
  kreadconfig --file "$RCFILE" --group Settings --key "$1"
}

function WriteConfig {
  kwriteconfig --file "$RCFILE" --group Settings --key "$1" "$2"
}

function CheckBinary {
  if [ ! -x "`which $1`" ]; then # check if $1 is in $PATH and can be executed
    ERROR_TEXT="amaroK-svn requires $1, which wasn't found in your \$PATH!"
    if [ "$2" ]; then
      ERROR_TEXT="$ERROR_TEXT ($2)"
    fi 
    Error "$ERROR_TEXT" $3
  fi
}

## Check requirements
CheckBinary kdialog "" --no-dialog
CheckBinary svn "Version 1.1 or newer is needed."
CheckBinary kde-config "kde-config sometimes falls out of the \$PATH for some reason."
CheckBinary kreadconfig
CheckBinary kwriteconfig

if [ "`id -u`" = 0 ]; then # if user is root
  kdialog --title "$KD_TITLE" --warningcontinuecancel "You are running amaroK-svn as root! This is not required, and generally not a good idea.\n(The script will get root privileges by itself when needed, see the settings for details.)\nAre you sure you want to continue the script anyway?"
  if [ "$?" != 0 ]; then
    exit 1
  fi
fi


## Settings
if [ -s "$RCFILE" -a "$1" != "-r" -a "$1" != "--reset" ]; then # If the settings exists and the script isn't called with -r or --reset.

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
    WriteConfig how_Root "$HOW_ROOT"
  fi

  echo "# Loading saved settings: (Start the script with -r or --reset to change.)"

else

  echo "# Asking for settings:"

  ## Language
  AUTO_LANG="`kreadconfig --group Locale --key Language | sed -re \"s/:.+//\"`"
  kdialog --title "$KD_TITLE" --yesno "I detected that you are running KDE with language '$AUTO_LANG'.\nIf this is correct (and you want it that way), I will download localization and documentation for amaroK in that language.\nDo you want this language?"
  if [ "$?" = "0" ]; then # If the user said yes
    GET_LANG="$AUTO_LANG"
  else
    kdialog --title "$KD_TITLE" --msgbox "Which language do you want to download localization and documentation for?\nA list of available languages is available at http://websvn.kde.org/trunk/l10n/ (It is CaSe sensitive!)\nIf you want to use the default language (American English), either leave this empty or set it to 'en_US' (it's not in the list above).\n(Click Ok to get to the input box.)"
    GET_LANG="`kdialog --title \"$KD_TITLE\" --inputbox \"Specify language to download localization and documentation for\"`"
  fi
  if  [ -z "$GET_LANG" ]; then
    GET_LANG="en_US"
  fi
  WriteConfig get_lang "$GET_LANG"

  ## ./configure flags
  kdialog --title "$KD_TITLE" --yesno "Do you want to use any extra configuration options (in addition to '--prefix=`kde-config --prefix` --enable-debug=full')?\nNo extra options is the default, and that works fine.\n(For a list of available flags, say yes and enter 'help' (CaSe insensitive) in the box, then wait for the script to get to the configuration step (step 8).)"
  if [ "$?" = "0" ]; then #If the user said yes
    CONF_FLAGS_RAW="`kdialog --title \"$KD_TITLE\" --inputbox \"Specify extra configuration options to use\"`"
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

  ## What to use to get sudo privileges for installation?
  HOW_ROOT="`kdialog --radiolist \"How do you want the script to get root privileges for the install/uninstall commands?\" kdesu \"With  'kdesu' (default, choose this if unsure)\" on sudo \"With 'sudo'\" off \"su -c\" \"With 'su -c'\" off`"
  if [ -z "$HOW_ROOT" ]; then #Fallback if the user pressed cancel
    HOW_ROOT="kdesu"
  fi
  WriteConfig how_root "$HOW_ROOT"
fi

SVN_SERVER="`ReadConfig svn_server`"
if  [ -z "$SVN_SERVER" ]; then #Save default value if empty
  SVN_SERVER="svn://anonsvn.kde.org"
  WriteConfig svn_server "$SVN_SERVER"
fi

## Echo settings (Doing this outside the if statement above because it's the same for both cases.)
echo "Will get localization and documentation for language '$GET_LANG'."
if [ "$CONF_HELP" != "true" -a "`echo $CONF_FLAGS`" ]; then
  echo "Will configure amaroK with the extra options '`echo $CONF_FLAGS`'."
fi
echo "Will be using '$HOW_ROOT' to get root privileges when needed."

## Base checkout
echo
echo "# 1/11 - Checking out base files."
svn co -N $SVN_SERVER/home/kde/trunk/extragear/multimedia amarok-svn
if [ "$?" != "0" ]; then # If the command didn't finish successfully
  Error "The SVN transfer didn't finish successfully."
fi
cd amarok-svn

## Get unsermake, if not installed already
echo
echo "# 2/11 - Getting unsermake."
if [ -x "`which unsermake`" ]; then # unsermake installed system wide?
  echo "Unsermake is already installed system wide."
else
  PATH="`pwd`/unsermake:$PATH"
  if [ -x "`which unsermake`" ]; then # Is unsermake installed and in the $PATH now?
    echo "Unsermake downloaded by this script. Checking for update."
  else
    echo "Unsermake wasn't found. Downloading it to '`pwd`/unsermake'."
  fi
  svn co -N $SVN_SERVER/home/kde/trunk/kdenonbeta/unsermake
  if [ "$?" != "0" ]; then # If the command didn't finish successfully
    Error "The SVN transfer didn't finish successfully."
  fi
fi

## Store the old uninstall commands
echo
echo "# 3/11 - Checking used files for the installed revision."
if [ ! -f "Makefile" ]; then
  echo "No older revision is installed."
else
  TMP_OLD_UNINFO="`mktemp`"
  unsermake -n uninstall | grep "rm -f" > $TMP_OLD_UNINFO
  if [ "$?" != "0" ]; then # If the command didn't finish successfully
    Error "Couldn't get list of used files in the installed revision."
  else
    echo "Done."
  fi
fi

## Continue checkout
echo
echo "# 4/11 - Checking out common SVN files."
svn co $SVN_SERVER/home/kde/branches/KDE/3.5/kde-common/admin # URL changed since KDE 3.5 branching
if [ "$?" != "0" ]; then # If the command didn't finish successfully
  Error "The SVN transfer didn't finish successfully."
fi
echo
echo "# 5/11 - Updating amaroK files."
svn up amarok
if [ "$?" != "0" ]; then # If the command didn't finish successfully
  Error "The SVN transfer didn't finish successfully.\nIf the message from svn (in console) is something like 'amarok is not under version control', you need a never version of svn.\nAt least version 1.1 is needed."
fi

echo
echo "# 6/11 - Getting localization and documentation:"
STEP_EN=""

if [ "$GET_LANG" != "en_US" ]; then # If a language (not en_US) is selected

  ## Localization
  echo "- # 1/4 - Checking if localization for selected language exists."
  svn ls $SVN_SERVER/home/kde/trunk/l10n/$GET_LANG/messages/extragear-multimedia/amarok.po > /dev/null 2>&1 # Check if language exists
  if [ "$?" != "0" ]; then # If the localization wasn't found
    GET_LANG="en_US"
    echo "WARNING:  Localization for selected language was not found. Reverting to 'en_US'."
  else # If localization exists
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
    TMP_FILE="`mktemp`"
    svn cat $SVN_SERVER/home/kde/trunk/l10n/$GET_LANG/messages/extragear-multimedia/amarok.po 2> /dev/null | tee $TMP_FILE > /dev/null
    if [ "$?" != "0" ]; then # If the command didn't finish successfully
      rm -f $TMP_FILE
      Error "The SVN transfer didn't finish successfully."
    fi
    if [ ! -s "$TMP_FILE" ]; then
      echo "WARNING: The downloaded file was empty!"
      echo "Some error must have occured."
      rm -f Makefile.am
      rm -f ../Makefile.am
      rm -f $TMP_FILE
      echo "No localization downloaded, so no localization will be installed!"
    elif [ -f "amarok.po" ]; then
      if [ -z "`diff -q $TMP_FILE amarok.po 2>&1`" ]; then
        echo "You already have the current version of the localization file."
        rm -f $TMP_FILE
        echo "Removed the downloaded version."
      else
        echo "New localization file downloaded."
        rm -f amarok.po
        mv -f $TMP_FILE amarok.po
        echo "Replaced the old copy with the downloaded version."
      fi
    else
      mv -f $TMP_FILE amarok.po
      echo "Localization file downloaded."
    fi
    cd ../..

    ## Localized documentation
    echo "- # 3/4 - Checking if localized documentation for selected language exists."
    svn ls $SVN_SERVER/home/kde/trunk/l10n/$GET_LANG/docs/extragear-multimedia/amarok > /dev/null 2>&1 # Check if localized documentation exists
    if [ "$?" != "0" ]; then # If the localized documentation wasn't found
      GET_LANG="en_US"
      echo "WARNING: Localized documentation for selected language was not found. Reverting to default (American English) documentation."
      STEP_EN=" 4/4 -"
    else # If a localized documentation exists
      echo "- # 4/4 - Checking out localized documentation."
      if [ ! -d "doc" ]; then
        mkdir doc
      fi
      cd doc
      echo "SUBDIRS = $GET_LANG" > Makefile.am
      svn co $SVN_SERVER/home/kde/trunk/l10n/$GET_LANG/docs/extragear-multimedia/amarok $GET_LANG
      if [ "$?" != "0" ]; then # If the command didn't finish successfully
        Error "The SVN transfer didn't finish successfully."
      fi
      cd $GET_LANG
      echo "KDE_LANG = $GET_LANG" > Makefile.am
      echo "KDE_DOCS = amarok" >> Makefile.am
      cd ../..

    fi
  fi
fi

if [ "$GET_LANG" = "en_US" ]; then # If no language (en_US) is selected. This is a stand alone if-statement to enable fallbacks when selected language isn't found.

  ## Default (American English) documentation
  echo "- #$STEP_EN Checking out default (American English) documentation."
  if [ ! -d "doc" ]; then
    mkdir doc
  fi
  cd doc
  echo "SUBDIRS = en" > Makefile.am
  svn co $SVN_SERVER/home/kde/trunk/extragear/multimedia/doc/amarok en
  if [ "$?" != "0" ]; then # If the command didn't finish successfully
    Error "The SVN transfer didn't finish successfully."
  fi
  cd en
  echo "KDE_LANG = en" > Makefile.am
  echo "KDE_DOCS = amarok" >> Makefile.am
  cd ../..

fi

## Preparation
echo
echo "# 7/11 - Preparing for configuration. (This will take a while.)"
WANT_AUTOCONF="2.5" unsermake -f Makefile.cvs
if [ "$?" != "0" ]; then # If the command didn't finish successfully
  Error "Preparation didn't finish successfully.\nProblems at this step are quite certainly problems with either unsermake or your automake configuration.\nAsk the friendly people in #amarok on freenode for help."
fi

## Configuration help
if [ "$CONF_HELP" = "true" ]; then
  TMP_FILE="`mktemp`"
  echo -e "<big><b><u>Configuration options</u></b></big>\n" > $TMP_FILE
  ./configure --help >> $TMP_FILE
  kdialog --title "$KD_TITLE :: Configuration options" --textbox $TMP_FILE 600 5000 & # height 5000 should be enough even for VERY big desktops. It will be limited to the desktop height anyway. Put it in the background (&) to make the user able to watch it while typing the selected options into the input box.
  kdialog --title "$KD_TITLE" --yesno "Do you want to use any extra configuration options (in addition to '--prefix=`kde-config --prefix` --enable-debug=full')?\nNo extra options is the default, and that works fine.\n(Available options are displayed in another window right now.)"
  rm -f $TMP_FILE
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
      echo "Will configure amaroK with the extra options '`echo $CONF_FLAGS`'."
    fi
  fi
fi

## Configuration
echo
echo "# 8/11 - Configuring. (This will also take a while.)"
./configure --prefix=`kde-config --prefix` --enable-debug=full$CONF_FLAGS
if [ "$?" != "0" ]; then # If the command didn't finish successfully
  Error "Configuration wasn't successful. amaroK was NOT installed/upgraded. Ask the friendly people in #amarok on freenode for help."
fi

## Compare uninstall commands and remove unused files
echo
echo "# 9/11 - Removing files that will be unused with the new revision."
if [ ! -f "$TMP_OLD_UNINFO" ]; then
  echo "No older revision is installed."
else
  TMP_NEW_UNINFO="`mktemp`"
  unsermake -n uninstall | grep "rm -f" > $TMP_NEW_UNINFO
  if [ "$?" != "0" ]; then # If the command didn't finish successfully
    Error "Couldn't get list of used files in the new revision."
  fi
  TMP_UNFILES="`mktemp`"
  diff --old-line-format="%L" --new-line-format="" --unchanged-line-format="" $TMP_OLD_UNINFO $TMP_NEW_UNINFO > $TMP_UNFILES # a diff that only outputs lines that only are in the first file, the grep removes some Makefile creation commands that come up all the time.
  if [ "$?" != "0" -a "$?" != "1" ]; then # If the command didn't finish successfully. diff returns 1 when there are differences.
    rm -f $TMP_OLD_UNINFO $TMP_NEW_UNINFO $TMP_UNFILES
    Error "Couldn't compare the lists with used files."
  elif [ -s "$TMP_UNFILES" ]; then
    kdialog --title "$KD_TITLE :: DEBUG: Check these commands before clicking OK!" --textbox $TMP_UNFILES 600 5000 # TODO remove this line when 3.0 goes final.
    echo "Will use '$HOW_ROOT' to get root privileges for removal of unused files."
    if [ "$HOW_ROOT" = "sudo" ]; then
      echo "(You might need to enter your password now.)"
      sudo bash $TMP_UNFILES
    elif [ "$HOW_ROOT" = "su -c" ]; then
      echo "(You probably have to enter the root password now.)"
      su -c "bash $TMP_UNFILES"
    else
      kdesu -t bash $TMP_UNFILES
    fi
    if [ "$?" != "0" ]; then # If the command didn't finish successfully
      echo "Done."
    else
      Error "Removal of unused files didn't finish successfully."
    fi
  else
    echo "There were no unused files detected."
  fi
  rm -f $TMP_OLD_UNINFO $TMP_NEW_UNINFO $TMP_UNFILES
fi

## Compilation
echo
echo "# 10/11 - Compiling. (The time of this step depends on the number of new source files that were downloaded.)"
unsermake
if [ "$?" != "0" ]; then # If the command didn't finish successfully
  Error "Compilation wasn't successful. amaroK was NOT installed/upgraded. Ask the friendly people in #amarok on freenode for help."
fi
echo
echo "Compilation successful."

## Installation
echo
echo "# 11/11 - Installing files."
echo "Will use '$HOW_ROOT' to get root privileges for installation."
if [ "$HOW_ROOT" = "sudo" ]; then
  echo "(You might need to enter your password now.)"
  sudo `which unsermake` install
elif [ "$HOW_ROOT" = "su -c" ]; then
  echo "(You probably have to enter the root password now.)"
  su -c "`which unsermake` install"
else
  kdesu -t `which unsermake` install
fi
if [ "$?" = "0" ]; then # If the command did finish successfully
  echo
  echo "# DONE - amaroK was successfully installed/updated! Start it from your menu or by typing 'amarok'."
  kdialog --title "$KD_TITLE" --msgbox "Done!\namaroK was successfully installed/updated!\nStart it from your menu or by typing \"amarok\"."
  exit 0 # Exit succsessfully
else
  echo
  Error "amaroK was compiled but NOT installed.\nIf the errors above seem to be permission errors, you could try installing amaroK manually.\n(You also might want to have a look at the settings for this script by starting it with -r or --reset.)\nTo install manually, get root privileges in some way and then run 'unsermake install' in the '`pwd`' directory."
fi
