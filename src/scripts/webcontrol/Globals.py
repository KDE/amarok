# for system call
import os

#
# the port number to listen to
#
PORT = 4774

#
# execution path of script
#
EXEC_PATH = None

#
# Use simple popen call in order not to depend on dcop python lib
# (since it does currently not work on my computer :)
#
def _initDcopCallPlayer(call):
    return os.popen("dcop amarok player %s"%call)

def _dcopCallPlayer(call):
    return _initDcopCallPlayer(call)

def _initDcopCallPlayerArg(call, val):
    return os.popen("dcop amarok player %s %s"%(call,val))

def _dcopCallPlayerArg(call, val):
    return _initDcopCallPlayerArg(call, val).read()

def _initDcopCallPlaylist(call):
    return os.popen("dcop amarok playlist %s"%call)

def _initDcopCallPlaylistArg(call, val):
    return os.popen("dcop amarok playlist %s %s"%(call,val))

def _dcopCallPlaylistArg(call, val):
    return _initDcopCallPlaylistArg(call, val).read()

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
        return self.value
    
class PlayerDcop ( DelayedDcop ):
    __super_init = DelayedDcop.__init__
    def __init__(self, command, val = None):
        self.__super_init(_initDcopCallPlayer,
                          _initDcopCallPlayerArg,
                          command, val)
        
class PlaylistDcop ( DelayedDcop ):
    __super_init = DelayedDcop.__init__
    def __init__(self, command, val = None):
        self.__super_init(_initDcopCallPlaylist,
                          _initDcopCallPlaylistArg,
                          command, val)
