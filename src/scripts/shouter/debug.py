############################################################################
# Debug output
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


import sys

debug_prefix = "[Shouter]"
debug_h = open('shouter.debug', 'a')
def debug( message ):
    if 0:
        print('%s %s\n' % (debug_prefix, str(message)))
    else:
        debug_h.write( '%s %s\n' % (debug_prefix, str(message)))
        debug_h.flush()
