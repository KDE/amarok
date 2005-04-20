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

    cfg = StreamConfig()
    safe_cfg = StreamConfig()
    def __init__( self ):
        QTabDialog.__init__(self)
        self.read_cfg()
        debug('init_stream')
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
        self.add_tool_tip(self.fGenre, 'Stream genre', 'genre')

        QLabel( 'Mount point', stream_lay)
        self.fMount = QLineEdit(self.cfg.mount, stream_lay)
        self.add_tool_tip(self.fMount, 'Define a name for your stream. Specifying "/myStream" here will allow clients to connect to http://<hostname>:<port>/myStream', 'mount')

        QLabel( 'Stream name', stream_lay)
        self.fName = QLineEdit(self.cfg.name, stream_lay)
        self.add_tool_tip(self.fName, 'Stream title sent to clients', 'name')

        QLabel( 'Home page', stream_lay)
        self.fURL = QLineEdit(self.cfg.url, stream_lay)
        self.add_tool_tip(self.fURL, 'URL sent to clients. Most user agents will ignore this.', 'url')

        QLabel( 'Description (line1)', stream_lay)
        self.fDesc1 = QLineEdit(self.cfg.desc1, stream_lay)
        self.add_tool_tip(self.fDesc1, 'Descriptive line sent to clients. Most user agents ignore this.', 'desc1')

        QLabel( 'Description (line2)', stream_lay)
        self.fDesc2 = QLineEdit(self.cfg.desc2, stream_lay)
        self.add_tool_tip(self.fDesc2, 'Descriptive line sent to clients. Most user agents ignore this.', 'desc2')

        self.addTab(stream_lay, '&Stream')

    def init_server(self):
        server_lay = QGrid(2, self)
        server_lay.setMargin(5)

        QLabel('Port', server_lay)        
        self.fPort = QSpinBox( 1000, 36600, 1, server_lay)
        self.fPort.setValue(int(self.cfg.port))
        self.add_tool_tip(self.fPort, 'Port to listen on. The server will bind to a different port if the configured port is unavailable', 'port' )

        QLabel('Buffer size', server_lay)
        self.fBufSize = QSpinBox( 0, 100000, 1024, server_lay)
        self.fBufSize.setValue( int(self.cfg.buf_size) )
        self.add_tool_tip(self.fBufSize, 'Each stream request keeps a part of the file being served in memory. Increasing the buffer size results in more memory needed per request but less frequent disk access. Value is specified in bytes.', 'buf_size')

        QLabel( 'Metadata update interval', server_lay)
        self.fIcyInterval = QSpinBox( 0, 100000, 4096, server_lay)
        self.fIcyInterval.setValue(int(self.cfg.icy_interval))
        self.add_tool_tip(self.fIcyInterval, 'Some user agents like amaroK and Winamp request that music metadata be periodically embedded in the data stream. The frequency at which this occurs is specified by the server. Value is in bytes.', 'icy_interval')        


        QLabel( 'Maximum number of clients', server_lay)
        self.fMaxClients = QSpinBox( 0, 100, 1, server_lay)
        self.fMaxClients.setValue( int(self.cfg.max_clients) )
        self.add_tool_tip(self.fMaxClients, 'Maximum number of active connections', 'max_clients')

        QLabel( 'Stream punctuality', server_lay)
        self.fPuncFactor = QSpinBox( 0, 100, 5, server_lay)
        self.fPuncFactor.setValue( int(self.cfg.punc_factor) )
        self.add_tool_tip(self.fPuncFactor, 'When a user agent requests a new stream, the stream punctuality determines what percent of the current file position they start from. If there is a new stream request while amaroK is currently at the 10 minute mark of a 15 minute song and sporting a 80% punctuality, (all-other factors aside) the request will begin streaming from 8:00. Value is in percent', 'punc_factor') 

        QLabel( 'Pre seek', server_lay)
        self.fPreSeek = QSpinBox( 0, 60, 1, server_lay)
        self.fPreSeek.setValue( float(self.cfg.pre_seek) )
        self.add_tool_tip(self.fPreSeek, 'This may useful for synchronizing a client with the amaroK playhead when the latency and buffering of a client is known.  WMP 9 doesn\'t allow the user to set buffering below 1 -- setting the server pre-seek to 1 second can counter some of this. Value is in seconds.', 'pre_seek' )

        QLabel( 'Supress server notification dialogs', server_lay)
        self.fSupressDialog = QCheckBox(server_lay)
        self.fSupressDialog.setChecked( self.cfg.supress_dialog )
        self.add_tool_tip(self.fSupressDialog, 'Supresses popup dialog windows. Some server messages will continue to be printed to the amaroK playlist status bar', 'supress_dialog', 'False')

        self.addTab(server_lay, 'Ser&ver')

    def init_pl(self):
        pl_lay = QGrid(2, self)
        pl_lay.setMargin(5)
        
        QLabel( 'Update streams on track change', pl_lay)
        self.fForceUpdate = QCheckBox(pl_lay)
        self.fForceUpdate.setChecked( self.cfg.force_update )
        self.connect( self.fForceUpdate, SIGNAL('clicked()'), self.on_force_update)
        self.add_tool_tip(self.fForceUpdate, 'If checked, a track change in amaroK will force all streams to update their internal playlists. This may be undesirable if whole songs and smooth playback are high priorities', 'force_update', 'False')

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
        self.add_tool_tip(self.fIdleMode, 'Stream behavior for idle periods is defined here. This situation typically arises when a user agent is has drawn a large buffer and the next track in the amaroK playlist is uncertain. Options range from sending silence to the client to getting a new file from the amaroK collection. In most cases where a file is obtained from the collection, an additional argument will need to be specified in the "Idle Argument" field', 'idle_mode', 'Bitrate')

        QLabel( 'Idle Argument', pl_lay)
        self.fIdleArg = QLineEdit(self.cfg.idle_arg, pl_lay)
        self.add_tool_tip(self.fIdleArg, 'Auxiliary argument used when a stream enters an idle state', 'idle_arg')

        self.on_idle_mode(self.fIdleMode.currentItem())
        self.on_force_update()

        QLabel( 'Probability of injection on track change', pl_lay )
        self.fInjectPct = QSpinBox( 0, 100, 5, pl_lay )
        self.fInjectPct.setValue(int(self.cfg.inject_pct))
        self.add_tool_tip(self.fInjectPct, 'Percentage of track change events into which a file is injected. This can be used to pepper a music stream with esoteric sound clips, advertisements, text-to-speech or other informative announcements.', 'inject_pct', '0')

        QLabel( 'Injection directory', pl_lay )
        self.fInjectDir = QLineEdit(self.cfg.inject_dir, pl_lay)
        self.add_tool_tip(self.fInjectDir, 'Directory from which injected files are read from', 'inject_dir')

        QLabel( 'File filter', pl_lay )
        self.fInjectFilt = QLineEdit(self.cfg.inject_filt, pl_lay)
        self.add_tool_tip(self.fInjectFilt, 'Injected file filter. Using a filter other than the default is curerntly unadvisable', 'inject_filt')

        self.connect( self.fInjectPct, SIGNAL('valueChanged(int)'), self.on_inject_pct )
        self.on_inject_pct(self.fInjectPct.value())

        self.addTab(pl_lay, '&Playlist')
        

    def init_dl(self):
        dl_lay = QGrid(2, self)

        QLabel( 'Allow file downloads', dl_lay)
        self.fEnableDl = QCheckBox(dl_lay)
        self.fEnableDl.setChecked( self.cfg.enable_dl )
        self.connect( self.fEnableDl, SIGNAL('clicked()'), self.on_enable_dl)
        self.add_tool_tip(self.fEnableDl, 'Allows files to be downloaded directly from the amaroK music collection. There is no injection of meta-data or stripping of id3 tags.', 'enable_dl')

        QLabel( 'Download mount point', dl_lay)
        self.fDlMount = QLineEdit(self.cfg.dl_mount, dl_lay)
        self.add_tool_tip(self.fDlMount, 'Download mount point. Specifying "/myFile" here will allow files to be downloaded from http://<hostname>:<port>/myFile', 'dl_mount')

        QLabel( 'Max download rate per request', dl_lay)
        self.fDlThrottle = QSpinBox(0, 1048576, 1, dl_lay)
        self.fDlThrottle.setValue( self.cfg.dl_throttle )
        self.add_tool_tip(self.fDlThrottle, 'Limit downloads to the specified rate. Value is in kB', 'dl_throttle')

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
        self.add_tool_tip(self.fReencoding, 'Specify which files should be reencoded before streaming', 'reencoding', 'None')

        QLabel( 'Stream format', enc_lay)
        self.fStreamFormat = QComboBox(enc_lay)
        #self.fStreamFormat = QComboBox(None)
        self.fStreamFormat.insertItem('mp3')
        self.fStreamFormat.insertItem('ogg')
        self.fStreamFormat.setCurrentText(self.cfg.stream_format)
        self.add_tool_tip(self.fStreamFormat, 'Format that is sent to clients. Using a value that is not "mp3" is currently unadvisable', 'stream_format')

        QLabel( 'Stream Bitrate', enc_lay)
        self.fStreamBr = QSpinBox(64, 320, 1, enc_lay)
        self.fStreamBr.setValue( self.cfg.stream_br )
        self.add_tool_tip(self.fStreamBr, 'Bitrate that files should be reencoded to. Value is in kb/s', 'stream_br')

        QLabel( 'Chunk size', enc_lay)
        self.fChunkSize = QSpinBox( 0, 1024, 32, enc_lay)
        self.fChunkSize.setValue( self.cfg.chunk_size )
        self.add_tool_tip(self.fChunkSize, 'All reencoding is done piecewise in chunks. Specifying a chunk size isn\'t needed if your machine has lots of disk space or your track lengths are shy of epic. Using a chunk-size of n, space requirements should be in the neighborhood of 10*n, possibly (a lot) more depending on the number of connected clients and their relative separation in the stream. All intermediate files are written to /tmp. Value is in kb. Specifying 0 disables chunking', 'chunk_size')

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

    def add_tool_tip(self, widget, msg, option=None, default=None):
        # Do something about reformating msg to fit a reasonable width
        l_width = 80
        msg, msg_s = list(msg), ""
        while msg:
            for c in msg[:l_width]:
                msg_s += msg.pop(0)
            msg_s += '\n'

        if option:
            if not default: 
                default = eval('self.safe_cfg.' + option)
                if not default: default = '(blank)'
            msg_s = msg_s + '\n\nDefault: %s' % default
        QToolTip.add( widget, msg_s )

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
