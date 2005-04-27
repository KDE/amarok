import ConfigParser
from exceptions import *
from debug import *
import sys

#STREAM_TYPES = [ 'Now playing', 'Playlist snapshot', 'Directory', 'Upload' ]
STREAM_TYPES = [ 'Now playing', 'Playlist snapshot']

class ServerConfig:
    """ Master server configuration """

    buf_size = 4096
    chunk_size = 524288
    desc1, desc2 = '', ''
    dl_throttle = 20
    icy_interval = 16384
    max_clients = 4
    port = 8000
    url = 'http://amarok.kde.org'

class StreamConfig:
    """ Container for stream configuration. Values are defaults and should be
    reset when the configuration is loaded from disk """

    mount = 'amarok'
    stream_type = 0
    genre = 'Mixed'
    name = 'amaroK shouter'
    repeat_pl = 1
    repeat_tr = 0
    random = 0
    stream_type0_arg = ''
    stream_type1_arg = ''
    stream_type2_arg = '~'
    stream_type3_arg = '~'
    #random = 0

class ConfigManager:
    cfg_parser = None
    server_cfg = None
    stream_cfgs = []

    def __init__(self):
        debug('ConfigManager init')
        self.cfg_parser = ConfigParser.ConfigParser()
        self.cfg_parser.read('shouterrc')
        self.stream_cfgs = []
        self.server_cfg = None

        try:
            self.server_cfg = self.get_server_cfg()
            for s in self.cfg_parser.sections():
                if s != 'Server': 
                    self.stream_cfgs.append(self.get_stream_cfg(s))
        except:
            debug('Exception in loading from config. Using defaults: %s' % sys.exc_info()[0])
            raise
        debug('ConfigManager parsed %d sections' % len(self.stream_cfgs))

    def get_server_cfg(self):
        #debug('Loading server cfg')
        cfg = ServerConfig()
        try:
            for k in dir(ServerConfig):
                if not k.startswith('_'):
                    try:
                        setattr(cfg, k, self.cfg_parser.getint('Server', k))
                    except ValueError, AttributeError:
                        setattr(cfg, k, self.cfg_parser.get('Server', k))
            return cfg
        except:
            debug('get_server_cfg caught exception')
            return ServerConfig()

    def _save_server_cfg(self, cfg_parser):
        debug('ConfigManager save server cfg')
        cfg_parser.add_section('Server')
        for k in dir(ServerConfig):
            if not k.startswith('_'):
                #debug(k)
                cfg_parser.set('Server', k, eval('self.server_cfg.%s' % k))
        #debug('ConfigManager end save server cfg')
    
    def get_stream_cfg(self, section):
        cfg = StreamConfig()
        for k in dir(StreamConfig):
            if not k.startswith('_'):
                try:
                    setattr(cfg, k, self.cfg_parser.getint(section, k))
                except ValueError:
                    setattr(cfg, k, self.cfg_parser.get(section, k))
        return cfg

    def _save_stream_cfg(self, sc, cfg_parser):
        debug('ConfigManager save config for ' + sc.mount)
        cfg_parser.add_section(sc.mount)
        for k in dir(StreamConfig):
            if not k.startswith('_'):
                cfg_parser.set(sc.mount, k, eval('sc.%s' % k))
        #debug('ConfigManager end save config for ' + section)

    def save(self):
        debug('ConfigManager save')
        f = file('shouterrc', 'w')
        cfg_parser = ConfigParser.ConfigParser()
        for sc in self.stream_cfgs:
            self._save_stream_cfg(sc, cfg_parser)
        self._save_server_cfg(cfg_parser)
        cfg_parser.write(f)
        f.close()
            


