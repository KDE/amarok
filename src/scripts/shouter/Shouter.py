#!/usr/bin/env python

############################################################################
# Main executable. Must be run by amaroK
# (c) 2005 James Bellenger <jbellenger@pristine.gm>
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

try:
    from qt import *
except:
    os.popen( "kdialog --sorry 'PyQt (Qt bindings for Python) is required for this script.'" )
    raise

import ConfigParser
import os
import sys
import Globals
import threading
from StreamController import * 
from ShouterRequest import * 
from time import sleep
from StreamConfig import *
from ShouterConfig import * 
import urllib
from debug import *
from sre import sub 
import socket

class Notification( QCustomEvent ):
    __super_init = QCustomEvent.__init__
    def __init__( self, str ):
        self.__super_init(QCustomEvent.User + 1)
        self.string = str

class Shouter( QApplication ):
    """ The main application, also sets up the Qt event loop """

    stream_ctrl = None
    file, oldfile = None, None
    cfg = StreamConfig

    def __init__( self, args ):
        QApplication.__init__( self, args )
        debug( 'starting' )

        # Start separate thread for reading data from stdin
        self.stdinReader = threading.Thread( target = self.readStdin )
        self.stdinReader.start()
        self.readSettings()
        self._run_streamer()

    def readSettings( self ):
        """ Reads settings from configuration file """

        config = ConfigParser.ConfigParser()
        config.read( 'shouterrc' )

        try:
            self.cfg.genre              = config.get( 'Stream', 'genre' )
            self.cfg.mount              = config.get( 'Stream', 'mount' )
            self.cfg.name               = config.get( 'Stream', 'name' )
            self.cfg.url                = config.get( 'Stream', 'url' )
            self.cfg.desc1              = config.get( 'Stream', 'desc1' )
            self.cfg.desc2              = config.get( 'Stream', 'desc2' )

            self.cfg.icy_interval       = config.getint( 'Server', 'icy_interval' )
            self.cfg.max_clients        = config.getint( 'Server', 'max_clients' )
            self.cfg.port               = config.getint( 'Server', 'port' )
            self.cfg.buf_size           = config.getint( 'Server', 'buf_size' )
            self.cfg.punc_factor        = config.getint( 'Server', 'punc_factor' )
            self.cfg.force_update       = config.getboolean( 'Server', 'force_update' )
            self.cfg.pre_seek           = config.getfloat( 'Server', 'pre_seek' )
            self.cfg.supress_dialog     = config.getboolean( 'Server', 'supress_dialog' )

            self.cfg.enable_dl          = config.getboolean( 'Downloads', 'enable_dl' )
            self.cfg.dl_mount           = config.get( 'Downloads', 'dl_mount' )
            self.cfg.dl_throttle        = config.getint( 'Downloads', 'dl_throttle' )

            self.cfg.reencoding         = config.getint( 'Encoding', 'reencoding' )
            self.cfg.format             = config.get( 'Encoding', 'stream_format' )
            self.cfg.stream_br          = config.getint( 'Encoding', 'stream_br' )
            self.cfg.chunk_size         = config.getint( 'Encoding', 'chunk_size' )

            if self.stream_ctrl is not None: self.stream_ctrl.cfg = self.cfg

        except:
            debug( 'Exception in loading from config, using defaults.' )

    def runBuiltIn( self ):
        pass


    def _run_streamer( self ):
        self.stream_ctrl = None
        port_i = self.cfg.port
        while self.cfg.port - port_i <= 10 :
            try:
                debug('creating StreamController: reencoding=%s' % self.cfg.reencoding)
                stream_ctrl = StreamController(('', self.cfg.port), ShouterRequest)
                stream_ctrl.cfg = self.cfg
                stream_ctrl.playlist = []
                playing = self.get_file()
                stream_ctrl.add(playing)

                debug( '%d: playlist = %s' % (self.cfg.port, str(stream_ctrl.playlist)) )
                self.m = QMessageBox("Amarok Shouter","Starting server on http://%s:%d%s" % (socket.gethostname(), self.cfg.port, self.cfg.mount), QMessageBox.Information,QMessageBox.Ok,QMessageBox.NoButton,QMessageBox.NoButton, None, "", False, QWidget.WDestructiveClose)
                if not self.cfg.supress_dialog: self.m.show()
                self.stream_ctrl = stream_ctrl

                threading.Thread(target = self.stream_ctrl.run).start()
                break
            except Exception, ex:
                debug( ex.args )
                self.cfg.port += 1
        if self.stream_ctrl is None: 
            if not self.cfg.supress_dialog: 
                QMessageBox.critical(None,"Amarok Shouter","Server failed to start")
            sys.exit(1)
        
        


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

        if string.contains( "engineStateChange: play" ):
            self.engineStatePlay()

        if string.contains( "engineStateChange: idle" ):
            self.engineStateIdle()

        if string.contains( "engineStateChange: pause" ):
            self.engineStatePause()

        if string.contains( "engineStateChange: empty" ):
            self.engineStateEmpty()

        if string.contains( "trackChange" ):
            self.trackChange()


    def configure( self ):
        self.dia = ConfigDialog()
        self.dia.show()
        self.connect( self.dia, SIGNAL( "destroyed()" ), self.readSettings )

    def engineStatePlay( self ):
        """ Called when Engine state changes to Play """
        pass

    def engineStateIdle( self ):
        """ Called when Engine state changes to Idle """
        pass

    def engineStatePause( self ):
        """ Called when Engine state changes to Pause """
        pass

    def engineStateEmpty( self ):
        """ Called when Engine state changes to Empty """
        if self.cfg.force_update: self.stream_ctrl.force_update()

    def trackChange( self ):
        """ Called when a new track starts """

        f = self.get_file()
        #self.stream_ctrl.playlist.append(f)
        if f is not None: 
            self.stream_ctrl.add(f)
            debug( "Appending %s" % f )
            if self.cfg.force_update: self.stream_ctrl.force_update()
            

    def get_file( self ):
        playing = Globals.PlayerDcop( "encodedURL" ).result()
        if playing.startswith( 'http' ):
            self.m = QMessageBox("Amarok Shouter", 'You appear to be playing a music stream. This script will attempt to restart after the current stream has been closed', QMessageBox.Warning,QMessageBox.Ok,QMessageBox.NoButton,QMessageBox.NoButton, None, "", False, QWidget.WDestructiveClose)
            if not self.cfg.supress_dialog: self.m.show()
            return None

        if len(playing.strip()) == 0:
            return None

        playing = urllib.url2pathname(sub('file:/*', '/', playing).rstrip())
        if len(playing) > 0 :
            return playing


############################################################################

def main( args ):
    app = Shouter( args )

    app.exec_loop()

if __name__ == "__main__":
    main( sys.argv )

