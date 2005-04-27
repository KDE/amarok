#!/bin/env python

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

import sys
import threading
from debug import *
from StreamConfig import *
from ShouterConfig import *
from StreamController import *
import socket

try:
    from qt import *
except:
    os.popen( 'kdialog --sorry "PyQt (Qt bindings for Python) is required for this script."' )
    raise

class Notification(QCustomEvent):
    __super_init = QCustomEvent.__init__
    def __init__(self, str):
        self.__super_init(QCustomEvent.User + 1)
        self.string = str

class Shouter(QApplication):
    stream_server = None
    cfg_mgr = None

    def __init__(self, args):
        QApplication.__init__(self, args)
        threading.Thread(target = self.read_stdin).start()
        self.read_settings()
        self.run_streamer()

    def read_settings(self):
        debug('read_settings')
        self.cfg_mgr = ConfigManager()
        if self.stream_server:
            self.stream_server.cfg_mgr = self.cfg_mgr

    def read_stdin(self):
        """ Reads incoming notifications from stdin """

        while True:
            # Read data from stdin. Will block until data arrives.
            line = sys.stdin.readline()
            if line:
                qApp.postEvent(self, Notification(line))
            else:
                break

    def run_streamer(self):
        debug('run_streamer')
        self.stream_server = None
        server_cfg = self.cfg_mgr.server_cfg
        p_i = server_cfg.port
        p_incr = 0

        while p_incr < 10:
            try:
                stream_server = StreamServer(('', p_i + p_incr), StreamController)
                #stream_srv.cfgs = cfgs

                #Set a global ConfigManager instance in StreamServer.
                StreamServer.cfg_mgr = self.cfg_mgr
                threading.Thread(target = stream_server.run).start()
                msg = ''
                stream_cnt = len(self.cfg_mgr.stream_cfgs)
                p = p_i + p_incr
                if stream_cnt == 0:       
                    msg = 'Server started on port %d, but without any configured streams' % p
                elif stream_cnt == 1:  
                    msg = 'Serving 1 stream on port %d' % p
                else:
                    msg = 'Serving %d streams on port %d' % (stream_cnt, p)
                Amarok.status(msg)
                debug(msg)
                self.stream_server = stream_server
                break
            except socket.error:
                p_incr += 1
            
        if self.stream_server is None:
            Amarok.status('Failed to start server. Check debug output for details')
            sys.exit(1)


    def customEvent( self, notification ):
        """ Handles notifications """

        string = QString(notification.string)

        if string.contains( 'configure' ):
            self.configure()

        if string.contains('engineStateChange'):
            Amarok.on_engine_state_change(str(string))

        #if string.contains('engineStateChange: playing'):
            #Amarok.on_engine_state_change('playing')

        #if string.contains('engineStateChange: paused'):
            #Amarok.on_engine_state_change('paused')

        if string.contains( "trackChange" ):
            Amarok.on_track_change()

    def configure(self):
        debug('configure')
        self.dia = ConfigDialog()
        self.dia.show()
        self.connect( self.dia, SIGNAL( 'destroyed()' ), self.read_settings )


def main(args):
    app = Shouter(args)
    app.exec_loop()

if __name__ == '__main__':
    main(sys.argv)

