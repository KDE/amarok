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
import sys
import threading
from os import *
from qt import *


class ConfigDialog( QDialog ):

    def __init__( self ):
        QDialog.__init__( self )
        self.setCaption( "Alarm Script - amaroK" )

        lay = QHBoxLayout( self )

        vbox = QVBox( self )
        lay.addWidget( vbox )

        htopbox = QHBox( vbox )
        QLabel( "Alarm time: ", htopbox )
        self.timeEdit = QTimeEdit( htopbox )

        hbox = QHBox( vbox )

        ok = QPushButton( hbox )
        ok.setText( "Ok" )

        cancel = QPushButton( hbox )
        cancel.setText( "Cancel" )
        cancel.setDefault( True )

        self.connect( ok,     SIGNAL( "clicked()" ), self.save )
        self.connect( cancel, SIGNAL( "clicked()" ), self, SLOT( "reject()" ) )

        self.adjustSize()

    def save( self ):
        wakeTime = str( self.timeEdit.time().toString() )
        print wakeTime

        file = open( "alarmrc", "w" )

        config = ConfigParser()
        config.add_section( "General" )
        config.set( "General", "alarmtime", wakeTime)
        config.write( file )

        file.close()

        self.accept()


class Alarm( QApplication ):

    def __init__( self, args ):
        QApplication.__init__( self, args )

        self.queue = Queue.Queue()

        self.timer = QTimer()
        self.connect( self.timer, SIGNAL( "timeout()" ), self.checkQueue )
        self.timer.start( 100 )

        self.t = threading.Thread( target = self.readStdin )
        self.t.start()

        config = ConfigParser()
        config.read( "alarmrc" )

        timestr = config.get( "General", "alarmtime" )
        print "Alarm Time: " + timestr

        time = QTime.fromString( timestr )
        secondsleft = QTime.currentTime().secsTo( time )

        QTimer.singleShot( secondsleft * 1000, self.wakeup )

    def wakeup( self ):
        popen( "dcop amarok player play" )
        self.quit()


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

    def checkQueue( self ):
        if not self.queue.empty():
            string = QString( self.queue.get_nowait() )

            if string.contains( "configure" ):
                self.configure()

    def configure( self ):
        print "Alarm Script: configuration"

        self.dia = ConfigDialog()
        self.dia.show()



############################################################################

def main( args ):
    app = Alarm( args )

    app.exec_loop()

if __name__ == "__main__":
    main( sys.argv )

