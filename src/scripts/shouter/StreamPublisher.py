############################################################################
# Zeroconf support - publishing configured streams
# (c) 2005 Jakub Stachowski <qbast@go2.pl>
#
# Depends on: Python 2.2, pyzeroconf 0.12+metaservice patch
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

from debug import *
import os
import sys
import string
# find directory containing common Zeroconf files
if not os.getenv("KDEDIR") is None: sys.path.insert(0,os.getenv("KDEDIR")+"/share/apps/amarok/scripts/common")
if not os.getenv("KDEDIRS") is None: sys.path=[p+"/share/apps/amarok/scripts/common" for p in string.split(os.getenv("KDEDIRS"),os.pathsep)]+sys.path
print sys.path
from Publisher import *

class StreamPublisher(Publisher):
    
    cfg_mgr = None
    port = None

    def services(self):
        sv = []
    	if not self.cfg_mgr.server_cfg.publish==1: return sv
        for i in self.cfg_mgr.stream_cfgs:
	    sv.append( { "name" : i.name , "port": self.port, "type": "_shoutcast._tcp", "properties": { "path": "/"+i.mount }} )
	    debug("Will register %s mounted at %s" % (i.name,i.mount))
	return sv
    
publisher = StreamPublisher()
