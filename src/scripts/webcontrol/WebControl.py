#!/usr/bin/env python
############################################################################
# WebControl script for amaroK
# (c) 2005 Jonas Drewsen <kde@xspect.dk>
#
# Depends on: Python 2.2, PyQt
#
############################################################################
# Based on
# Python-Qt template script for amaroK
# (c) 2005 Mark Kretschmann <markey@web.de>
#
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

import Globals
from Playlist import Playlist
import RequestHandler
import BaseHTTPServer

import time

# necessary for <= python2.2 that cannot handle "infds" in var
import string

try:
    from qt import *
except:
    os.popen( "kdialog --sorry 'PyQt (Qt bindings for Python) is required for this script.'" )
    raise


# Replace with real name
debug_prefix = "[WebControl Script]"


class ConfigDialog( QDialog ):
    """ Configuration widget """

    def __init__( self ):
        QDialog.__init__( self )
        self.setWFlags( Qt.WDestructiveClose )
        self.setCaption( "WebControl - amaroK" )

        self.config = ConfigParser.ConfigParser()

        allowControl = RequestHandler.AmarokStatus.allowControl
        try:
            config = ConfigParser.ConfigParser()
            config.read( "webcontrolrc" )
            allowControl = string.find(config.get( "General", "allowcontrol" ), "True") >= 0
        except:
            pass

        self.lay = QHBoxLayout( self )

        self.vbox = QVBox( self )
        self.lay.addWidget( self.vbox )

        self.hbox1 = QHBox( self.vbox )

        self.allowControl = QCheckBox( QString("Allow control"), self.hbox1 )
        self.allowControl.setChecked(allowControl)

        self.hbox = QHBox( self.vbox )

        self.ok = QPushButton( self.hbox )
        self.ok.setText( "Ok" )

        self.cancel = QPushButton( self.hbox )
        self.cancel.setText( "Cancel" )
        self.cancel.setDefault( True )

        self.connect( self.ok,     SIGNAL( "clicked()" ), self.save )
        self.connect( self.cancel, SIGNAL( "clicked()" ), self, SLOT( "reject()" ) )

        self.adjustSize()

    def save( self ):
        """ Saves configuration to file """

        self.file = file( "webcontrolrc", 'w' )

        self.config = ConfigParser.ConfigParser()
        self.config.add_section( "General" )
        self.config.set( "General", "allowcontrol", self.allowControl.isChecked() )
        self.config.write( self.file )
        self.file.close()
        debug( "Saved config" )
        self.accept()


class Notification( QCustomEvent ):
    __super_init = QCustomEvent.__init__
    def __init__( self, str ):
        self.__super_init(QCustomEvent.User + 1)
        self.string = str

class WebControl( QApplication ):
    """ The main application, also sets up the Qt event loop """

    def __init__( self, args ):
        QApplication.__init__( self, args )
        debug( "Started." )

        self.readSettings()

        self.t = threading.Thread( target = self.readStdin )
        self.t.start()

        RequestHandler.PLIST = Playlist()

        self.srv = BaseHTTPServer.HTTPServer(('',Globals.PORT),RequestHandler.RequestHandler)

        self.snsrv = QSocketNotifier(self.srv.fileno(), QSocketNotifier.Read)
        self.snsrv.connect( self.snsrv, SIGNAL('activated(int)'), self.readSocket )

    def readSocket( self ):
        # Got a read event on the HTTP server socket.
        self.srv.handle_request()

    def readSettings( self ):
        """ Reads settings from configuration file """
        config = ConfigParser.ConfigParser()
        config.read( "webcontrolrc" )

        try:
            RequestHandler.AmarokStatus.allowControl = string.find(config.get( "General", "allowcontrol" ), "True") >= 0
        except:
            debug( "No config file found, using defaults." )


############################################################################
# Stdin-Reader Thread
############################################################################

    def readStdin( self ):
        while True:
            line = sys.stdin.readline()

            if line:
                qApp.postEvent( self, Notification(line) )
            else:
                break

############################################################################
# Notification Handling
############################################################################

    def customEvent( self, notification ):
        """ Handles the notifications """

        string = QString(notification.string)
        debug( "Received notification: " + str( string ) )

        if string.contains( "configure" ):
            self.configure()

        elif string.contains( "engineStateChange: play" ):
            self.engineStatePlay()

        elif string.contains( "engineStateChange: idle" ):
            self.engineStateIdle()

        elif string.contains( "engineStateChange: pause" ):
            self.engineStatePause()

        elif string.contains( "engineStateChange: empty" ):
            self.engineStatePause()

        elif string.contains( "trackChange" ):
            self.trackChange()

        else:
            debug( "Unknown notification: " + str(string) + " -> ignoring")

# Notification callbacks. Implement these functions to react to specific notification
# events from amaroK:

    def configure( self ):
        debug( "configuration" )

        self.dia = ConfigDialog()
        self.dia.show()
        self.connect( self.dia, SIGNAL( "destroyed()" ), self.readSettings )

    def engineStatePlay( self ):
        """ Called when Engine state changes to Play """
        RequestHandler.AmarokStatus.dcop_trackcurrenttime = Globals.PlayerDcop("trackCurrentTime")
        RequestHandler.AmarokStatus.dcop_trackcurrenttime.result()
        RequestHandler.AmarokStatus.dcop_tracktotaltime = Globals.PlayerDcop("trackTotalTime")
        RequestHandler.AmarokStatus.dcop_tracktotaltime.result()
        RequestHandler.AmarokStatus.playState = RequestHandler.AmarokStatus.EnginePlay

    def engineStateIdle( self ):
        """ Called when Engine state changes to Idle """
        RequestHandler.AmarokStatus.playState = RequestHandler.AmarokStatus.EngineIdle

    def engineStatePause( self ):
        """ Called when Engine state changes to Pause """
        RequestHandler.AmarokStatus.playState = RequestHandler.AmarokStatus.EnginePause

    def engineStateEmpty( self ):
        """ Called when Engine state changes to Empty """
        RequestHandler.AmarokStatus.playState = RequestHandler.AmarokStatus.EngineEmpty

    def trackChange( self ):
        """ Called when a new track starts """
        RequestHandler.AmarokStatus.dcop_trackcurrentindex = Globals.PlaylistDcop("getActiveIndex")
        RequestHandler.AmarokStatus.dcop_trackcurrentindex.result()
        RequestHandler.AmarokStatus.dcop_trackcurrenttime = Globals.PlayerDcop("trackCurrentTime")
        RequestHandler.AmarokStatus.dcop_trackcurrenttime.result()
        RequestHandler.AmarokStatus.dcop_tracktotaltime = Globals.PlayerDcop("trackTotalTime")
        RequestHandler.AmarokStatus.dcop_tracktotaltime.result()


############################################################################

def debug( message ):
    """ Prints debug message to stdout """

    print debug_prefix + " " + message

def main( args ):

    Globals.EXEC_PATH = os.path.abspath(sys.path[0])

    app = WebControl( args )
    app.exec_loop()

if __name__ == "__main__":
    main( sys.argv )


