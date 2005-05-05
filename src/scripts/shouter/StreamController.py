############################################################################
# Root server and initial request binning
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

from BaseHTTPServer import BaseHTTPRequestHandler
from SocketServer import ThreadingTCPServer
from StreamConfig import *
from debug import *
import threading
from httplib import *
from ShouterExceptions import *
from Services import *
from StreamPublisher import *
from sre import match

INDEX_URL = 'index.pls'

class StreamController(BaseHTTPRequestHandler):
    stream = None
    stream_cfg = None
    server_version = 'amaroK shouter/0.2'

    def do_GET(self):
        path = self.path[1:]
        if path == INDEX_URL:
            Amarok.status('Sending service listing to %s' % self.request.getpeername()[0])
            self.send_response(200)
            self.send_header('Content-Type', 'audio/x-scpls')
            self.send_header('Connection', 'close')
            self.end_headers()
            url_base = 'http://%s:%d' % self.request.getsockname()
            pls =  '[playlist]\n'
            pls += 'numberofentries=%d\n' % len(StreamServer.cfg_mgr.stream_cfgs)
            i = 1
            for sc in StreamServer.cfg_mgr.stream_cfgs:
                pls += 'File%d=%s/%s\n' % (i, url_base, sc.mount)
                pls += 'Title%d=%s\n' % (i, sc.name)
                pls += 'Length%d\n' % i
                i += 1
            pls += 'Version=2\n'
            self.request.send(pls)
        else:
            service = None
            for sc in StreamServer.cfg_mgr.stream_cfgs:
                if sc.mount == path:
                    service = eval('Service%d(self, sc, StreamServer.cfg_mgr.server_cfg)' % sc.stream_type)
                    break
            if not service:
                if not path:
                    self.send_response(301)
                    self.send_header('Location', INDEX_URL)
                else:
                    self.send_response(404)
                    raise unmapped_mount_error
            else:
                Amarok.status('Starting stream for %s on %s' % (self.request.getpeername()[0], sc.mount))
                service.start()
                raise service_ended_error

    def do_PROPFIND(self):
        debug('StreamController do_PROPFIND')
        
        self.send_response(207, 'Multi-Status')
        self.send_header('Content-Type', 'text/xml; charset="utf-8"')
        resp = """
        <?xml version="1.0" encoding="utf-8" ?>
        <D:multistatus xmlns:D="DAV:">
        """.lstrip()
        url_base = 'http://%s:%d' % self.request.getsockname()
        if match(r'/?', self.path):
            for sc in StreamServer.cfg_mgr.stream_cfgs:
                resp += '<D:response>'
                resp += '<D:href>%s/%s</D:href>' % (url_base, sc.mount)
                resp += '<propstat>'
                resp += '<displayname>%s</displayname>' % sc.name
                resp += '</propstat>'
                resp += '</D:response>'
        resp +='</D:multistatus>'
        self.send_header('Content-Length', len(resp))
        self.end_headers()
        debug(resp)
        self.request.send(resp)
        
    def do_HEAD(self):
        debug('StreamController do_HEAD')

class StreamServer(ThreadingTCPServer):
    log_f = None
    cfg_mgr = None
    sockets = []

    def verify_request(self, request, client_address):
        if len(self.sockets) < self.cfg_mgr.server_cfg.max_clients:
            self.sockets.append(request)
            return True
        else:
            debug('Rejecting request. Sending 503')
            request.send('HTTP/1.1 503 Server is full')
            return False

    def handle_error(self, request, client_address):
        try:
            debug('handle_error trying to remove request')
            self.sockets.remove(request)
            self.log( 'Dropping socket to %s. Active sockets: %d' % (client_address[0], len(self.sockets)))
            #Amarok.status('Dropping connection to %s' % client_address[0])
        except:
            debug('error in handle_error')
        # FIXME: Don't do this if error came from an unmapped mount or the death
        # of a service
        ThreadingTCPServer.handle_error(self, request, client_address)

    def log(self, msg):
        self.log_f.write(time.strftime('[%Y.%m.%d - %H:%M:%s] ') + str(msg) + '\n')
        self.log_f.flush()

    def run(self):
        self.log_f = open('access.log', 'a')
        publisher.cfg_mgr = self.cfg_mgr
        publisher.port = self.server_address[1]
        threading.Thread(target = publisher.run).start()
        self.serve_forever()

class Stream:
    cfg = None
    def __init__(self, socket, cfg):
        self.cfg = cfg

class IcecastStream(Stream):
    udp_port = 6000
    pass

class ShoutcastStream(Stream):
    pass

