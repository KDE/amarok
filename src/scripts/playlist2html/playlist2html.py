#!/usr/bin/env python

"""
 This scripts starts a smal http-server that serves the current-playlist as
 HTML. Start it and point ypur browser to http://localhost:4773

 Author: André Kelpe fs111 at web dot de
 License: GPL

"""


import user
import os
import SimpleHTTPServer
import BaseHTTPServer
from xml.dom import minidom

# the current.xml file
PLAYLIST = "%s/.kde/share/apps/amarok/current.xml"%(user.home)

# the fields to be shown via http
FIELDS = ("Artist", "Title", "Album", "TrackNo", "Length", "Score")

# the port number to listen to
PORT = 4773


PLIST = None

class Track(object):
    """Class that holds the information of one track in the current playlist"""
    __slots__ = FIELDS

    def __init__(self, **kwargs):
        for key,value in kwargs:
            setattr(self, key,value)


    def toRow(self):
        """Returns the a html-table-row with member values of this class"""
        tmp = ['<td>%s</td>'%(i) for i in [getattr(self,f) for f in \
                self.__slots__ ]]
        return '<tr>%s</tr>'%(''.join(tmp))        




class RequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    """We need our own 'RequestHandler, to handle the requests, that arrive at
    our server."""

    def __init__(self, request, client_address, server):
#        print "DEBUG: init called"
#        self.playlist = Playlist()
        SimpleHTTPServer.SimpleHTTPRequestHandler.__init__(self, \
                request, client_address, server)




    def do_GET(self):
        """Overwrite the do_GET-method of the super class."""
        self.send_response(200)
        self.send_header("content-type","text/html")
        self.end_headers()
        self.wfile.write(PLIST.toHtml())



class Playlist:
    """The Playlist class represents the Playlist as one object. It holds all
    needed information and does most of the work"""

    def __init__(self):
        """The Constructor takes no arguments."""
        self.tracks = []
        self.mtime = self._getMtime()
        self.firstRun = 1
        self.code=""" <html>\n
                    <head>
                    <style>
                    body
                    {
                        font-size:10pt;
                        padding-left:5%%;
                        background-color: #9cb2cd;
                        color: white;
                    }
                    </style>
                    
                    </head> 
                    <title>AmaroK playlist</title>\n <body>\n %s </body>\n
                </html>\n
              """
        self.encoding = "ISO-8859-15"
        self.fullPage = ""
        self._buildDoc()



    def toHtml(self):
        """Returns a html representation of the whole Playlist"""
        if self.firstRun:
            self.fullPage = self.code%(self._createTable()).encode(self.encoding)
            self.firstRun = 0
        else:    
            newmtime = self._getMtime()    
            if newmtime !=self.mtime:
                self._buildDoc()
                self.fullPage = self.code%(self._createTable()).encode(self.encoding)
                self.mtime = newmtime
        return self.fullPage    


    def _getMtime(self):
        """gets the mtime from the current.xml file, to check if the current.xml
        has been modified or not"""
        return os.stat(PLAYLIST)[-2]
    

    def _buildDoc(self):
        """Build the DOM-doc and calls the _extractTracks-Method"""
        self.doc = minidom.parse(PLAYLIST)
        self._extractTracks()

    def _extractTracks(self):
        """extracts all "item"-elements from the doc and creates an Track-object
        to store the associated information"""
        append = self.tracks.append
        for item in self.doc.getElementsByTagName("item"):
            curTrack = Track()
            for elem in FIELDS:
                try:
                    value = item.getElementsByTagName(elem)[0].firstChild.nodeValue
                except:
                    value = '  '
                setattr(curTrack, elem, value)    
            append(curTrack)


    def _createTable(self):
        """Returns the HTML-Table"""

        tbl = """<table cellspacing="2">
        <colgroup> <col width="150"> <col width="330"> <col width="150">
        <col width="10"> <col width="30"> <col width="10"> </colgroup>
        %s%s</table>"""
        rows = [track.toRow() for track in self.tracks]
        thead = "".join(['<th>%s</th>'%(i) for i in FIELDS])
        return tbl  %(thead,"".join(rows))            




def main():
    """main is the starting-point for our script."""
    global PLIST
    PLIST = Playlist()
    srv = BaseHTTPServer.HTTPServer(('',PORT),RequestHandler)
    srv.serve_forever()


if __name__ == "__main__":
    main()


