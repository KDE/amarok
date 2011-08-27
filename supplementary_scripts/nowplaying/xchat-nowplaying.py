#! /usr/bin/env python

# XChat now playing script for Amarok 2
# by Sam Lade, 2010-2011
# This script is public domain.

# Usage:
# In XChat:
# Place the script in ~/.xchat2/ and it will be loaded automatically on start
# (load manually with /py load xchat-nowplaying.py)
# Type /playing to post the currently playing track in the current channel.
#
# For other clients:
# Use with /exec -out command, e.g. to bind (in irssi):
# /alias np exec -out - /home/myself/xchat-nowplaying.py
#
# Adjust the format of the playing string to taste.
# Set a lastfmuser if desired and the script will link to your user page.
# Requires the python-dbus module.
lastfmuser = ""

import dbus

def get_playing():
    try:
        amarokbus = dbus.SessionBus().get_object("org.mpris.amarok", "/TrackList")
        tracknumber = amarokbus.GetCurrentTrack()
        data = amarokbus.GetMetadata(tracknumber)
        title = unicode(data[dbus.String("title")]).encode("utf-8")
        artist = unicode(data[dbus.String("artist")]).encode("utf-8")
        album = unicode(data[dbus.String("album")]).encode("utf-8")
        date = unicode(data[dbus.String("year")]).encode("utf-8")

        if not title:
            title = u"?"

        if artist:
            artist = " by {0}".format(artist)

        if (album != "") & (date != ""):
            extra = " [{0}, {1}]".format(album, date)
        elif (album != "") | (date != ""):
            extra = " [{0}{1}]".format(album, date)
        else:
            extra = ""

        if lastfmuser:
            lastfm = " -- see http://www.last.fm/user/{0} for more".format(lastfmuser)
        else:
            lastfm = ""

        playing = "{0}{1}{2}{3}".format(title, artist, extra, lastfm)
        del amarokbus
        return playing
    except dbus.exceptions.DBusException:
        return "__DBusException__"


try:
    import xchat

except ImportError:
    res = get_playing()
    if res != "__DBusException__":
        print "Playing:", res
    else:
        import sys
        sys.exit(1)

else:
    __module_name__ = "amarok-nowplaying"
    __module_version__ = "0.1.3"
    __module_description__ = "now playing script for Amarok 2"
    print "Starting", __module_name__, __module_version__


    def nowplaying_command_hook(word, word_eol, userdata):
        res = get_playing()
        if res != "__DBusException__":
            xchat.command("me " + "is listening to " + res)
        else:
            print "DBus exception occurred; most likely Amarok is not running."
        return xchat.EAT_ALL

    xchat.hook_command("playing",nowplaying_command_hook)
