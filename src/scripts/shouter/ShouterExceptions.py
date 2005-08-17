############################################################################
# Exception classes
# (c) 2005 James Bellenger <jamesb@squaretrade.com>
#
# Depends on: Python 2.2
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

from exceptions import *

class cfg_not_found_error(Exception):
    pass

class unmapped_mount_error(Exception):
    pass

class service_ended_error(Exception):
    pass

class url_is_stream_error(Exception):
    pass

class url_is_null_error(Exception):
    pass

class indeterminate_queue_error(Exception):
    pass

class remote_url_error(Exception):
    pass

class amarok_not_playing_error(Exception):
    pass

class unknown_length_error(Exception):
    pass

class playlist_empty_error(Exception):
    pass

class bad_format_error(Exception):
    pass
