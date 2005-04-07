from qt import *
from StreamConfig import *
from debug import *
import ConfigParser

class ConfigDialog( QDialog ):
    """ Configuration widget """

    cfg = StreamConfig
    def __init__( self ):
        QDialog.__init__( self )
        self.setSizePolicy( QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding))
        self.setMinimumHeight(400)
        self.setMinimumWidth(300)
        self.setWFlags( Qt.WDestructiveClose )
        self.setCaption( "Shouter - amaroK" )

        try:
            config = ConfigParser.ConfigParser()
            config.read( "shouterrc" )

            self.cfg.genre     = config.get( 'Stream', 'genre' )
            self.cfg.mount     = config.get( 'Stream', 'mount' )
            self.cfg.name      = config.get( 'Stream', 'name' )
            self.cfg.url    = config.get( 'Stream', 'url' )
            self.cfg.desc1     = config.get( 'Stream', 'desc1' )
            self.cfg.desc2     = config.get( 'Stream', 'desc2' )

            self.cfg.icy_interval     = config.getint( 'Server', 'icy_interval' )
            self.cfg.max_clients     = config.getint( 'Server', 'max_clients' )
            self.cfg.port          = config.getint( 'Server', 'port' )
            self.cfg.buf_size     = config.getint( 'Server', 'buf_size' )
            self.cfg.punc_factor     = config.getint( 'Server', 'punc_factor' )
            self.cfg.force_update     = config.getboolean( 'Server', 'force_update' )
            self.cfg.pre_seek     = config.getfloat( 'Server', 'pre_seek' )

            self.cfg.enable_dl     = config.getboolean( 'Downloads', 'enable_dl' )
            self.cfg.dl_mount     = config.get( 'Downloads', 'dl_mount' )
            self.cfg.dl_throttle     = config.getint( 'Downloads', 'dl_throttle' )

            self.cfg.reencoding     = config.getint( 'Encoding', 'reencoding' )
            self.cfg.stream_format     = config.get( 'Encoding', 'stream_format' )
            self.cfg.stream_br     = config.getint( 'Encoding', 'stream_br' )
            self.cfg.chunk_size     = config.getint( 'Encoding', 'chunk_size' )

            self.cfg.chunk_size /= 1024

        except:
            pass

        self.vbox = QVBox(self)
        self.vbox.setMargin(4)
        self.vbox.setSpacing(10)
        self.vbox.setMinimumHeight(700)
        self.vbox.setMinimumWidth(500)

        self.fStream = QGroupBox(self.vbox)
        #self.fStream = QGroupBox(self)
        self.fStream.setColumns(2)
        self.fStream.setTitle('Stream')
        self.fStream.setMinimumHeight(200)
        self.fStream.setMinimumWidth(200)

        QLabel('Genre', self.fStream)
        self.fGenre = QLineEdit(self.cfg.genre, self.fStream)

        QLabel( 'Mount point', self.fStream)
        self.fMount = QLineEdit(self.cfg.mount, self.fStream)

        QLabel( 'Stream name', self.fStream)
        self.fName = QLineEdit(self.cfg.name, self.fStream)

        QLabel( 'Home page', self.fStream)
        self.fURL = QLineEdit(self.cfg.url, self.fStream)

        QLabel( 'Description (line1)', self.fStream)
        self.fDesc1 = QLineEdit(self.cfg.desc1, self.fStream)

        QLabel( 'Description (line2)', self.fStream)
        self.fDesc2 = QLineEdit(self.cfg.desc2, self.fStream)

        #####################
        self.fServer = QGroupBox(self.vbox)
        self.fServer.setColumns(2)
        self.fServer.setTitle('Server')

        QLabel('Port', self.fServer)        
        self.fPort = QSpinBox( 1000, 36600, 1, self.fServer)
        self.fPort.setValue(int(self.cfg.port))

        QLabel('Buffer size', self.fServer)
        self.fBufSize = QSpinBox( 0, 100000, 1024, self.fServer)
        self.fBufSize.setValue( int(self.cfg.buf_size) )

        QLabel( 'Metadata update interval (bytes)', self.fServer)
        self.fIcyInterval = QSpinBox( 0, 100000, 4096, self.fServer)
        self.fIcyInterval.setValue(int(self.cfg.icy_interval))

        QLabel( 'Maximum number of clients', self.fServer)
        self.fMaxClients = QSpinBox( 0, 100, 1, self.fServer)
        self.fMaxClients.setValue( int(self.cfg.max_clients) )

        QLabel( 'Stream punctuality (%)', self.fServer)
        self.fPuncFactor = QSpinBox( 0, 100, 5, self.fServer)
        self.fPuncFactor.setValue( int(self.cfg.punc_factor) )

        QLabel( 'Pre seek (seconds)', self.fServer)
        self.fPreSeek = QSpinBox( 0, 60, 1, self.fServer)
        self.fPreSeek.setValue( float(self.cfg.pre_seek) )

        QLabel( 'Update streams on track change', self.fServer)
        self.fForceUpdate = QCheckBox(self.fServer)
        self.fForceUpdate.setChecked( self.cfg.force_update )

        #####################
        self.fDownloads = QGroupBox(self.vbox)
        self.fDownloads.setColumns(2)
        self.fDownloads.setTitle('Downloads')

        QLabel( 'Allow file downloads', self.fDownloads)
        self.fEnableDl = QCheckBox(self.fDownloads)
        self.fEnableDl.setChecked( self.cfg.enable_dl )

        QLabel( 'Download mount point', self.fDownloads)
        self.fDlMount = QLineEdit(self.cfg.dl_mount, self.fDownloads)

        QLabel( 'Max download rate per request (kB/s)', self.fDownloads)
        self.fDlThrottle = QSpinBox(0, 1048576, 1, self.fDownloads)
        self.fDlThrottle.setValue( self.cfg.dl_throttle )

        #####################
        self.fEncoding = QGroupBox(self.vbox)
        self.fEncoding.setColumns(2)
        self.fEncoding.setTitle('Encoding')
        self.fEncoding.setDisabled(True)

        QLabel( 'Reencoding', self.fEncoding)
        self.fReencoding = QComboBox(self.fEncoding)
        self.fReencoding.insertItem( 'None' )
        self.fReencoding.insertItem( 'Different formats only' )
        self.fReencoding.insertItem( 'All tracks')
        self.fReencoding.setCurrentItem(self.cfg.reencoding)

        QLabel( 'Stream format', self.fEncoding)
        self.fStreamFormat = QComboBox(self.fEncoding)
        self.fStreamFormat.insertItem('mp3')
        self.fStreamFormat.insertItem('ogg')
        self.fStreamFormat.setCurrentText(self.cfg.stream_format)

        QLabel( 'Stream Bitrate (kb/s)', self.fEncoding)
        self.fStreamBr = QSpinBox(64, 320, 1, self.fEncoding)
        self.fStreamBr.setValue( self.cfg.stream_br )

        QLabel( 'Chunk size (kb. \'0\' disables chunking)', self.fEncoding)
        self.fChunkSize = QSpinBox( 0, 1024, 32, self.fEncoding)
        self.fChunkSize.setValue( self.cfg.chunk_size )

        hbox = QHBox(self.vbox)
        self.ok = QPushButton(hbox)
        self.ok.setText( 'Save' )
        self.cancel = QPushButton(hbox)
        self.cancel.setText( 'Cancel' )

        self.connect( self.ok,         SIGNAL( 'clicked()' ), self.save )
        self.connect( self.cancel,     SIGNAL( 'clicked()' ), self, SLOT( 'reject()' ) )
        self.adjustSize()

    def save( self ):
        """ Saves configuration to file """

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
            self.config.set( 'Server', 'force_update', self.fForceUpdate.isChecked() )

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
