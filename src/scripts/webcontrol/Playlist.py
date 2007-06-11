#!/usr/bin/env python
# -*- coding: Latin-1 -*-
#
# Author: Andr� Kelpe fs111 at web dot de
#       : Jonas Drewsen kde at xspect dot dk
#       : Peter Ndikuwera pndiku at gmail dot com
#
# License: GPL
#

import user
import os
from xml.dom import minidom
import time

import string

import Globals

# the current.xml file
PLAYLISTFILE = "%s/.kde/share/apps/amarok/current.xml"%(user.home)

# the fields to be shown via http
FIELDS = ("TrackNo", "Title", "Artist", "Album", "Length", "Rating")

class Track(object):
    """Class that holds the information of one track in the current playlist"""
    __slots__ = FIELDS

    max_field_value_lengths = [(0,"")] * len(FIELDS)

    def __init__(self, **kwargs):
        for key,value in kwargs.iteritems():
            setattr(self, key, value)


    def toRow(self, style='', id=None, trackno=None, reqid=None, sesid = None):
        """Returns the a html-table-row with member values of this class"""
        
        trno = str(trackno)
        astart = "<a "
        if id:
            astart += "name='nowplaying' "
        # append "#nowplaying" if we want to jump to current song.
        astart += ("class='track' href='javascript:dolink(\"action=goto&value="
                   + trno + "&"
                   + reqid + "&"
                   + sesid + "\");'>")
        
        aend = "</a>"
        
        tmp = [ '<td>' + astart + i + aend +'</td>' for i in [getattr(self,f) for f in self.__slots__ ] ]

        index = 0
#        for f in self.__slots__ :
#            print string.strip(f)
        
        for i in [getattr(self,f) for f in self.__slots__ ]:
            if len(string.strip(i)) > Track.max_field_value_lengths[index][0]:
                Track.max_field_value_lengths[index] = (len(string.strip(i)),i)
            index += 1
        
        tr_style = ''        
        tr_id = ''
        
        if style:
            tr_style='class="%s"'%(style)

        if trackno == 0:
            tr_id = 'id="trackone"'

        if id:
            tr_id = 'id="%s"'%(id)
        
        return '<tr %s %s>%s</tr>'%(tr_style, tr_id, ''.join(tmp))        

class Playlist:
    """The Playlist class represents the Playlist as one object. It holds all
    needed information and does most of the work"""

    def __init__(self):
        """The Constructor takes no arguments."""
        self.tracks = []
        self.encoding = "UTF-8"
        #"ISO-8859-15"
        self.fullPage = ""
        self.templateFilename = "template.thtml"
        self.templateLastChanged = -1
        self.templateLastChanged = self._loadHtmlTemplate()

    def _loadHtmlTemplate(self):
        """Loads the global string template variables that are used for
        rendering the page. The template can be changed by setting
        self.templateFilename to a template file."""
        ep = Globals.EXEC_PATH
        st = os.stat(ep + "/" + self.templateFilename)[8]

        if self.templateLastChanged != st:
            tmp = open(ep + "/" + self.templateFilename).read()
            a = compile(tmp, "<string>", 'exec')
            eval(a)
        return st
            
    def toHtml(self, status):
        """Returns a html representation of the whole Playlist"""
        self.templateLastChanged = self._loadHtmlTemplate()
        self.sync()
        self.tracks = []
        self.mtime = self._getMtime()
        self._buildDoc()
        self._setFullPage(status) 
        return self.fullPage    


    def _createButton(self, name, action, reqid, sesid):
        """Return a button to be used as an action"""
        return ("<a href='javascript:dolink(\"action="
                + action + reqid + sesid + "\");'><img src='player_" + name + ".png'></a>")
    
    def _createVolume(self, vol_val, reqid, sesid):
        """Return a HTML volume seletor."""
        
        volume = '<table width="100%" class="volume">'
        volume += "<tr>";

        button = "<div style='width:6px; height:12px;'></div>";

        for i in range(1,vol_val+1):
            volume += ("<td class='volumeset'>" +
                       "<a href='javascript:dolink(\"action=setvolume&value="
                       + str(i*10) + reqid + sesid + "\");'>" + button + "</a></td>")

        for i in range(vol_val+1, 11):
            volume += ("<td class='volumeunset'>" +
                       "<a href='javascript:dolink(\"action=setvolume&value="
                       + str(i*10) + reqid + sesid + "\");'>" + button + "</a></td>")

        volume += "</tr></table>"
        return volume
    
    def _createTimeStr(self, status):
        """Returns the string representation of the the remaining time of the current song."""

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

        return hours_str + mins_str + secs_str
        
    def _trackTimeStr(self, trackTime):
        """Returns the string representation of the track time of a song."""

        _time = int(trackTime)

        hours = _time / (60*60)
        mins  = (_time % (60*60)) / 60
        secs  = _time - 60 * mins - 60 * 60 * hours

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

        return hours_str + mins_str + secs_str
        
    def _ratingStars(self, rating):
        """Returns a number of stars showing the rating of a song."""
        
        _rating = int(rating)

        _numBig = _rating / 2
        _numSmall = _rating % 2

        img_str=""

        for i in range(1,_numBig+1):
            img_str = img_str + "<img height='12' width='12' src='star.png'>"
            
        if _numSmall> 0:
            img_str = img_str + "<img height='12' width='12' src='smallstar.png'>"
        
        return img_str

    def _createActions(self, status):
        """Returns HTML for allactions that can be performed and for the time counter""" 

        if not status.controlsEnabled():
            return ""

        reqid = "&reqid=" + str(status.reqid)
        sesid = "&sesid=" + str(status.sesid)

        playpause = self._createButton("play", "play", reqid, sesid)

        if status.isPlaying():
            playpause = self._createButton("pause", "pause", reqid, sesid) 

        buttons = (playpause + "&nbsp;" +
                   self._createButton("stop", "stop", reqid, sesid) + "&nbsp;" +
                   self._createButton("start", "prev", reqid, sesid) + "&nbsp;" +
                   self._createButton("end", "next", reqid, sesid))

        vol_val = int(float(status.getVolume()) / 10.0 + 0.5)
        volume = self._createVolume(vol_val, reqid, sesid)
        
        return actions % ( buttons, volume, self._createTimeStr(status) )
    
    def _setFullPage(self, status):
        """Renders the fullpage into the varialbe self.fullPage"""
        counter = ""
        if status.isPlaying():
            counter = "countdown(" + str(status.timeLeft()) + ");"

        self.fullPage = code % ( counter,
                                 self._createActions(status).encode(self.encoding,'replace'),
                                 os.environ['LOGNAME'],
                                 self._createTable(status).encode(self.encoding,'replace') )
        
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
        i = 1
        for item in self.doc.getElementsByTagName("item"):
            curTrack = Track()
            for elem in FIELDS:
                try:
                    value = item.getElementsByTagName(elem)[0].firstChild.nodeValue
                except:
                    value = '  '
                if elem == "Title":
                    value = value.strip()
                if elem == "Length":
                    value = self._trackTimeStr(value);
                if elem == "Rating":
                    value = self._ratingStars(value);
                if elem == "TrackNo":
                    value = str(i)
                    i = i + 1
                setattr(curTrack, elem, value)
            append(curTrack)


    def _createTable(self, status):
        """Returns the HTML-Table"""
        tbl = tblhead
        rows = self._createRows(status) 
        thead = "<tr id='trackheader'>" + "".join(['<td>%s</td>'%(i) for i in FIELDS]) + "</tr>"
        rowsstr = "".join(rows)
        return tbl%("tracks", "<thead>" + thead + "</thead><tbody>" + rowsstr + "</tbody>") 

    def _createRows(self, status):
        """Returns the table rows"""
        retval = []
        i = 1
        reqid = "&reqid=" + str(status.reqid)
        sesid = "&sesid=" + str(status.sesid)
        curindex = status.getActiveIndex()
        id = None
        for track in self.tracks:
            style = 'tr_one'
            if i %2 == 0:
                style = 'tr_two'
            if curindex == (i-1):
                id = "nowplaying"
            retval.append(track.toRow(style=style, id=id, trackno=(i-1), reqid=reqid, sesid=sesid))
            i = i+1
        return retval
    
    def sync(self):
        """Saves the current Amarok-Playlist to the current.xml file. Calls
        amarok via dcop"""
        os.system("dcop amarok playlist saveCurrentPlaylist")
