#!/usr/bin/env python
# -*- coding: Latin-1 -*-

"""
 This scripts starts a small http-server that serves the current-playlist as
 HTML. Start it and point your browser to http://localhost:4773/

 Author: André Kelpe fs111 at web dot de
 License: GPL

"""
import SimpleHTTPServer
import BaseHTTPServer
from Playlist import Playlist

# the port number to listen to
PORT = 4773


PLIST = None

class RequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    """We need our own 'RequestHandler, to handle the requests, that arrive at
    our server."""

    def do_GET(self):
        """Overwrite the do_GET-method of the super class."""
        self.send_response(200)
        self.send_header("content-type","text/html")
        self.end_headers()
        self.wfile.write(PLIST.toHtml())


def main():
    """main is the starting-point for our script."""
    global PLIST
    PLIST = Playlist()
    srv = BaseHTTPServer.HTTPServer(('',PORT),RequestHandler)
    srv.serve_forever()


if __name__ == "__main__":
    main()

