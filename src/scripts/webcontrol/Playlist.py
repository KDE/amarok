#!/usr/bin/env python
# -*- coding: Latin-1 -*-
#
# Author: André Kelpe fs111 at web dot de
#       : Jonas Drewsen kde at xspect dot dk
#
# License: GPL
#

import user
import os
from xml.dom import minidom
import time

# the current.xml file
PLAYLISTFILE = "%s/.kde/share/apps/amarok/current.xml"%(user.home)

# the fields to be shown via http
FIELDS = ("Artist", "Title", "Album", "TrackNo", "Length", "Genre",  "Score" )

class Track(object):
    """Class that holds the information of one track in the current playlist"""
    __slots__ = FIELDS

    def __init__(self, **kwargs):
        for key,value in kwargs.iteritems():
            setattr(self, key, value)


    def toRow(self, style='', id=None, trackno=None, reqid=None):
        """Returns the a html-table-row with member values of this class"""
        
        trno = str(trackno)
        astart = "<a "
        if id:
            astart += "name='nowplaying' "
        # append "#nowplaying" if we want to jump to current song.
        astart += "class='track' href='?action=goto&value=" + trno + "&" + reqid + "'>"
        aend = "</a>"
        
        tmp = [ '<td>' + astart + i + aend +'</td>' for i in [getattr(self,f) for f in self.__slots__ ] ]

        tr_style = ''        
        tr_id = ''
        
        if style:
            tr_style='class="%s"'%(style)

        if trackno == 0:
            tr_id = 'id="trackone"'

        if id:
            tr_id = 'id="%s"'%(id)
        
        return '<tr %s %s>%s</tr>'%(tr_style, tr_id, ''.join(tmp))        

    def getTitle(self):
        return getattr(self,"Title")

class Playlist:
    """The Playlist class represents the Playlist as one object. It holds all
    needed information and does most of the work"""

    def __init__(self):
        """The Constructor takes no arguments."""
        self.tracks = []
        self.code1=""" <html>\n
                    <head>\n
                    <meta http-equiv="Expires" content="0" />\n
                    <meta http-equiv="Pragma" content="no-cache" />\n
                    <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1" />\n
                    <link rel="stylesheet" href="main.css" type="text/css" />\n
                    <script type="text/javascript" src="main.js"></script>\n
                    </head>\n
                    <title>AmaroK playlist</title>\n
                    <body onload="anim(1);"""
                    
        self.code2 = """;setHeaders();" id="mybody">\n
                    <table class="topnavi" width="100%" cellpadding="0" cellspacing="0" border="0">\n
                    <tr><td valign="top" nowrap="nowrap"><div style="text-align:left;font-size:10">\n"""
        self.code3 = """</div>\n
                    </td><td align="right">\n
                    <div style="text-align:right;font-size:16pt;" >current
                    <a id="amaroklink" href="http://amarok.kde.org" title="visit amarok hompage">amarok</a>
                    playlist of """
        self.code4 = """</div>\n
                    </td></tr></table><div class'topspace'>&nbsp</div>\n"""
        self.code5 = """</body>\n
                </html>\n
              """
        self.encoding = "ISO-8859-15"
        self.fullPage = ""



    def toHtml(self, status):
        """Returns a html representation of the whole Playlist"""
        self.sync()
        self.tracks = []
        self.mtime = self._getMtime()
        self._buildDoc()
        self._setFullPage(status)
        return self.fullPage    


    def _createButton(self, name, action, reqid):
        return "<a href='?action=" + action + reqid + "'><img src='b_" + name + ".png'></a>"
    
    def _createVolume(self, vol_val, reqid):
        volume = '<table width="100" class="volume">'
        volume += "<tr>";

        for i in range(1,vol_val+1):
            volume += """<td class='volumeset'>
            <a href='?action=setvolume&value=""" + str(i*10) + reqid + "'>&nbsp;</a></td>"

        for i in range(vol_val+1, 11):
            volume += """<td class='volumeunset'>
            <a href='?action=setvolume&value=""" + str(i*10) + reqid + "'>&nbsp;</a></td>"

        volume += "</tr></table>"
        return volume
    
    def _createActions(self, status):


        if not status.controlsEnabled():
            return ""
        

        reqid = "&reqid=" + str(status.reqid)

        playpause = self._createButton("play", "play", reqid)

        if status.isPlaying():
            playpause = self._createButton("pause", "pause", reqid) 

        buttons = (self._createButton("prev", "prev", reqid) + "&nbsp;" +
                   playpause + "&nbsp;" +
                   self._createButton("stop", "stop", reqid) + "&nbsp;" +
                   self._createButton("next", "next", reqid))

        vol_val = int(float(status.getVolume()) / 10.0 + 0.5)
        volume = self._createVolume(vol_val, reqid)
        

        _gmtime = time.gmtime(status.timeLeft())

        hours = status.timeLeft() / (60*60)
        mins  = (status.timeLeft() % (60*60)) / 60
        secs  = status.timeLeft() - 60 * mins - 60 * 60 * hours

        hours_str = str(hours) + ":"
        if hours < 10:
            hours_str = "0" + hours_str
        if hours == 0:
            hours_str = ""

        mins_str = str(mins) + ":"
        if mins < 10:
            mins_str = "0" + mins_str

        secs_str = str(secs)
        if secs < 10:
            secs_str = "0" + secs_str

        time_left = hours_str + mins_str + secs_str

        actions = ("<table><tr><td>" +
                   buttons +
                   "</td><td><img width='9' height='8' src='vol_speaker.png'></td>" +
                   "<td>" + volume + "</td>" +
                   "<td width='16'>Time:<td><td id='countdown'>" + time_left +
                   "</td></tr></table>")
        return actions
    
    def _setFullPage(self, status):
        self.fullPage = self.code1
        if status.playState == status.EnginePlay:
            self.fullPage += "countdown(" + str(status.timeLeft()) + ");"
        self.fullPage += self.code2
        self.fullPage += self._createActions(status).encode(self.encoding)
        self.fullPage += self.code3
        self.fullPage += os.environ['LOGNAME']
        self.fullPage += self.code4
        self.fullPage += self._createTable(status).encode(self.encoding)
        self.fullPage += self.code5
        
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
                if elem == "Title":
                    value = value.strip()
                setattr(curTrack, elem, value)
            append(curTrack)


    def _createTable(self, status):
        """Returns the HTML-Table"""

        tbl = """<div class="trackshmove"></div>
        <table class="tracks" width="100%%" cellpadding="5px" cellspacing="2">
        <!--<colgroup> <col width="150"> <col width="330"> <col width="150">
        <col width="10"> <col width="30"> <col width="10"> </colgroup>-->
        %s%s</table>"""
        rows = self._createRows(status) 
        thead = "".join(['<th id="%sfield">%s</th>'%(i,i) for i in FIELDS])
        return tbl%("<tr id='trackheader'>"+thead+"</tr>","".join(rows))            

    def _createRows(self, status):
        """Returns the table rows"""
        retval = []
        i = 1
        reqid = "&reqid=" + str(status.reqid)
        curindex = status.getActiveIndex()
        id = None
        for track in self.tracks:
            style = 'tr_one'
            if i %2 == 0:
                style = 'tr_two'
            if curindex == (i-1):
                id = "nowplaying"
            retval.append(track.toRow(style=style, id=id, trackno=i-1, reqid=reqid))
            i = i+1
        return retval
    
    def sync(self):
        """Saves the current amaroK-Playlist to the current.xml file. Calls
        amarok via dcop"""
        os.system("dcop amarok playlist saveCurrentPlaylist")
