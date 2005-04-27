############################################################################
# Dumping ground for any and everything that is part of the amarok interface
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

from Globals import *
import urllib
from sre import *
from debug import *
from xml.dom import minidom

class _Amarok:
    is_playing = False
    now_playing = ''
    encoded_url = ''
    fname = ''
    state = 0

    def __init__(self):
        debug('_Amarok init')
        self.on_engine_state_change()

    def on_engine_state_change(self, string = ''):
        if not string:
            playing = PlayerDcop('isPlaying').result() == 'true'
            if playing: string = 'playing'
        if string.find('playing') >= 0:
            self.state = 1
        else:
            self.state = 0
        debug('Amarok state = %d' % self.state)

    def on_track_change(self):
        self.state += 1

    def query(self, sql):
        #sql = sql.replace("'", "\\'").replace('"', '\\"')
        result = CollectionDcop('query "%s"' % sql).result()
        try:
            result = result.splitlines()
        except AttributeError:
            result = [result]
        return result

    def status(self, msg):
        PlaylistDcop( 'shortStatusMessage "%s"' % ('%s ' % debug_prefix + str(msg)) ).result()

    #def get_current_file(self):
        #if self.state:
            #PlayerDcop('

    def random_status(self):
        return int(PlayerDcop('randomModeStatus').result() == 'true')

    def repeat_pl_status(self):
        return int(PlayerDcop('repeatPlaylistStatus').result() == 'true')

    def repeat_tr_status(self):
        return int(PlayerDcop('repeatTrackStatus').result() == 'true')

    def get_bitrate(self, fname=None):
        debug('_Amarok get_bitrate')

        fail_safe = 192
        if fname:
            sql = 'SELECT bitrate FROM tags WHERE url=\'%s\''
            result = self.query(sql % fname)
            if result:
                return result[0]

        dcop_br = PlayerDcop('bitrate').result()
        try:
            bitrate = findall(r'(\d*)\skbps', dcop_br)[0]
            return int(bitrate)
        except:
            return fail_safe

Amarok = _Amarok()
