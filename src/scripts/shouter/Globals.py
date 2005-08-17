############################################################################
# Global utility functions
# (c) 2005 James Bellenger <jamesb@squaretrade.com>
#
# Adapted from Globals.py written by
# Jonas Christian Drewsen and André Kelpe
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

def _init_dcop_call_player(call):
    return os.popen("dcop --no-user-time amarok player %s"%call)

def _dcop_call_player(call):
    return _init_dcop_call_player(call)

def _init_dcop_call_player_arg(call, val):
    return os.popen("dcop --no-user-time amarok player %s %s"%(call,val))

def _dcop_call_player_arg(call, val):
    return _initDcopCallPlayerArg(call, val).read()

def _init_dcop_call_playlist(call):
    return os.popen("dcop --no-user-time amarok playlist %s"%call)

def _init_dcop_call_playlist_arg(call, val):
    return os.popen("dcop --no-user-time amarok playlist %s %s"%(call,val))

def _dcop_call_playlist_arg(call, val):
    return _init_dcop_call_playlist_arg(call, val).read()

def _init_dcop_call_collection(call):
    return os.popen("dcop --no-user-time amarok collection %s"%call)

def _init_dcop_call_collection_arg(call, val):
    debug("dcop --no-user-time amarok collection %s %s"%(call,val))
    return os.popen("dcop --no-user-time amarok collection %s %s"%(call,val))

def _dcop_call_collection_arg(call, val):
    return _init_dcop_call_collection_arg(call, val).read()

class DelayedDcop:
    def __init__(self, initcall, initcallarg, command, val = None):
        self.initcall = initcall
        self.initcallarg = initcallarg
        self.value = None
        self.fd = None
        self.arg = val
        self.command = command
        
    def init(self):
        if not (self.value is None and self.fd is None):
            return
        if self.arg is None:
            self.fd = self.initcall(self.command)
        else:
            self.fd = self.initcallarg(self.command, self.arg)

    def result(self):
        self.init()
        if self.value is None:
            self.value = self.fd.read()
        value = self.value.rstrip()
        try:
            value = int(value)
        except ValueError:
            pass
        return value
    
class PlayerDcop ( DelayedDcop ):
    __super_init = DelayedDcop.__init__
    def __init__(self, command, val = None):
        self.__super_init(_init_dcop_call_player,
                          _init_dcop_call_player_arg,
                          command, val)
        
class PlaylistDcop ( DelayedDcop ):
    __super_init = DelayedDcop.__init__
    def __init__(self, command, val = None):
        self.__super_init(_init_dcop_call_playlist,
                          _init_dcop_call_playlist_arg,
                          command, val)

class CollectionDcop ( DelayedDcop ):
    __super_init = DelayedDcop.__init__
    def __init__(self, command, val = None):
        self.__super_init(_init_dcop_call_collection,
                          _init_dcop_call_collection_arg,
                          command, val)
