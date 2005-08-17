############################################################################
# Configuration window for shouter script
# (c) 2005 James Bellenger <jamesb@squaretrade.com>
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

from qt import *
from StreamConfig import *
from debug import *
from Amarok import *
import sys
import traceback
import os
import Playlist
import ShouterExceptions

class TabState:
    cfg = None
    t_i = 0
    widget = None
    def __init__(self, t_i):
        debug('init tabstate at t_i %d' % t_i)
        self.t_i = t_i

class ServerTabState(TabState):
    #f_buf_size = None
    #f_chunk_size = None
    f_desc1 = None
    f_desc2 = None
    f_dl_throttle = None
    #f_icy_interval = None
    f_max_clients = None
    f_port = None
    f_url = None
    f_publish = None

    def __init__(self, t_i):
        self.cfg = ServerConfig()
        TabState.__init__(self, t_i)

    def get_config(self):
        #self.cfg = ServerConfig()
        #self.cfg.buf_size = int(self.f_buf_size.value())
        #self.cfg.chunk_size = int(self.f_chunk_size.value())
        self.cfg.desc1 = str(self.f_desc1.text())
        self.cfg.desc2 = str(self.f_desc2.text())
        self.cfg.dl_throttle = int(self.f_dl_throttle.value())
        #self.cfg.icy_interval = int(self.f_icy_interval.value())
        self.cfg.max_clients = int(self.f_max_clients.value())
        self.cfg.port = int(self.f_port.value())
        self.cfg.url = str(self.f_url.text())
        if self.f_publish.isChecked(): self.cfg.publish = 1
        else: self.cfg.publish = 0
        return self.cfg

class StreamTabState(TabState):
    f_mount = None
    f_stream_type = None
    f_genre = None
    f_name = None
    f_stream_type_arg_button = None
    f_stream_type_arg = None

    def __init__(self, t_i):
        self.cfg = StreamConfig()
        TabState.__init__(self, t_i)

    def get_config(self):
        self.cfg.random = Amarok.random_status()
        self.cfg.repeat_tr = Amarok.repeat_tr_status()
        self.cfg.repeat_pl = Amarok.repeat_pl_status()
        self.cfg.mount = str(self.f_mount.text())
        self.cfg.stream_type = int(self.f_stream_type.currentItem())
        self.cfg.genre = str(self.f_genre.text())
        self.cfg.name = str(self.f_name.text())
        return self.cfg

class ConfigDialog(QTabDialog):
    """ Configuration widget """

    cfg_mgr = None;
    tab_states = []

    def __init__(self):
        debug('ConfigDialog init')
        QTabDialog.__init__(self)
        self.tab_states = []
        self.setWFlags(Qt.WDestructiveClose)
        self.cfg_mgr = ConfigManager()

        try:
            self.tab_states.append(ServerTabState(len(self.tab_states)))
            self._init_server_tab(self.cfg_mgr.server_cfg, 0)

            for sc in self.cfg_mgr.stream_cfgs:
                self.add_stream_tab(sc)
            
            # cheat by hijacking the help/defaults buttons for +/- stream
            self.setOkButton('&Save')
            self.setHelpButton('&Add Stream')
            self.setDefaultButton('&Drop Stream')
            self.setCancelButton('&Cancel')
            self.connect(self, SIGNAL('applyButtonPressed()'), self._on_ok)
            self.connect(self, SIGNAL('cancelButtonPressed()'), SLOT('reject()'))
            self.connect(self, SIGNAL('helpButtonPressed()'), self._on_add_stream)
            self.connect(self, SIGNAL('defaultButtonPressed()'), self._on_drop_stream)
            self.setCaption('amaroK Shouter')
        except Exception, e:
            debug('Caught exception in init: ' + str(sys.exc_info()[1]))
            raise
    
    def _init_server_tab(self, server_cfg, i):
        debug('init tab %d' % i)
        lay = QGrid(2, self)
        lay.setSpacing(10)
        lay.setMargin(3)
        ts = self.tab_states[i]
        ts.widget = lay
        ts.cfg = server_cfg

        #QLabel('Buffer size', lay)
        #ts.f_buf_size = QSpinBox(0, 100000, 1024, lay)
        #ts.f_buf_size.setValue(server_cfg.buf_size)

        #QLabel('Chunk size', lay)
        #ts.f_chunk_size = QSpinBox(0, 1024, 32, lay)
        #ts.f_chunk_size.setValue(server_cfg.chunk_size)

        QLabel('Description (line1)', lay)
        ts.f_desc1 = QLineEdit(server_cfg.desc1, lay)

        QLabel('Description (line2)', lay)
        ts.f_desc2 = QLineEdit(server_cfg.desc2, lay)

        QLabel('Max download rate per request', lay)
        ts.f_dl_throttle = QSpinBox(0, 1048576, 1, lay)
        ts.f_dl_throttle.setValue(server_cfg.dl_throttle)

        #QLabel('Metadata update interval', lay)
        #ts.f_icy_interval = QSpinBox(0, 100000, 4096, lay)
        #ts.f_icy_interval.setValue(server_cfg.icy_interval)

        QLabel('Maximum number of clients', lay)
        ts.f_max_clients = QSpinBox(0, 100, 1, lay)
        ts.f_max_clients.setValue(server_cfg.max_clients)

        QLabel('Port', lay)        
        ts.f_port = QSpinBox(1000, 36600, 1, lay) 
        ts.f_port.setValue(server_cfg.port)

        QLabel('Home page', lay)
        ts.f_url = QLineEdit(server_cfg.url, lay)
	
        ts.f_publish = QCheckBox('Announce streams on LAN',lay)
        ts.f_publish.setChecked(server_cfg.publish == 1)

        self.addTab(lay, 'Ser&ver')

    def _init_stream_tab(self, stream_cfg, i):
        debug('init tab %d' % i)
        lay = QGrid(2, self)
        lay.setSpacing(10)
        lay.setMargin(3)
        ts = self.tab_states[i]
        ts.cfg = stream_cfg
        ts.widget = lay

        QLabel( 'Mount point', lay)
        ts.f_mount = QLineEdit(stream_cfg.mount, lay)
        self.connect( ts.f_mount, 
                      SIGNAL('textChanged(const QString &)'), 
                      self._on_mount)

        QLabel('Genre', lay)
        ts.f_genre = QLineEdit(stream_cfg.genre, lay)

        QLabel( 'Stream name', lay)
        ts.f_name = QLineEdit(stream_cfg.name, lay)

        QLabel( 'Stream Type', lay)
        ts.f_stream_type = QComboBox(lay)
        for ss in STREAM_TYPES:
            ts.f_stream_type.insertItem(ss)
        ts.f_stream_type.setCurrentItem(stream_cfg.stream_type)

        QLabel('Stream argument', lay)
        hbox = QHBox(lay)
        hbox.setSpacing(10)
        ts.f_stream_type_arg = QLineEdit(hbox)
        arg = str(eval('stream_cfg.stream_type%d_arg' % stream_cfg.stream_type))
        ts.f_stream_type_arg.setText(arg)
        ts.f_stream_type_arg_button = QPushButton('Configure...', hbox)

        self._on_stream_type(stream_cfg.stream_type, t_i=i)
        self.connect( ts.f_stream_type, 
                      SIGNAL('activated(int)'), 
                      self._on_stream_type)
        self.connect( ts.f_stream_type_arg_button, 
                      SIGNAL('clicked()'), 
                      self._on_config_stream_type)

        self.addTab(lay, stream_cfg.mount)

    def add_stream_tab(self, stream_config):
        self.tab_states.append(StreamTabState(len(self.tab_states)))
        self._init_stream_tab(stream_config, len(self.tab_states)-1)

    def remove_stream_tab(self, i):
        debug('remove stream %d' % i)
        if i:
            tb = self.tabBar()
            # This causes some silent crashes. Don't know why
            #t = tb.tabAt(i)
            t = tb.tab(i)
            tb.removeTab(t)

            try:
                self.tab_states.pop(i)
            except IndexError:
                # I'm not myself sure why an IndexError gets _sometimes_ thrown
                # when trying to kill the last tab. 
                self.tab_states.pop()

            tb.setCurrentTab(0)
            self.showPage(self.tab_states[0].widget)

    def _on_mount(self, txt):
        tb = self.tabBar()
        tb.tab(tb.currentTab()).setText(txt)
        t_i = self.tabBar().currentTab()
        if not self._check_mount(txt):
            debug('mount conflict')

    def _check_mount(self, mount, add_stream=False):
        # TODO: Clean this up, possibly split 
        if not len(mount):
            return False

        t_i = self.tabBar().currentTab()
        for i in range(1, len(self.tab_states)):
            ts = self.tab_states[i]
            if add_stream:
                if mount == ts.f_mount.text(): 
                    return False
            elif not ts.f_mount.hasFocus():
                if mount == ts.f_mount.text():
                    return False
        return True

    def _on_add_stream(self):
        try:
            mount, n = 'amarok', 1
            if not self._check_mount(mount, True):
                mount = 'stream%d'
                while not self._check_mount(mount % n, True):
                    n += 1
                mount = mount % n
            sc = StreamConfig()
            sc.mount = mount
            self.add_stream_tab(sc)
            self.showPage(self.tab_states[-1].widget)
        except:
            debug(sys.exc_info()[1])
            raise

    def _get_current_tab_state(self):
        w = self.currentPage()
        for ts in self.tab_states[1:]:
            if ts.widget == w:
                return ts
        #return self.tab_states[0]

    def _on_drop_stream(self):
        self.remove_stream_tab(self.tabBar().currentTab())

    def _on_stream_type(self, i, t_i=None):
        w = self.currentPage()
        for ts in self.tab_states[1:]:
            if ts.widget == w:
                break
        #ts = self._get_current_tab_state()
        b = ts.f_stream_type_arg_button
        if i:
            b.setDisabled(False)
            ts.f_stream_type_arg.setDisabled(False)
        else:
            b.setDisabled(True)
            ts.f_stream_type_arg.setDisabled(True)
        arg = str(eval('ts.cfg.stream_type%d_arg' % i))
        ts.f_stream_type_arg.setText(arg)
            
    def _on_config_stream_type(self):
        #w = self.currentPage()
        #for ts in self.tab_states[1:]:
            #if ts.widget == w:
                #break
        ts = self._get_current_tab_state()
        st = ts.f_stream_type.currentItem()
        arg = ''
        if st == 0:
            # live stream
            pass
        elif st == 1:
            # snapshot playlist
            arg = Playlist.save_current_as(str(ts.f_mount.text()))
            warnings = []
            if not Playlist.validate(arg):
                warnings.append('Some entries in your playlist have an \
                indeterminate length, bad bitrate or are of an unsupported \
                format and will be ignored')
            if not ts.get_config().repeat_pl:
                warnings.append('You have not enabled the repeat playlist option.\
                This could make for a rather shortlived stream')
            if warnings:
                msg = '<qt><b>Your playlist has been saved, but with the following warnings:</b>'
                msg += '<ul>'
                for w in warnings: msg += '<li>%s</li>' % w
                msg += '</ul>'
                msg += '</qt>'
                os.popen("kdialog --title 'amaroK Shouter warning' \
                    --sorry '%s'" % msg )
        elif st == 2:
            # directory
            # A bit slower but orders of magnitude prettier, let's use kdialog
            # instead of QDialog
            dir = os.popen('kdialog --title "Select directory" --getexistingdirectory ~').readline()
            arg = dir

        elif st == 3:
            dir = os.popen('kdialog --title "Select directory to save uploaded files" --getexistingdirectory ~').readline()
            arg = dir

        ts.f_stream_type_arg.setText(arg)
        setattr(ts.cfg, 'stream_type%d_arg' % st, arg)
            
    def save(self):
        try:
            ts = self.tab_states[0]
            self.cfg_mgr.server_cfg = ts.get_config()

            cfgs = []
            for ts in self.tab_states[1:]:
                cfg = ts.get_config()
                debug('saving ' + cfg.mount)
                cfgs.append(cfg)
            self.cfg_mgr.stream_cfgs = cfgs
            self.cfg_mgr.server_cfg = self.tab_states[0].get_config()

            self.cfg_mgr.save()
        except Exception, e:
            debug('Caught exception in save: ' + str(sys.exc_info()[1]))
            raise

    def _on_ok(self):
        self.save()
        self.accept()
