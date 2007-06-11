############################################################################
# Zeroconf support - publishing webcontrol page
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

import os
import getpass
import sys
import string
# find directory containing common Zeroconf files
temp=sys.path[:]
sys.path.insert(0,os.path.abspath(os.path.dirname(sys.argv[0])+'/../common'))
if not os.getenv("KDEDIR") is None: sys.path.insert(0,os.getenv("KDEDIR")+"/share/apps/amarok/scripts/common")
if not os.getenv("KDEDIRS") is None: sys.path=[p+"/share/apps/amarok/scripts/common" for p in string.split(os.getenv("KDEDIRS"),os.pathsep)]+sys.path
from Publisher import *
sys.path=temp

class WebPublisher(Publisher):
    
    port = None

    def services(self):
        return [{ "name" : "Amarok WebControl for "+getpass.getuser(), "port": self.port, "type": "_http._tcp", "properties": {}}]
    
publisher = WebPublisher()