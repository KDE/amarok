############################################################################
# Configuration window for shouter script
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

from qt import *
from StreamConfig import *
from debug import *
import ConfigParser

class ConfigDialog( QTabDialog ):
    """ Configuration widget """

    cfg = StreamConfig
    def __init__( self ):
        QTabDialog.__init__(self)
        self.read_cfg()
        self.init_stream()
        self.init_server()
        self.init_pl()
        self.init_dl()
        self.init_enc()
        self.setCancelButton("Cancel")
        self.connect( self, SIGNAL("applyButtonPressed()"), self.save) 
        self.connect( self, SIGNAL("okButtonPressed()"), self.on_ok) 
        self.connect( self, SIGNAL("cancelButtonPressed()"), SLOT("reject()")) 
        self.setCaption( "Shouter - amaroK" )

    def read_cfg(self):
        try:
            config = ConfigParser.ConfigParser()
            config.read( "shouterrc" )

            self.cfg.genre              = config.get( 'Stream', 'genre' )
            self.cfg.mount              = config.get( 'Stream', 'mount' )
            self.cfg.name               = config.get( 'Stream', 'name' )
            self.cfg.url                = config.get( 'Stream', 'url' )
            self.cfg.desc1              = config.get( 'Stream', 'desc1' )
            self.cfg.desc2              = config.get( 'Stream', 'desc2' )

            self.cfg.icy_interval       = config.getint( 'Server', 'icy_interval' )
            self.cfg.max_clients        = config.getint( 'Server', 'max_clients' )
            self.cfg.port               = config.getint( 'Server', 'port' )
            self.cfg.buf_size           = config.getint( 'Server', 'buf_size' )
            self.cfg.punc_factor        = config.getint( 'Server', 'punc_factor' )
            self.cfg.pre_seek           = config.getfloat( 'Server', 'pre_seek' )
            self.cfg.supress_dialog     = config.getboolean( 'Server', 'supress_dialog' )

            self.cfg.force_update       = config.getboolean( 'Playlist', 'force_update' )
            self.cfg.idle_mode          = config.getint( 'Playlist', 'idle_mode' )
            self.cfg.idle_arg           = config.get( 'Playlist', 'idle_arg' )
            self.cfg.inject_pct         = config.getint( 'Playlist', 'inject_pct' )
            self.cfg.inject_dir         = config.get( 'Playlist', 'inject_dir' )
            self.cfg.inject_filt        = config.get( 'Playlist', 'inject_filt' )

            self.cfg.enable_dl          = config.getboolean( 'Downloads', 'enable_dl' )
            self.cfg.dl_mount           = config.get( 'Downloads', 'dl_mount' )
            self.cfg.dl_throttle        = config.getint( 'Downloads', 'dl_throttle' )

            self.cfg.reencoding         = config.getint( 'Encoding', 'reencoding' )
            self.cfg.format             = config.get( 'Encoding', 'stream_format' )
            self.cfg.stream_br          = config.getint( 'Encoding', 'stream_br' )
            self.cfg.chunk_size         = config.getint( 'Encoding', 'chunk_size' )


            self.cfg.chunk_size /= 1024

        except:
            pass

    def init_stream(self):
        stream_lay = QGrid(2, self)
        stream_lay.setMargin(5)
        QLabel('Genre', stream_lay)
        self.fGenre = QLineEdit(self.cfg.genre, stream_lay)

        QLabel( 'Mount point', stream_lay)
        self.fMount = QLineEdit(self.cfg.mount, stream_lay)

        QLabel( 'Stream name', stream_lay)
        self.fName = QLineEdit(self.cfg.name, stream_lay)

        QLabel( 'Home page', stream_lay)
        self.fURL = QLineEdit(self.cfg.url, stream_lay)

        QLabel( 'Description (line1)', stream_lay)
        self.fDesc1 = QLineEdit(self.cfg.desc1, stream_lay)

        QLabel( 'Description (line2)', stream_lay)
        self.fDesc2 = QLineEdit(self.cfg.desc2, stream_lay)
        self.addTab(stream_lay, '&Stream')

    def init_server(self):
        server_lay = QGrid(2, self)
        server_lay.setMargin(5)

        QLabel('Port', server_lay)        
        self.fPort = QSpinBox( 1000, 36600, 1, server_lay)
        self.fPort.setValue(int(self.cfg.port))

        QLabel('Buffer size', server_lay)
        self.fBufSize = QSpinBox( 0, 100000, 1024, server_lay)
        self.fBufSize.setValue( int(self.cfg.buf_size) )

        QLabel( 'Metadata update interval (bytes)', server_lay)
        self.fIcyInterval = QSpinBox( 0, 100000, 4096, server_lay)
        self.fIcyInterval.setValue(int(self.cfg.icy_interval))

        QLabel( 'Maximum number of clients', server_lay)
        self.fMaxClients = QSpinBox( 0, 100, 1, server_lay)
        self.fMaxClients.setValue( int(self.cfg.max_clients) )

        QLabel( 'Stream punctuality (%)', server_lay)
        self.fPuncFactor = QSpinBox( 0, 100, 5, server_lay)
        self.fPuncFactor.setValue( int(self.cfg.punc_factor) )

        QLabel( 'Pre seek (seconds)', server_lay)
        self.fPreSeek = QSpinBox( 0, 60, 1, server_lay)
        self.fPreSeek.setValue( float(self.cfg.pre_seek) )

        QLabel( 'Supress server notification dialogs', server_lay)
        self.fSupressDialog = QCheckBox(server_lay)
        self.fSupressDialog.setChecked( self.cfg.supress_dialog )

        self.addTab(server_lay, 'Ser&ver')

    def init_pl(self):
        pl_lay = QGrid(2, self)
        pl_lay.setMargin(5)
        
        QLabel( 'Update streams on track change', pl_lay)
        self.fForceUpdate = QCheckBox(pl_lay)
        self.fForceUpdate.setChecked( self.cfg.force_update )
        self.connect( self.fForceUpdate, SIGNAL('clicked()'), self.on_force_update)

        QLabel( 'Idle Mode', pl_lay)
        self.fIdleMode = QComboBox(pl_lay)
        i = iter(IDLE_MODES)
        while 1:
           try:
               self.fIdleMode.insertItem(i.next())
           except StopIteration:
               break
        self.fIdleMode.setCurrentItem(self.cfg.idle_mode)
        self.connect( self.fIdleMode, SIGNAL('activated(int)'), self.on_idle_mode )

        QLabel( 'Idle Argument', pl_lay)
        self.fIdleArg = QLineEdit(self.cfg.idle_arg, pl_lay)

        self.on_idle_mode(self.fIdleMode.currentItem())
        self.on_force_update()
        self.addTab(pl_lay, '&Playlist')

        QLabel( 'Probability of injection on track change', pl_lay )
        self.fInjectPct = QSpinBox( 0, 100, 5, pl_lay )
        self.fInjectPct.setValue(int(self.cfg.inject_pct))

        QLabel( 'Injection directory', pl_lay )
        self.fInjectDir = QLineEdit(self.cfg.inject_dir, pl_lay)

        QLabel( 'File filter', pl_lay )
        self.fInjectFilt = QLineEdit(self.cfg.inject_filt, pl_lay)

        self.connect( self.fInjectPct, SIGNAL('valueChanged(int)'), self.on_inject_pct )
        self.on_inject_pct(self.fInjectPct.value())
        

    def init_dl(self):
        dl_lay = QGrid(2, self)

        QLabel( 'Allow file downloads', dl_lay)
        self.fEnableDl = QCheckBox(dl_lay)
        self.fEnableDl.setChecked( self.cfg.enable_dl )
        self.connect( self.fEnableDl, SIGNAL('clicked()'), self.on_enable_dl)

        QLabel( 'Download mount point', dl_lay)
        self.fDlMount = QLineEdit(self.cfg.dl_mount, dl_lay)

        QLabel( 'Max download rate per request (kB/s)', dl_lay)
        self.fDlThrottle = QSpinBox(0, 1048576, 1, dl_lay)
        self.fDlThrottle.setValue( self.cfg.dl_throttle )

        self.on_enable_dl()
        self.addTab(dl_lay, '&Downloads')


    def init_enc(self):
        enc_lay = QGrid(2, self)

        QLabel( 'Reencoding', enc_lay)
        self.fReencoding = QComboBox(enc_lay)
        self.fReencoding.insertItem( 'None' )
        self.fReencoding.insertItem( 'Different formats only' )
        self.fReencoding.insertItem( 'All tracks')
        self.fReencoding.setCurrentItem(self.cfg.reencoding)
        self.connect(self.fReencoding, SIGNAL('activated(int)'), self.on_reencoding )

        # Disabled for now
        #QLabel( 'Stream format', enc_lay)
        #self.fStreamFormat = QComboBox(enc_lay)

        self.fStreamFormat = QComboBox(None)
        self.fStreamFormat.insertItem('mp3')
        self.fStreamFormat.insertItem('ogg')
        self.fStreamFormat.setCurrentText(self.cfg.stream_format)

        QLabel( 'Stream Bitrate (kb/s)', enc_lay)
        self.fStreamBr = QSpinBox(64, 320, 1, enc_lay)
        self.fStreamBr.setValue( self.cfg.stream_br )

        QLabel( 'Chunk size (kb. \'0\' disables chunking)', enc_lay)
        self.fChunkSize = QSpinBox( 0, 1024, 32, enc_lay)
        self.fChunkSize.setValue( self.cfg.chunk_size )

        self.on_reencoding(self.fReencoding.currentItem())
        self.addTab(enc_lay, '&Encoding')

    def on_force_update(self):
        pass

    def on_idle_mode(self, index):
        if index==0 or index==5 or index==6:
            self.fIdleArg.setDisabled(True)
        else:
            self.fIdleArg.setDisabled(False)

    def on_enable_dl(self):
        state = not self.fEnableDl.isChecked()
        self.fDlMount.setDisabled(state)
        self.fDlThrottle.setDisabled(state)

    def on_inject_pct(self, pct):
        self.fInjectDir.setDisabled(not bool(pct))
        self.fInjectFilt.setDisabled(not bool(pct))

    def on_reencoding(self, index):
        state = not index
        self.fStreamBr.setDisabled(state)
        self.fChunkSize.setDisabled(state)

    def on_ok(self):
        debug('on_ok')

        self.save()
        self.close()

    def save( self ):
        """ Saves configuration to file """

        debug( 'save' )
        try:
            self.file = open( 'shouterrc', 'w' )

            self.config = ConfigParser.ConfigParser()
            self.config.add_section( 'Stream' )
            self.config.set( 'Stream', 'genre', self.fGenre.text() )
            mount = self.fMount.text()
            if not mount.startsWith('/'): mount.prepend('/')
            self.config.set( 'Stream', 'mount', mount)
            self.config.set( 'Stream', 'name', self.fName.text() )
            self.config.set( 'Stream', 'url', self.fURL.text() )
            self.config.set( 'Stream', 'desc1', self.fDesc1.text() )
            self.config.set( 'Stream', 'desc2', self.fDesc2.text() )

            self.config.add_section( 'Server' )
            self.config.set( 'Server', 'port', self.fPort.text() )
            self.config.set( 'Server', 'buf_size', self.fBufSize.text() )
            self.config.set( 'Server', 'icy_interval', self.fIcyInterval.text() )
            self.config.set( 'Server', 'max_clients', self.fMaxClients.text() )
            self.config.set( 'Server', 'punc_factor', self.fPuncFactor.text() )
            self.config.set( 'Server', 'pre_seek', self.fPreSeek.text() )
            self.config.set( 'Server', 'supress_dialog', self.fSupressDialog.isChecked() )

            self.config.add_section( 'Playlist' )
            self.config.set( 'Playlist', 'force_update', self.fForceUpdate.isChecked() )
            self.config.set( 'Playlist', 'idle_mode', self.fIdleMode.currentItem() )
            self.config.set( 'Playlist', 'idle_arg', self.fIdleArg.text() )
            self.config.set( 'Playlist', 'inject_pct', self.fInjectPct.text() )
            self.config.set( 'Playlist', 'inject_dir', self.fInjectDir.text() )
            self.config.set( 'Playlist', 'inject_filt', self.fInjectFilt.text() )

            self.config.add_section( 'Downloads' )
            self.config.set( 'Downloads', 'enable_dl', self.fEnableDl.isChecked() )
            dlmount = self.fDlMount.text()
            if not dlmount.startsWith('/'): dlmount.prepend('/')
            self.config.set( 'Downloads', 'dl_mount', dlmount )
            self.config.set( 'Downloads', 'dl_throttle', self.fDlThrottle.text() )

            self.config.add_section( 'Encoding' )
            self.config.set( 'Encoding', 'reencoding', self.fReencoding.currentItem())
            self.config.set( 'Encoding', 'stream_format', self.fStreamFormat.currentText() )
            self.config.set( 'Encoding', 'stream_br', self.fStreamBr.text() )
            self.config.set( 'Encoding', 'chunk_size', self.fChunkSize.value() * 1024)

            self.config.write( self.file )
            self.file.close()
        except:
            debug( 'Error while writing configuration' )
            raise

        self.accept()
