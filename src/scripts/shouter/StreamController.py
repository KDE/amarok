import sys
import os
import SocketServer
from sre import search,findall
from StreamConfig import *
import time
from debug import *
from ShouterRequest import *
from Encoder import *

class StreamController (SocketServer.ThreadingTCPServer):
    """ Controls stream threads and holds socket connections. 
    A central thread check-in point for the latest playlist"""

    sockets = dict()
    playlist = []
    coded_files = dict()
    cfg = None
    
    log_f = None

    def add(self, fname):
        self.playlist.append(fname)
        if self.cfg.reencoding: 
            self.coded_files[fname] = Encoder(fname)
        
    def log( self, msg ):
        self.log_f.write( time.strftime('[%Y.%m.%d - %H:%M:%s] ') + str(msg) + '\n' )
        self.log_f.flush()
        
    def handle_error( self, request, client_address ):
        sr = self.sockets.pop(request)
        self.log( 'Dropping socket to %s. Active sockets: %d' % (client_address[0], len(self.sockets) ))
        raise
        
    def finish_request(self, request, client_address):
        debug( 'Finishing request, reencoding=%s' % self.cfg.reencoding )
        sr = None
        if self.cfg.reencoding: 
            debug( 'Creating ReencodedRequest')
            sr = ReencodedRequest(request, client_address, self)
        else: 
            debug( 'Creating StreamRequest')
            sr = StreamRequest(request, client_address, self)

        if len(self.sockets) < self.cfg.max_clients : 
            self.sockets[request] = sr
            sr.run()
        else:
            self.log( 'Rejecting request from %s with active connections = %d' % (client_address[0], len(self.sockets)))
            sr._send_status(403)

    def force_update(self):
        """ Force all streams to advance to the last playlist file """

        debug( 'force_update' )
        for s in self.sockets:
            debug( 'doing force_update on key=%s, value=%s' % (str(s), str(self.sockets[s])) )
            sr = self.sockets[s]
            sr.blind = False
            
    def run( self ):
        self.log_f = open( 'access.log', 'a' )
        self.serve_forever()
