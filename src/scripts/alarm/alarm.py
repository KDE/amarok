#!/usr/bin/env python

############################################################################
# Config dialog for alarm script
# (c) 2005 Mark Kretschmann <markey@web.de>
#
# Depends on: PyQt
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

from ConfigParser import *
import sys
import os.path
from qt import *


file = open( "alarmrc", "r" )

config = ConfigParser()
config.readfp( file )

#time = config.get( "General", "alarm time" )
#print time

file.close()



