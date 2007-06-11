#!/usr/bin/env python

############################################################################
# Config dialog for alarm script
# (c) 2005 Mark Kretschmann <markey@web.de>
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

from ConfigParser import *
import Queue
import os.path
import sys
import threading
from os import *

try:
    from qt import *
except:
    popen( "kdialog --sorry 'PyQt (Qt bindings for Python) is required for this script.'" )
    raise


class ConfigDialog( QDialog ):

    def __init__( self ):
        QDialog.__init__( self )
        self.setWFlags( Qt.WDestructiveClose )
        self.setCaption( "Alarm Script - Amarok" )

        self.lay = QHBoxLayout( self )

        self.vbox = QVBox( self )
        self.lay.addWidget( self.vbox )

        self.htopbox = QHBox( self.vbox )
        QLabel( "Alarm time: ", self.htopbox )
        self.timeEdit = QTimeEdit( self.htopbox )

        self.hbox = QHBox( self.vbox )

        self.ok = QPushButton( self.hbox )
        self.ok.setText( "Ok" )

        self.cancel = QPushButton( self.hbox )
        self.cancel.setText( "Cancel" )
        self.cancel.setDefault( True )

        self.connect( self.ok,     SIGNAL( "clicked()" ), self.save )
        self.connect( self.cancel, SIGNAL( "clicked()" ), self, SLOT( "reject()" ) )

        self.adjustSize()

    def __del__( self ):
        print "ConfigDialog dtor"

    def save( self ):
        wakeTime = str( self.timeEdit.time().toString() )
        print wakeTime

        self.file = file( "alarmrc", 'w' )

        self.config = ConfigParser()
        self.config.add_section( "General" )
        self.config.set( "General", "alarmtime", wakeTime)
        self.config.write( self.file )
        self.file.close()

        self.accept()


class Alarm( QApplication ):

    def __init__( self, args ):
        QApplication.__init__( self, args )

        self.queue = Queue.Queue()
        self.startTimer( 100 )

        self.t = threading.Thread( target = self.readStdin )
        self.t.start()

        self.alarmTimer = QTimer()
        self.connect( self.alarmTimer, SIGNAL( "timeout()" ), self.wakeup )

        self.readSettings()

    def __del__( self ):
        print "Alarm dtor"

    def wakeup( self ):
        popen( "dcop amarok player play" )
        self.quit()


    def readSettings( self ):
        config = ConfigParser()
        config.read( "alarmrc" )

        try:
            timestr = config.get( "General", "alarmtime" )
            print "Alarm Time: " + timestr

            time = QTime.fromString( timestr )
            secondsleft = QTime.currentTime().secsTo( time )

            if secondsleft > 0:
                self.alarmTimer.start( secondsleft * 1000, True )
        except:
            pass


############################################################################
# Stdin-Reader Thread
############################################################################

    def readStdin( self ):
        while True:
            line = sys.stdin.readline()

            if line:
                self.queue.put_nowait( line )
            else:
                break


############################################################################
# Command Handling
############################################################################

    def timerEvent( self, event ):
        if not self.queue.empty():
            string = QString( self.queue.get_nowait() )
            print "[Alarm Script] Received notification: " + str( string )

            if string.contains( "configure" ):
                self.configure()

    def configure( self ):
        print "Alarm Script: configuration"

        self.dia = ConfigDialog()
        self.dia.show()
        self.connect( self.dia, SIGNAL( "destroyed()" ), self.readSettings )


############################################################################

def main( args ):
    app = Alarm( args )

    app.exec_loop()

if __name__ == "__main__":
    main( sys.argv )

