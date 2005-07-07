#!/usr/bin/env python

############################################################################
# Python wrapper script for running the amaroK LiveCD remastering scripts
# from within amaroK.  Based on the Python-Qt template script for amaroK
# (c) 2005 Mark Kretschmann <markey@web.de>
# (c) 2005 Leo Franchi <lfranchi@gmail.com>
#
# Depends on: Python 2.2, PyQt
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

import ConfigParser
import os
import sys
import threading

try:
    from qt import *
except:
    popen( "kdialog --sorry 'PyQt (Qt bindings for Python) is required for this script.'" )
    raise


# Replace with real name
debug_prefix = "LiveCD Remastering"


class ConfigDialog:
    """ Configuration widget """

    def __init__( self ):
        path = None
        try:
            config = ConfigParser.ConfigParser()
            config.read( "remasterrc" )
            path = config.get( "General", "path" )
        except:
            pass



    def save( self, path ):
        """ Saves configuration to file """
        self.file = file( "remasterrc", 'w' )
        self.config = ConfigParser.ConfigParser()
        self.config.add_section( "General" )
        self.config.set( "General", "path", path )
        self.config.write( self.file )
        self.file.close()


    def readConfig( self ) :
        path = ""
        try:
            config = ConfigParser.ConfigParser()
            config.read("remasterrc")
            path = config.get("General", "path")
        except:
            pass
        return path



class Notification( QCustomEvent ):
    __super_init = QCustomEvent.__init__
    def __init__( self, str ):
        self.__super_init(QCustomEvent.User + 1)
        self.string = str

class Remasterer( QApplication ):
    """ The main application, also sets up the Qt event loop """

    def __init__( self, args ):
        QApplication.__init__( self, args )
        debug( "Started." )

        # Start separate thread for reading data from stdin
        self.stdinReader = threading.Thread( target = self.readStdin )
        self.stdinReader.start()

        self.readSettings()

        # ugly hack, thanks mp8 anyway
        os.system("dcop amarok script removeCustomMenuItem \"amaroK live\" \"Add playlist to livecd\"")
        os.system("dcop amarok script removeCustomMenuItem \"amaroK live\" \"Add selected to livecd\"")
        os.system("dcop amarok script removeCustomMenuItem \"amaroK live\" \"Create Remastered CD\"")
        os.system("dcop amarok script removeCustomMenuItem \"amaroK live\" \"Clear Music on livecd\"")
        
        os.system("dcop amarok script addCustomMenuItem \"amaroK live\" \"Add playlist to livecd\"")
        os.system("dcop amarok script addCustomMenuItem \"amaroK live\" \"Add selected to livecd\"")
        os.system("dcop amarok script addCustomMenuItem \"amaroK live\" \"Create Remastered CD\"")
        os.system("dcop amarok script addCustomMenuItem \"amaroK live\" \"Clear Music on livecd\"")


    def readSettings( self ):
        """ Reads settings from configuration file """

        try:
            path = config.get( "General", "path" )

        except:
            debug( "No config file found, using defaults." )


############################################################################
# Stdin-Reader Thread
############################################################################

    def readStdin( self ):
        """ Reads incoming notifications from stdin """

        while True:
            # Read data from stdin. Will block until data arrives.
            line = sys.stdin.readline()

            if line:
                qApp.postEvent( self, Notification(line) )
            else:
                break


############################################################################
# Notification Handling
############################################################################

    def customEvent( self, notification ):
        """ Handles notifications """

        string = QString(notification.string)
        debug( "Received notification: " + str( string ) )

        if string.contains( "configure" ):
            self.configure()
        if string.contains( "stop"):
            self.stop()

        elif string.contains( "customMenuClicked" ):
            if "selected" in string:
                self.copyTrack( string )
            elif "playlist" in string:
                self.copyPlaylist()
            elif "Create" in string:
                self.createCD()
            elif "Clear" in string:
                self.clearCD()


# Notification callbacks. Implement these functions to react to specific notification
# events from amaroK:

    def configure( self ):
        debug( "configuration" )

        self.dia = ConfigDialog()
        #self.dia.show()
        #self.connect( self.dia, SIGNAL( "destroyed()" ), self.readSettings )

        # now the fun part, we run part 1
        fd = os.popen("kde-config --prefix", "r")
        kdedir = fd.readline()
        kdedir = kdedir.strip()
        scriptdir = kdedir + "/share/apps/amarok/scripts/amarok_live"
        fd.close()

        os.system("kdesu -t sh %s/amarok.live.remaster.part1.sh" % scriptdir)
        #os.wait()
        fd = open("/tmp/amarok.script", 'r')
        path = fd.readline()
        print "got path: %s" % path
        fd.close()

        self.dia.save(path)

    def clearCD( self ):
        
        self.dia = ConfigDialog()
        path = self.dia.readConfig()

        os.system("rm -rf %s/amarok.live/music/* %s/amarok.live/home/amarok/.kde/share/apps/amarok/playlists %s/amarok.live/home/amarok/.kde/share/apps/amarok/current.xml" % (path, path, path))

    def stop( self ):
        
        fd = open("/tmp/amarok.stop", "w")
        fd.write( "stopping")
        fd.close()

        os.system("dcop amarok script removeCustomMenuItem \"amaroK live\" \"Add playlist to livecd\"")
        os.system("dcop amarok script removeCustomMenuItem \"amaroK live\" \"Add to livecd\"")
        os.system("dcop amarok script removeCustomMenuItem \"amaroK live\" \"Create Remastered CD\"")
        os.system("dcop amarok script removeCustomMenuItem \"amaroK live\" \"Clear Music on livecd\"")


    def copyPlaylist( self ):

        self.dia = ConfigDialog()
        path = self.dia.readConfig()
        if path == "":
            os.system("dcop amarok playlist popupMessage 'Please run configure first.'")
            return

        tmpfileloc = os.tmpnam()
        os.system("dcop amarok playlist saveM3u '%s' false" % tmpfileloc)
        tmpfile = open(tmpfileloc)

        import urllib

        files = ""
        m3u = ""
        for line in tmpfile.readlines():
            if line[0] != "#":
                
                line = line.strip()

                # get filename
                name = line.split("/")[-1]

               #make url
                url = "file://" + urllib.quote(line)

               #make path on livecd
                livecdpath = "/music/" + name

                files += url + " "
                m3u += livecdpath + "\n"

        tmpfile.close()

        files = files.strip()

        os.system("kfmclient copy %s file://%s/amarok.live/music/" % (files, path))

        import random
        suffix = random.randint(0,10000)
        os.system("mkdir %s/amarok.live/home/amarok/.kde/share/apps/amarok/playlists/" % path)
        m3uOut = open("/tmp/amarok.live.%s.m3u" % suffix, 'w')

        m3u = m3u.strip()
        m3uOut.write(m3u)

        m3uOut.close()
        
        os.system("mv /tmp/amarok.live.%s.m3u %s/amarok.live/home/amarok/.kde/share/apps/amarok/playlists/" % (suffix,path))
        os.system("rm /tmp/amarok.live.%s.m3u" % suffix)


        os.remove(tmpfileloc)

    def copyTrack( self, menuEvent ):

        event = str( menuEvent )
        debug( event )
        self.dia = ConfigDialog()

        path = self.dia.readConfig()
        if path == "":
            os.system("kdialog --sorry 'You have not specified where the amaroK live iso is. Please click configure and do so first.'")
        else:
            # get the list of files. yes, its ugly. it works though.
            #files =  event.split(":")[-1][2:-1].split()[2:]
            #trying out a new one 
         #files = event.split(":")[-1][3:-2].replace("\"amaroK live!\" \"add to livecd\" ", "").split("\" \"")
            #and another
          
            files = event.replace("customMenuClicked: amaroK live Add selected to livecd", "").split

            allfiles = ""
            for file in files:
                allfiles += file + " "
            allfiles = allfiles.strip()
            os.system("kfmclient copy %s file://%s/amarok.live/music/" % (allfiles, path))

    def createCD( self ):
        
        self.dia = ConfigDialog()
        path = self.dia.readConfig()
        if path == "":
            os.system("kdialog --sorry 'You have not configured amaroK live! Please run configure.")

        fd = os.popen("kde-config --prefix", "r")
        kdedir = fd.readline()
        kdedir = kdedir.strip()
        scriptdir = kdedir + "/share/apps/amarok/scripts/amarok_live"
        fd.close()

        os.system("kdesu sh %s/amarok.live.remaster.part2.sh %s" % (scriptdir, path))

        fd = open("/tmp/amarok.script", 'r')
        y = fd.readline()
        y = y.strip()
        if y == "end": # user said no more, clear path
            self.dia.save("")
        fd.close()


############################################################################

def debug( message ):
    """ Prints debug message to stdout """

    print debug_prefix + " " + message

def main( args ):
    app = Remasterer( args )

    # not sure if it works or not...  playing it safe
    dia = ConfigDialog()
    if dia.readConfig() == "":
        #app.configure()
        os.system("dcop amarok playlist popupMessage 'Please click configure to begin.'")


    app.exec_loop()

if __name__ == "__main__":
    main( sys.argv )

