############################################################################
# Helpful container for configuration options 
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


IDLE_MODES = ['Silence', 'Directory', 'Genre', 'Year', 'Bitrate', 'Random collection entry', 'amaroK playlist']

class StreamConfig:
    """ Container for stream configuration. Values are defaults and should be
    reset when the configuration is loaded from disk """

    mount = '/amarok'
    genre = 'Mixed'
    name = 'amaroK shouter'
    url = 'http://amarok.kde.org'
    desc1, desc2 = '', ''
    port = 8000
    buf_size = 4096
    icy_interval = 16384
    max_clients = 4
    punc_factor = 100
    pre_seek = 2
    force_update = False
    enable_dl = True
    supress_dialog = False
    dl_mount = '/current'
    dl_throttle = 20
    reencoding = 0 # 0=none | 1=dissimilar formats | 2=all tracks
    stream_format = 'mp3'
    stream_br = 192
    chunk_size = 524288
    idle_mode = 4 # Random collection entry
    idle_arg= ''
    inject_dir = ''
    inject_pct = 0
    inject_filt = '*.mp3'

def idle_mode(i):
    try:
        return IDLE_MODES[i]
    except IndexError:
        return IDLE_MODES[0]
               
        
