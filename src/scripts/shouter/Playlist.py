############################################################################
# Implementation of playlist models
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

from debug import *
from Amarok import *
from xml.dom import minidom
from Globals import *
import ShouterExceptions 
from sre import *
import random
import urllib
import time


_STATIC_PLS = dict()
_DIRECTORY_PLS = dict()
SILENCE_F = '../scripts/shouter2/silence/silence-%d.mp3'
SILENT_META = '--- Server is paused ---'

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
        self.items = minidom.parse(self.fname).getElementsByTagName('item')


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
            f = self.pl[i]
            total = f.length
            current = PlayerDcop( 'trackCurrentTime' ).result()
            if current > total : current = total
            if total == 0:
                raise ShouterExceptions.amarok_not_playing_error
            frac = float(current)/total
            return (f, frac)
        raise ShouterExceptions.amarok_not_playing_error

    #def get_next_url(self):
        #""" Returns -- at the time of invocation -- the url that is
        #guaranteed to be played next pending no future adjustments to the
        #playlist order or queueing """
        #self.reload()
#
        #queued = dict()
        #for i in range(len(self.pl)):
            #n = self.pl[i]
            #if n.hasAttribute('queue_index'):
                #q_i = int(n.getAttribute('queue_index'))
                #url = n.getAttribute('url')
                #queued[q_i] = (i, url)
        #try:
            #return queued[1][1]
        #except KeyError:
            #if not self.random:
                #try:
                    #n = self.pl[queued[0][0] + 1]
                    #return n.getAttribute('url')
                #except IndexError:
                    #if self.repeat_pl:
                        #n = self.pl[0]
                        #return n.getAttribute('url')
                    #elif self.repeat_tr:
                        #try:
                            #return queued[0][1]
                        #except:
                            #pass
        #raise ShouterExceptions.indeterminate_queue_error
            
# Masked to keep instances unique for fnames
def StaticPlaylist(fname, random=0, repeat=1):
    if not _STATIC_PLS.has_key(fname):
        debug('Creating static playlist for %s' % fname)
        _STATIC_PLS[fname] = _StaticPlaylist(fname, random, repeat)
    return _STATIC_PLS[fname]

class _StaticPlaylist(XMLPlaylist):
    start_time = 0
    queue = None
    skip = []

    def __init__(self, fname, random=0, repeat=1):
        debug('_StaticPlaylist init %s' % fname)
        XMLPlaylist.__init__(self, random, repeat, fname)
        self.process()
        self.start_time = int(time.time())
        

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
                except ShouterExceptions.unknown_length_error:
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
        dt = now - self.start_time
        pl = self.pl

        for i in self.queue:
            f = pl[self.queue[i]]
            if f.length <= dt: 
                dt -= f.length
            else:
                return (f, float(dt)/f.length)

        # If we've made it this far, drop the queue, it isn't needed anymore
        self.queue = []
        self.start_time = now - dt

        while True:
            for f in pl:
                if f.length <= dt:
                    dt -= f.length
                else:
                    return (f, float(dt)/f.length)
            if self.repeat_pl:
                self.start_time = now - dt
            else:
                raise ShouterExceptions.playlist_empty_error

def DirectoryPlaylist(dirname, random=1, repeat=1):
    if not _DIRECTORY_PLS.has_key(dirname):
        debug('Creating directory playlist for %s' % dirname)
        _DIRECTORY_PLS[fname] = _DirectoryPlaylist(dirname, random, repeat)
    return _DIRECTORY_PLS[dirname]

class _DirectoryPlaylist:
    start_time = 0
    queue = None
    skip = []
    pl = []

    def __init__(self, dirname, random=1, repeat=1):
        gen = os.walk(dirname)
        files = []
        try:
            while True:
                files.extend(gen.next()[2])
        except StopIteration:
            pass
                
        debug('_DirectoryPlaylist init %s' % fname)
        self.start_time = int(time.time())
        
class File:
    tags = ['Artist', 'Title', 'Length', 'Genre', 'Album', 'Year', 'TrackNo', 'Bitrate']
    attr = ['url', 'queue_index']

    def __init__(self, node, strict=False):
        for t in self.tags:
            try:
                val = node.getElementsByTagName(t)[0].childNodes[0].data
                setattr(self, t.lower(), val)
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

    def get_meta(self):
        return self.artist + ' - ' + self.title

    def get_fname(self):
        if self.url.startswith('file:///'):
            return urllib.url2pathname(sub('file:/*', '/', self.url))
        raise ShouterExceptions.remote_url_error

class DirFile(File):
    def __init__(self, fname):
        pass

class SilentFile(File):
    def __init__(self, br):
        for t in self.tags: 
            setattr(self, t.lower(), 'Unknown')
        for a in self.attr:
            setattr(self, a.lower(), 'Unknown')

        self.bitrate = br
        
        # Converting a perfectly good filename to a url only so that it can be
        # converted back again may be verging on poor taste
        self.url = 'file://' + os.path.join(os.getcwd() + '/' + SILENCE_F % br)

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

    # I thought there was a better way of copying files somewhere ...
    debug('cp %s %s' % (fname_current, fname_new))
    rv = os.system('cp %s %s' % (fname_current, fname_new))
    if rv: 
        return False
    return fname_new
