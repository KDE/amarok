############################################################################
# Implementation of playlist models
# (c) 2005 James Bellenger <jamesb@squaretrade.com>
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

from debug import *
from Amarok import *
from xml.dom import minidom
from Globals import *
from shutil import copy
import ShouterExceptions 
from sre import *
import random
import urllib
import time
import sys
from xml.parsers.expat import ExpatError


_STATIC_PLS = dict()
SILENCE_F = os.path.join(os.path.dirname(sys.argv[0]), 'silence/silence-%d.mp3')
SILENT_META = '--- Server is waiting for input ---'

class XMLPlaylist:
    random    = False
    repeat_pl = True
    pl = []
    items = []
    fname = ''

    def __init__(self, random=0, repeat_pl=1, fname=''):
        self.random = random
        self.repeat_pl = repeat_pl
        self.fname = fname
        self.load()

    def load(self):
        try:
            self.items = minidom.parse(self.fname).getElementsByTagName('item')
        except ExpatError:
            debug(sys.exc_info()[1])
            debug('*** Caught error while parsing xml file. Check the tags of the item listed at the above line in the file ~/.kde/share/apps/amarok/current.xml')


class LivePlaylist(XMLPlaylist):
    encoded_url = None

    def load(self):
        if not self.fname:
            self.fname = self.save_current_playlist()

        XMLPlaylist.load(self)
        self.pl = []
        for n in self.items:
            try:
                self.pl.append(File(n))
            except:
                raise

    def reload(self):
        self.random = PlayerDcop('randomModeStatus').result() == 'true'
        self.repeat_pl = PlayerDcop('repeatPlaylistStatus').result() == 'true'
        self.save_current_playlist()
        self.load()
        
    def save_current_playlist(self):
        return PlaylistDcop('saveCurrentPlaylist').result()

    def get_play_cursor(self):
        """ Returns the tuple (File, time fraction) """

        debug('LivePlaylist get_play_cursor')

        if Amarok.state:
            self.reload()
            i = PlaylistDcop('getActiveIndex').result()
            if i == -1:
                raise ShouterExceptions.amarok_not_playing_error
            f = None
            try:
                f = self.pl[i]
            except IndexError:
                debug('Caught index error loading i=%d len(pl)=%d. Reloading' % (i, len(self.pl)))
                self.reload()
                f = self.pl[i]
            
            total = f.length
            current = PlayerDcop( 'trackCurrentTime' ).result()
            
            # In case Amarok is playing a stream and it wasn't caught before,
            # total may come back as '-'
            if total == '-':
                debug('total = %s raising exception' % str(total))
                raise ShouterExceptions.amarok_not_playing_error

            if current > total : current = total
            if total == 0:
                raise ShouterExceptions.amarok_not_playing_error
            frac = float(current)/total
            return (f, frac)
        raise ShouterExceptions.amarok_not_playing_error


# Masked to keep instances unique for fnames
def StaticPlaylist(fname, random=0, repeat=1):
    if not _STATIC_PLS.has_key(fname):
        debug('Creating static playlist for %s' % fname)
        _STATIC_PLS[fname] = _StaticPlaylist(fname, random, repeat)
    return _STATIC_PLS[fname]

class _StaticPlaylist(XMLPlaylist):
    queue = None
    skip = []

    def __init__(self, fname, random=0, repeat=1):
        debug('_StaticPlaylist init %s' % fname)
        XMLPlaylist.__init__(self, random, repeat, fname)
        self.process()
        

    def process(self):
        # Queued tracks should be played in order, but only on the first loop
        # Subsequent loops through the playlist should play in the correct order
        # 1. Collect all queued tracks into a separate list
        # 2. Remove any skipped tracks from pl
        # 3. Move track-after-last-queue to the fore
        # 4. Append tracks before last queue
        # 5. if random is set, shuffle
        # 6. Renumber queue nodes according to new playlist order

        queue = dict()      # queue index : pl index
        skip  = list()      # DOM element
        files = dict()      # DOM element : File obj

        # Make two passes. First pass is to get the skip list and remove its
        # elements from self.items. Second pass needed to reindex the queue.
        for c in range(2):
            for i, n in enumerate(self.items):
                try:
                    f = File(n, strict=True)
                    files[n] = f
                    if isinstance(f.queue_index, int):
                        queue[f.queue_index] = i
                except (ShouterExceptions.unknown_length_error, ShouterExceptions.bad_format_error):
                    skip.append(n)

            if c == 0:
                [ self.items.remove(n) for n in skip ]
                files, queue, = {}, {}

        # Rearrange playlist around queue
        pl_xml_new = []
        if queue:
            i = queue[queue.keys()[-1]]
            pl_xml_new = self.items[i+1:]
            if self.repeat_pl:
                pl_xml_new.extend(self.items[:i+1])
        else:
            pl_xml_new = self.items

        if self.random:
            random.shuffle(pl_xml_new)

        # Renumber queue items
        for i in queue:
            queue[i] = pl_xml_new.index(self.items[queue[i]])

        # Map xml nodes to files
        pl_f = [ files[n] for n in pl_xml_new ]

        self.queue = queue
        self.pl = pl_f
        self.skip = skip

    def get_play_cursor(self):
        debug('StaticPlayList get_play_cursor')
        now = int(time.time())
        dt = now - Amarok.start_time
        pl = self.pl

        for i in self.queue:
            f = pl[self.queue[i]]
            if f.length <= dt: 
                dt -= f.length
            else:
                return (f, float(dt)/f.length)

        # If we've made it this far, drop the queue, it isn't needed anymore
        self.queue = []
        Amarok.start_time = now - dt

        while True:
            for f in pl:
                if f.length <= dt:
                    dt -= f.length
                else:
                    return (f, float(dt)/f.length)
            if self.repeat_pl:
                Amarok.start_time = now - dt
            else:
                raise ShouterExceptions.playlist_empty_error

    def get_next_file(self, current):
        debug('StaticPlaylist get_next_file')
        for i,f in enumerate(self.pl):
            if f is current:
                try:
                    return self.pl[i+1]
                except IndexError:
                    if self.repeat_pl:
                        return self.pl[0]
                    else:
                        raise ShouterExceptions.playlist_empty_error
        raise ShouterExceptions.playlist_empty_error

class File:
    tags = ['Artist', 'Title', 'Length', 'Genre', 'Album', 'Year', 'TrackNo', 'Bitrate']
    attr = ['url', 'queue_index']

    def __init__(self, node, strict=False):
        for t in self.tags:
            try:
                val = node.getElementsByTagName(t)[0].childNodes[0].data
                setattr(self, t.lower(), val.encode('iso-8859-1'))
            except:
                pass
        for a in self.attr:
            val = node.getAttribute(a)
            try:
                val = int(val)
            except:
                pass
            setattr(self, a.lower(), val)

        try:
            self.bitrate = int(findall(r'^(\d*).*$', self.bitrate)[0])
        except ValueError:
            self.bitrate = ''

        try:
            l_tup = findall(r'(\d*):(\d*)', self.length)[0]
            m, s = ( int(l_tup[0]), int(l_tup[1]) )
            self.length = m * 60 + s
        except:
            if strict:
                raise ShouterExceptions.unknown_length_error

        if not self.url.lower().endswith('.mp3'):
            if strict:
                raise ShouterExceptions.bad_format_error

    def get_meta(self):
        meta = ''
        if self.artist:
            meta += self.artist
            if self.title:
                meta += ' - ' + self.title
        elif self.title:
            meta += self.title
        if not meta:
            meta = 'Unknown Artist and Album'
        return meta

    def get_fname(self):
        if self.url.startswith('file:///'):
            return urllib.url2pathname(str(self.url)).replace('file://', '')
        raise ShouterExceptions.remote_url_error


class SilentFile(File):
    def __init__(self, br):
        for t in self.tags: 
            setattr(self, t.lower(), 'Unknown')
        for a in self.attr:
            setattr(self, a.lower(), 'Unknown')

        self.bitrate = br
        
        # Converting a perfectly good filename to a url only so that it can be
        # converted back again may be verging on poor taste
        self.url = 'file://' + SILENCE_F % br

    def get_meta(self):
        return SILENT_META
            
        
def validate(fname):
    debug('validate %s' % fname)
    spl = _StaticPlaylist(fname)
    if spl.skip: return False
    return True

def save_current_as(fname):
    debug('save_current_as %s' % fname)
    fname_current = PlaylistDcop('saveCurrentPlaylist').result()
    fname_new = os.path.basename(fname_current)
    fname_new = os.path.join( os.getcwd(), 
                              fname_new.replace('current', str(fname)))

    try:
        copy(fname_current, fname_new)
        return fname_new
    except:
        return False
