#!/usr/bin/env python

############################################################################
# Config dialog for alarm script
# (c) 2005 Mark Kretschmann <markey@web.de>
#
# Depends on: PyQt
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

import sys
import os.path
from qt import *


class config( QDialog ):

    def __init__( self ):
        QDialog.__init__( self )
        self.setCaption( "Alarm Script - amaroK" )

        lay = QHBoxLayout( self )

        vbox = QVBox( self )
        lay.addWidget( vbox )

        htopbox = QHBox( vbox )
        QLabel( "Alarm time: ", htopbox )
        timeEdit = QTimeEdit( htopbox )

        hbox = QHBox( vbox )

        ok = QPushButton( hbox )
        ok.setText( "Ok" )

        cancel = QPushButton( hbox )
        cancel.setText( "Cancel" )
        cancel.setDefault( True )

        self.connect( ok,     SIGNAL( "clicked()" ), self, SLOT( "accept()" ) )
        self.connect( cancel, SIGNAL( "clicked()" ), self, SLOT( "reject()" ) )

        self.adjustSize()


############################################################################

def main( args ):
    app = QApplication( args )

    widget = config()
    widget.show()

    app.connect( app, SIGNAL( "lastWindowClosed()" ), app, SLOT( "quit()" ) )
    app.exec_loop()


if __name__ == "__main__":
    main( sys.argv )

