############################################################################
# Extension of socket server
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
import os
import SocketServer
from sre import search,findall
from StreamConfig import *
import time
from debug import *
from ShouterRequest import *
#from Encoder import *
from FileProvider import *

class StreamController (SocketServer.ThreadingTCPServer):
    """ Controls stream threads and holds socket connections. 
    A central thread check-in point for the latest playlist"""

    sockets = dict()
    playlist = []
    coded_files = dict()
    playlist = dict()
    cfg = None
    
    log_f = None

    def add(self, fname):
        if fname:
            self.playlist.append(fname)
            self.coded_files[fname] = FileProvider(fname, self.cfg)
        
    def log( self, msg ):
        self.log_f.write( time.strftime('[%Y.%m.%d - %H:%M:%s] ') + str(msg) + '\n' )
        self.log_f.flush()
        
    def handle_error( self, request, client_address ):
        sr = self.sockets.pop(request)
        self.log( 'Dropping socket to %s. Active sockets: %d' % (client_address[0], len(self.sockets) ))
        Globals.status('Dropping connection to %s' % client_address[0])
        raise
        
    def finish_request(self, request, client_address):
        debug( 'Creating StreamRequest')
        sr = StreamRequest(request, client_address, self)

        if len(self.sockets) < self.cfg.max_clients : 
            Globals.status('Starting stream for %s' % client_address[0])
            self.sockets[request] = sr
            sr.run()
        else:
            self.log( 'Rejecting request from %s with active connections = %d' % (client_address[0], len(self.sockets)))
            sr._send_status(403)

    def force_update(self):
        """ Force all streams to advance to the last playlist file """

        debug( 'force_update' )
        for s in self.sockets:
            sr = self.sockets[s]
            sr.blind = False
            
    def run( self ):
        self.log_f = open( 'access.log', 'a' )
        self.serve_forever()

