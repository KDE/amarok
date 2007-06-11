#!/usr/bin/env python
# -*- coding: Latin-1 -*-
import user
import os
from xml.dom import minidom


# the current.xml file
PLAYLISTFILE = "%s/.kde/share/apps/amarok/current.xml"%(user.home)

# the fields to be shown via http
FIELDS = ("Artist", "Title", "Album", "Track", "Length", "Genre",  "Score" )

class Track(object):
    """Class that holds the information of one track in the current playlist"""
    __slots__ = FIELDS

    def __init__(self, **kwargs):
        for key,value in kwargs.iteritems():
            setattr(self, key, value)


    def toRow(self, style=''):
        """Returns the a html-table-row with member values of this class"""
        tmp = ['<td>%s</td>'%(i) for i in [getattr(self,f) for f in \
                self.__slots__ ]]
        tr_style = ''        
        if style:
            tr_style='class="%s"'%(style)
        return '<tr %s>%s</tr>'%(tr_style, ''.join(tmp))        


class Playlist:
    """The Playlist class represents the Playlist as one object. It holds all
    needed information and does most of the work"""

    def __init__(self):
        """The Constructor takes no arguments."""
        self.sync()
        self.tracks = []
        self.mtime = self._getMtime()
        self.firstRun = 1
        self.code=""" <html>\n
                    <head>
                    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
                    <style>
                    body {
                        font-size:10pt;
                        margin-left:5%%;
                        margin-right:5%%;
                        background-color: #9cb2cd;
                        color: white;
                        }
                     table{border:1px white;}   
                    .tr_one { background-color:#394062; }
                    .tr_two { background-color:#202050;}
                    th{color:dark-grey;}
                    .window {background-color:#d9d9d9; border: 1px solid black;
                    padding:15px;}
                    a:link { color:#074876; border:0px; text-decoration:none; }
                    a:visited { color:#074876; text-decoration:none; text-decoration:none; }

                    a:active { color:#074876; text-decoration:none; text-decoration:none; } 
                    a:hover { text-decoration:none; text-decoration:none; background-color:#074876;
                            color:white; }
                    </style>
                    
                    </head> 
                    <title>AmaroK playlist</title>\n <body>
                    <div style="text-align:center;font-size:16pt;" >current
                    <a href="http://amarok.kde.org" title="visit amarok hompage">amarok</a>
                    playlist of %s</div>
                    \n
                    <div class="window">%s</div> </body>\n
                </html>\n
              """
        self.encoding = "UTF-8"
        self.fullPage = ""
        self._buildDoc()



    def toHtml(self):
        """Returns a html representation of the whole Playlist"""
        if self.firstRun:
            self._setFullPage()
            self.firstRun = 0
        else:    
            newmtime = self._getMtime()    
            if newmtime !=self.mtime:
                self._buildDoc()
                self._setFullPage()
                self.mtime = newmtime
        return self.fullPage    


    def _setFullPage(self):
        self.fullPage = self.code%(os.environ['LOGNAME'],(self._createTable()).encode(self.encoding,'replace'))


    def _getMtime(self):
        """gets the mtime from the current.xml file, to check if the current.xml
        has been modified or not"""
        return os.stat(PLAYLISTFILE)[-2]
    

    def _buildDoc(self):
        """Build the DOM-doc and calls the _extractTracks-Method"""
        self.doc = minidom.parse(PLAYLISTFILE)
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

        tbl = """<table width="100%%" cellpadding="5px" cellspacing="2">
        <!--<colgroup> <col width="150"> <col width="330"> <col width="150">
        <col width="10"> <col width="30"> <col width="10"> </colgroup>-->
        %s%s</table>"""
        rows = self._createRows() 
        thead = "".join(['<th>%s</th>'%(i) for i in FIELDS])
        return tbl  %(thead,"".join(rows))            

    def _createRows(self):
        """Returns the table rows"""
        retval = []
        i = 1
        for track in self.tracks:
            style = 'tr_one'
            if i %2 == 0:
                style = 'tr_two'
            retval.append(track.toRow(style=style))
            i = i+1
        return retval 
    
    def sync(self):
        """Saves the current Amarok-Playlist to the current.xml file. Calls
        amarok via dcop"""
        os.system("dcop amarok playlist saveCurrentPlaylist")
