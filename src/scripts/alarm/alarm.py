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
import sys
from os import *
from qt import *


class Alarm( QApplication ):

    def __init__( self, args ):
        QApplication.__init__( self, args )
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

def main( args ):
    app = Alarm( args )

    app.exec_loop()

if __name__ == "__main__":
    main( sys.argv )

