#!/usr/bin/env python
# -*- coding: Latin-1 -*-

"""
 This scripts starts a small http-server that serves the current-playlist as
 HTML. Start it and point your browser to http://localhost:4773/

 Author: André Kelpe fs111 at web dot de
       : Jonas Drewsen kde at xspect dot dk

 License: GPL

"""
import SimpleHTTPServer
import BaseHTTPServer
from Playlist import Playlist

# for system call
import os

# the port number to listen to
PORT = 4774

PLIST = None

# keep track of request ids in order to not repeat
# requests if user refreshes his browser
REQ_ID = 0

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

#
# Holding current AmarokStatus. A bunch of init_XXX functions in
# order to begin dcop requests as early as possible to avoid too
# much latency
#
class AmarokStatus:

    EngineEmpty = 1
    EngineIdle = 2
    EnginePause = 3
    EnginePlay = 4

    allowControl = 0
    playState = -1

    dcop_isplaying = None
    dcop_volume = None
    dcop_trackcurrentindex = None
    dcop_trackcurrenttime = None
    dcop_tracktotaltime = None
    
    def __init__(self):
        self.controls_enabled = 0
        self.time_left = None

        self.dcop_isplaying = PlayerDcop("isPlaying")
        self.dcop_volume = PlayerDcop("getVolume")
        self.dcop_trackcurrentindex = PlaylistDcop("getActiveIndex")
        self.dcop_trackcurrenttime = PlayerDcop("trackCurrentTime")
        self.dcop_tracktotaltime = PlayerDcop("trackTotalTime")
        
    def isPlaying(self):
        if self.playState != -1:
            res = self.playState == self.EnginePlay
        else:
            res = "true" in self.dcop_isplaying.result()
            if res:
                self.playState = self.EnginePlay
            else:
                self.playState = self.EnginePause
        
        return res

    def getActiveIndex(self):
        return int(self.dcop_trackcurrentindex.result())

    def getVolume(self):
        return int(self.dcop_volume.result())

    def timeLeft(self):
        cur =   int(self.dcop_trackcurrenttime.result())
        total = int(self.dcop_tracktotaltime.result())
        return total - cur

    def controlsEnabled(self):
        return self.allowControl

class RequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    """We need our own 'RequestHandler, to handle the requests, that arrive at
    our server."""
    
    def _amarokPlay(self):
        AmarokStatus.playState = AmarokStatus.EnginePlay
        _dcopCallPlayer("play")

    def _amarokPause(self):
        AmarokStatus.playState = AmarokStatus.EnginePause
        _dcopCallPlayer("pause")

    def _amarokNext(self):
        _dcopCallPlayer("next")

    def _amarokGoto(self,index):
        AmarokStatus.playState = AmarokStatus.EnginePlay
        AmarokStatus.currentTrackIndex = int(index)
        _dcopCallPlaylistArg("playByIndex",index)

    def _amarokPrev(self):
        _dcopCallPlayer("prev")

    def _amarokStop(self):
        _dcopCallPlayer("stop")
        AmarokStatus.playState = AmarokStatus.EngineStop
        
    def _amarokSetVolume(self, val):
        _dcopCallPlayerArg("setVolume",val)

    def _handleAction(self):
        global REQ_ID

        querystr = self.path.split("?")

        if len(querystr) <= 1:
            return

        queries = querystr[-1].split("&");

        qmap = {}
        for query in queries:
            var = query.split("=")
            if len(var) != 2:
                continue
            qmap[var[0]] = var[1]

        # abort a request that has already been completed
        # probably a refresh from the users browser
        if qmap.has_key("reqid") and REQ_ID == int(qmap["reqid"]):
            return 0

        if qmap.has_key("action"):
            a = qmap["action"]
            if a == "stop":
                self._amarokStop()
            elif a == "play":
                self._amarokPlay()
            elif a == "pause":
                self._amarokPause()
            elif a == "prev":
                self._amarokPrev()
            elif a == "next":
                self._amarokNext()
            elif a == "goto":
                self._amarokGoto(qmap["value"])
            elif a == "setvolume":
                self._amarokSetVolume(qmap["value"])
        return 1
    
    def _amarokStatus(self):
        status = AmarokStatus()
        return status
        
    def _sendFile(self, path):
        global EXEC_PATH
        # only allow doc root dir access
        elem = self.path.split("/")
        if len(elem):
            path = elem[-1]
            f = open(EXEC_PATH + "/" + path, 'r')
            self.copyfile(f, self.wfile)

    def do_HEAD(self):
        """Serve a HEAD request."""
        RequestHandler.extensions_map.update({
            '': 'application/octet-stream', # Default
            '.png': 'image/png',
            '.js': 'text/plain',
            '.css': 'text/plain'
            })
        f = self.send_head()
        if f:
            f.close()
   
    def do_GET(self):
        """Overwrite the do_GET-method of the super class."""
        RequestHandler.extensions_map.update({
            '': 'application/octet-stream', # Default
            '.png': 'image/png',
            '.js': 'text/plain',
            '.css': 'text/plain'
            })

        global REQ_ID
        if AmarokStatus.allowControl and self._handleAction():
            REQ_ID = REQ_ID + 1

        newreqid = REQ_ID + 1
        
        #
        # Surely there must be a better way that this:)
        #
        self.send_response(200)
        if ".png" in self.path:
            self.send_header("content-type","image/png")
            self.send_header("Cache-Control","no-cache")
            self.end_headers()
            self._sendFile(self.path)
        elif ".js" in self.path or ".css" in self.path:
            self.send_header("content-type","text/plain")
            self.end_headers()
            self._sendFile(self.path)
        else:
            status = self._amarokStatus()
            status.dcop_volume.init()
            status.dcop_trackcurrenttime.init()
            status.dcop_tracktotaltime.init()
            self.send_header("content-type","text/html")
            self.send_header("Cache-Control","no-cache")
            self.end_headers()
            status.reqid = newreqid
            self.wfile.write(PLIST.toHtml(status))

        self.wfile.close()

def main():
    """main is the starting-point for our script."""
    global PLIST
    PLIST = Playlist()
    srv = BaseHTTPServer.HTTPServer(('',PORT),RequestHandler)
    srv.serve_forever()


if __name__ == "__main__":
    main()

