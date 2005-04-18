############################################################################
# Handler for re/trans-coding of file formats
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

from binfuncs import *
import sys
import os
import pipes
import StreamConfig
import tempfile
import threading
import time
from debug import *
from sre import *
from FormatHelper import *
from ShouterExceptions import *

# 
# Problems with reencoding:
#     1. It takes a long time. Clients joining a stream shouldn't have to
#     wait for 2 minutes while the server reencodes the requested file to a
#     different format 
#     2. It's computationally expensive. Nobody wants to see
#     their CPU parked at 100% while the server is reencoding the current
#     song even though nobody is listening. A file should be reencoded once
#     and only once, regardless of how many clients are connected.  
#     3. Decoding has the potential to take a lot of disk space. Be 
#     conservative
#
# This implementation isn't great but I think it addresses most of these problems. 
# On initialization, a file is chunked into manageable blocks. They're processed
# only when requested and only if they haven't already been processed. The encoder 
# will try and start a chunk in advance of when it is needed so that there isn't a 
# break in the datastream being sent out of the socket.
#
# TODO:
#     Chunks aren't cleaning themselves up
#     mp3 chunking


NONE, INITIALIZED, CHUNKED, DECODED, ENCODED = 'none', 'initialized', 'chunked', 'decoded', 'encoded'

class Chunk:
    fd, fname = None, None
    fsrc = None
    format = None
    post_chunkname = None
    lock = None
    state = NONE
    pos, chunk_size = 0, 0
    start_format, end_format = '', ''
    cfg = None

    def __init__(self, source_file, pos, cfg, start_format, end_format):
        self.lock = threading.Lock()
        self.fsrc = source_file
        self.pos = pos

        #set real chunk size
        fsize = os.stat(source_file)[6]
        dm = divmod(fsize - pos, cfg.chunk_size)
        if dm[0] == 0:
            self.chunk_size = dm[1]
        else:
            self.chunk_size = cfg.chunk_size

        self.cfg = cfg
        self.start_format, self.end_format = start_format, end_format
        self.state = INITIALIZED
        self.locked = False
    
    def activate(self):
        if self.state != INITIALIZED: return
        self.lock.acquire()
        self._chunk()
        self._decode()
        self._encode()
        self.lock.release()
        debug('activated chunk at %d' % self.pos)

    def _chunk(self):
        """ Break off from main file into a temporary chunkfile """

        debug( 'chunking %d' % self.pos )
        self.fd, self.fname = tempfile.mkstemp(suffix='.chunk%d' % self.pos, prefix='shouter-', dir='/tmp')
        try:
            helper = self.start_format.lower() + 'Helper().chunk("%s", "%s", %d, %d)'
            eval(helper % (self.fname, self.fsrc, self.pos, self.chunk_size))
        except NameError:
            helper = 'GenericHelper().chunk("%s", "%s", %d, %d)'
            eval(helper % (self.fname, self.fsrc, self.pos, self.chunk_size))

        #os.unlink(self.fname)
        debug('finishing chunk, fname=%s' % self.fname)
        self.state = CHUNKED

    def _decode(self):
        """ Decode to pcm """

        fd_pcm, fname_pcm = tempfile.mkstemp( suffix='.pcm', prefix='shouter-', dir='/tmp' )
        try:
            helper = self.start_format.lower() + 'Helper().decode("%s", "%s")'
            eval(helper % (self.fname, fname_pcm))
        except NameError:
            helper = 'GenericHelper().decode("%s", "%s")'
            eval(helper % (self.fname, fname_pcm))

        debug('trying to unlink %s with size %d' % (self.fname, os.stat(self.fname)[6]))
        #os.unlink(self.fname)
        self.fd, self.fname = fd_pcm, fname_pcm
        self.state = DECODED

    def _encode(self):
        """ Turn pcm file into encoded stream """ 

        fd, fname = tempfile.mkstemp(suffix='.enc', prefix='shouter-', dir='/tmp')
        helper = self.end_format.lower() + 'Helper().encode("%s", "%s", %d)'
        eval(helper % (self.fname, fname, self.cfg.stream_br))
        #os.unlink(self.fname)
        self.fd, self.fname = fd, fname
        self.state = ENCODED
    
    def reset(self):
        """ Delete encoded files and reset state """

        os.unlink(self.fname)
        self.state = NONE
    

class Encoder:
    """ Fancy name for a chunked file """

    chunks = []
    cfg = None
    format = None
    start = 0

    def __init__(self, fname, cfg=StreamConfig.StreamConfig()):
        """ Draw chunk boundries and get first chunk underway """

        debug('spawning encoder for %s' % fname)
        self.cfg = cfg
        start_format = findall(r'\.(\w*?)$', fname)[0]
        start = 0
        try:
            get_data_start = start_format + 'Helper().get_data_start("%s")'
            start = eval(get_data_start % fname)
        except NameError:
            get_data_start = 'GenericHelper().get_data_start("%s")'
            start = eval(get_data_start % fname)
        f = file(fname, 'r')
        f.seek(start)
        fsize = os.stat(fname)[6]
        end_format = cfg.stream_format

        if not self.cfg.chunk_size:
            self.cfg.chunk_size=os.stat(fname)[6]

        while f.tell() < fsize:
            pos = f.tell()
            self.chunks.append(Chunk(fname, pos, cfg, start_format, end_format))
            f.seek(cfg.chunk_size, 1)

        debug( 'initialized %d %s -> %s chunks' % (len(self.chunks), start_format, end_format))
        f.close()

        #if cfg.reencode: 
            #self.chunks[0].activate()

    def get_chunk_index(self, pos):
        i = iter(self.chunks)
        for i in self.chunks:
            if i.pos <= pos < i.pos + i.chunk_size:
                return self.chunks.index(i)
        raise chunk_not_found_error(pos)
        

    def read_from(self, size, pos):
        c_i = self.get_chunk_index(pos)
        c = self.chunks[c_i]

        # Should never happen
        if c.state == NONE: raise chunk_error
    
         # This should only happen when the first client joins a new stream.
        if c.state == INITIALIZED: 
            if not c.lock.locked():
                debug('reading from unencoded chunk %d. activating...' % c_i)
                c.activate()
            else:
                # if the needed chunk is being activated in another thread, 
                # wait until it's ready
                while c.lock.locked():
                    debug('Waiting for lock on %s' % str(c))
                    time.sleep(.1)

            
        # Activate next chunk in a separate thread
        if c_i != len(self.chunks)-1 :
            if self.chunks[c_i+1].state == INITIALIZED:
                if not self.chunks[c_i+1].lock.locked():
                    debug('chunk %d is initialized and unlocked. activating...' % (c_i+1))
                    threading.Thread(target=self.chunks[c_i+1].activate).start()

        f = file(c.fname, 'r')
        f.seek(pos - c.pos)
        buf = f.read(size)
        f.close()
        #debug('read_from size=%d pos=%d c_i=%d chunk.tell=%d c.pos=%d len(buf)=%d' % (size, pos, c_i, temp, c.pos, len(buf)))
        return buf
    
    def clean(self):
        for i in self.chunks:
            i.reset()
    


