#!/usr/bin/env python

############################################################################
# Python wrapper script for running the Amarok LiveCD remastering scripts
# from within Amarok.  Based on the Python-Qt template script for Amarok
# (c) 2005 Mark Kretschmann <markey@web.de>
# 
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
import signal
from time import sleep

try:
    from qt import *
except:
    os.popen( "kdialog --sorry 'PyQt (Qt bindings for Python) is required for this script.'" )
    raise


# Replace with real name
debug_prefix = "LiveCD Remastering"


class ConfigDialog ( QDialog ):
    """ Configuration widget """

    def __init__( self ):
        QDialog.__init__( self )
        self.setWFlags( Qt.WDestructiveClose )
        self.setCaption("Amarok Live! Configuration")

        self.lay = QGridLayout( self, 3, 2)

        self.lay.addColSpacing( 0, 300 )

        self.isopath = QLineEdit( self )
        self.isopath.setText( "Path to Amarok Live! iso" )
        self.tmppath = QLineEdit( self )
        self.tmppath.setText( "Temporary directory used, 2.5gb free needed" )

        self.lay.addWidget( self.isopath, 0, 0 )
        self.lay.addWidget( self.tmppath, 1, 0 )

        self.isobutton = QPushButton( self )
        self.isobutton.setText("Browse..." )
        self.tmpbutton = QPushButton( self )
        self.tmpbutton.setText("Browse..." )

        self.cancel = QPushButton( self )
        self.cancel.setText( "Cancel" )
        self.ok = QPushButton( self )
        self.ok.setText( "Ok" )

        self.lay.addWidget( self.isobutton, 0, 1 )
        self.lay.addWidget( self.tmpbutton, 1, 1 )
        self.lay.addWidget( self.cancel, 2, 1 )
        self.lay.addWidget( self.ok, 2, 0)

        self.connect( self.isobutton, SIGNAL( "clicked()" ), self.browseISO )
        self.connect( self.tmpbutton, SIGNAL( "clicked()" ), self.browsePath )

        self.connect( self.ok, SIGNAL( "clicked()" ), self.save )
        self.connect( self.ok, SIGNAL( "clicked()" ), self.unpack )
#        self.connect( self.ok, SIGNAL( "clicked()" ), self.destroy )
        self.connect( self.cancel, SIGNAL( "clicked()" ), self, SLOT("reject()") )

        self.adjustSize()

        path = None
        try:
            config = ConfigParser.ConfigParser()
            config.read( "remasterrc" )
            path = config.get( "General", "path" )
            iso = config.get( "General", "iso")
               
            if not path == "": self.tmppath.setText(path)
            if not iso == "": self.isopath.setText(iso)
        except:
            pass



    def save( self ):
        """ Saves configuration to file """
        self.file = file( "remasterrc", 'w' )
        self.config = ConfigParser.ConfigParser()
        self.config.add_section( "General" )
        self.config.set( "General", "path", self.tmppath.text() )
        self.config.set( "General", "iso", self.isopath.text() )
        self.config.write( self.file )
        self.file.close()

        self.accept()

    def clear():

        self.file = file( "remasterrc", 'w' )
        self.config = ConfigParser.ConfigParser()
        self.config.add_section( "General" )
        self.config.set( "General", "path", ""  )
        self.config.set( "General", "iso", "" )
        self.config.write( self.file )
        self.file.close()

    def browseISO( self ):

        path = QFileDialog.getOpenFileName( "/home",
                                                 "CD Images (*.iso)",
                                                 self,
                                                 "iso choose dialogr",
                                                 "Choose ISO to remaster")
        self.isopath.setText( path )

    def browsePath( self ):
       
        tmp = QFileDialog.getExistingDirectory( "/home",
                                                self,
                                                "get tmp dir",
                                                "Choose working directory",
                                                1)
        self.tmppath.setText( tmp )


    def unpack( self ):

        # now the fun part, we run part 1
        fd = os.popen("kde-config --prefix", "r")
        kdedir = fd.readline()
        kdedir = kdedir.strip()
        scriptdir = kdedir + "/share/apps/amarok/scripts/amarok_live"
        fd.close()

        path, iso = self.readConfig()
        os.system("kdesu -t sh %s/amarok.live.remaster.part1.sh %s %s" % (scriptdir, path, iso))
        #os.wait()
        print "got path: %s" % path




    def readConfig( self ) :
        path = ""
        iso = ""
        try:
            config = ConfigParser.ConfigParser()
            config.read("remasterrc")
            path = config.get("General", "path")
            iso = config.get("General", "iso")
        except:
            pass
        return (path, iso)



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
        os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Add playlist to livecd\"")
        os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Add selected to livecd\"")
        os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Create Remastered CD\"")
        os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Clear Music on livecd\"")
        
        os.system("dcop amarok script addCustomMenuItem \"Amarok live\" \"Add playlist to livecd\"")
        os.system("dcop amarok script addCustomMenuItem \"Amarok live\" \"Add selected to livecd\"")
        os.system("dcop amarok script addCustomMenuItem \"Amarok live\" \"Create Remastered CD\"")
        os.system("dcop amarok script addCustomMenuItem \"Amarok live\" \"Clear Music on livecd\"")


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
# events from Amarok:

    def configure( self ):
        debug( "configuration" )

        self.dia = ConfigDialog()
        self.dia.show()
        #self.connect( self.dia, SIGNAL( "destroyed()" ), self.readSettings )

    def clearCD( self ):
        
        self.dia = ConfigDialog()
        path, iso = self.dia.readConfig()

        os.system("rm -rf %s/amarok.live/music/* %s/amarok.live/playlist/* %s/amarok.live/home/amarok/.kde/share/apps/amarok/current.xml" % (path, path, path))

    def onSignal( self, signum, stackframe ):
        stop()

    def stop( self ):
        
        fd = open("/tmp/amarok.stop", "w")
        fd.write( "stopping")
        fd.close()

        os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Add playlist to livecd\"")
        os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Add selected to livecd\"")
        os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Create Remastered CD\"")
        os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Clear Music on livecd\"")


    def copyPlaylist( self ):

        self.dia = ConfigDialog()
        path, iso = self.dia.readConfig()
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
#        os.system("mkdir %s/amarok.live/home/amarok/.kde/share/apps/amarok/playlists/" % path)
        m3uOut = open("/tmp/amarok.live.%s.m3u" % suffix, 'w')

        m3u = m3u.strip()
        m3uOut.write(m3u)

        m3uOut.close()
        
        os.system("mv /tmp/amarok.live.%s.m3u %s/amarok.live/playlist/" % (suffix,path))
        os.system("rm /tmp/amarok.live.%s.m3u" % suffix)


        os.remove(tmpfileloc)

    def copyTrack( self, menuEvent ):

        event = str( menuEvent )
        debug( event )
        self.dia = ConfigDialog()

        path,iso = self.dia.readConfig()
        if path == "":
            os.system("kdialog --sorry 'You have not specified where the Amarok live iso is. Please click configure and do so first.'")
        else:
            # get the list of files. yes, its ugly. it works though.
            #files =  event.split(":")[-1][2:-1].split()[2:]
            #trying out a new one 
         #files = event.split(":")[-1][3:-2].replace("\"Amarok live!\" \"add to livecd\" ", "").split("\" \"")
            #and another
          
            files = event.replace("customMenuClicked: Amarok live Add selected to livecd", "").split()

            allfiles = ""
            for file in files:
                allfiles += file + " "
            allfiles = allfiles.strip()
            os.system("kfmclient copy %s file://%s/amarok.live/music/" % (allfiles, path))

    def createCD( self ):
        
        self.dia = ConfigDialog()
        path,iso = self.dia.readConfig()
        if path == "":
            os.system("kdialog --sorry 'You have not configured Amarok live! Please run configure.")

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
            self.dia.clear()
        fd.close()


############################################################################

def onSignal( signum, stackframe ):
    fd = open("/tmp/amarok.stop", "w")
    fd.write( "stopping")
    fd.close()

    print 'STOPPING'

    os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Add playlist to livecd\"")
    os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Add selected to livecd\"")
    os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Create Remastered CD\"")
    os.system("dcop amarok script removeCustomMenuItem \"Amarok live\" \"Clear Music on livecd\"")


def debug( message ):
    """ Prints debug message to stdout """

    print debug_prefix + " " + message

def main():
    app = Remasterer( sys.argv )

    # not sure if it works or not...  playing it safe
    dia = ConfigDialog()

    app.exec_loop()

if __name__ == "__main__":

    mainapp = threading.Thread(target=main)
    mainapp.start()
    signal.signal(15, onSignal)
    print signal.getsignal(15)
    while 1: sleep(120)

    #main( sys.argv )

