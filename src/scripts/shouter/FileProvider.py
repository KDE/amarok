############################################################################
# Abstraction layer for files. This mainly hides the reencoded-ness of a
# file from the request handler. 
# Will obsolesce ShouterRequest.ReencodedRequest
#
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


import os 
from debug import *
from StreamConfig import *
from Encoder import *
from sre import *

class FileProvider:
        """ Abstraction for file """

        pos = 0
        f = None
        fname = None
        fsize = 0
        cfg = None
        enc = None
        reencoding = False

        def __init__(self, fname, cfg=StreamConfig.StreamConfig()):
            self.fname = fname
            self.fsize = os.stat(fname)[6]
            self.pos = 0
            self.cfg = cfg
            if cfg.reencoding:
                fext = findall(r'.*\.(.+?)$', fname.lower())[0]
                if (fext != cfg.stream_format) or cfg.reencoding==2:
                    self.reencoding = True
                    self.enc = Encoder(fname)
                else:
                    self.f = file(fname, 'r')
                    self.f.seek(0)
            else:
                self.f = file(fname, 'r')
                self.f.seek(0)

        def tell(self):
           return self.pos

        def seek(self, pos):
           if self.f is not None: self.f.seek(pos)
           self.pos = pos

        def read(self, size):
           if self.f is not None:
               buf = self.f.read(size)
               self.pos += len(buf)
               return buf
           else:
               buf = self.enc.read_from(size, self.pos)
               self.pos += len(buf)
               return buf
