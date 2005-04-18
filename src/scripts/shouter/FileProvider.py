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
from FormatHelper import FORMATS
from ShouterExceptions import *

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
            fext = ''

            if not fname.startswith('/'): raise format_error

            try:
                fext = findall(r'.*\.(.+?)$', fname.lower())[0]
            except IndexError:
                raise format_error

            if fext == cfg.stream_format:
                self.f = file(fname, 'r')
                self.f.seek(0)
            else:
                if cfg.reencoding:
                    try:
                        FORMATS.index(fext)
                        self.enc = Encoder(fname)
                    except ValueError:
                        raise format_error
                else: raise format_error

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

