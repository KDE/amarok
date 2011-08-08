# XChat now playing script for Amarok 2
# by Sam Lade, 2010-2011
# This script is public domain.

# Usage:
# Place the script in ~/.xchat2/ and it will be loaded automatically on start
# (load manually with /py load xchat-nowplaying.py)
# Type /playing to post the currently playing track in the current channel.
# Adjust the format of the playing string to taste.
# Set a lastfmuser if desired and the script will link to your user page.
# Requires the python-dbus module.

import dbus, xchat
__module_name__ = "amarok-nowplaying"
__module_version__ = "0.1.2"
__module_description__ = "now playing script for Amarok 2"
print "Starting", __module_name__, __module_version__

lastfmuser = ""

def nowplaying_command_hook(word, word_eol, userdata):
    try:
        amarokbus = dbus.SessionBus().get_object("org.mpris.amarok", "/TrackList")
        tracknumber = amarokbus.GetCurrentTrack()
        data = amarokbus.GetMetadata(tracknumber)
        title = unicode(data[dbus.String("title")]).encode("utf-8")
        artist = unicode(data[dbus.String("artist")]).encode("utf-8")
        album = unicode(data[dbus.String("album")]).encode("utf-8")
        date = unicode(data[dbus.String("year")]).encode("utf-8")
        playing = 'is listening to "' + title + '" by ' + artist
        playing += ' [' + album + ", " + date + "]"
        if lastfmuser:
            playing += " -- see http://www.last.fm/user/" + lastfmuser + " for more"
        xchat.command("me " + playing)
        del amarokbus
    except dbus.exceptions.DBusException:
        print "DBus exception occurred; most likely Amarok is not running."
    finally:
        return xchat.EAT_ALL
xchat.hook_command("playing",nowplaying_command_hook)
