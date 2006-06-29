// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information


#define DEBUG_PREFIX "MediaBrowser"

#include <config.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "browserToolBar.h"
#include "clicklineedit.h"
#include "collectiondb.h"
#include "colorgenerator.h"
#include "contextbrowser.h"
#include "debug.h"
#include "deviceconfiguredialog.h"
#include "devicemanager.h"
#include "expression.h"
#include "hintlineedit.h"
#include "mediabrowser.h"
#include "medium.h"
#include "mediumpluginmanager.h"
#include "metabundle.h"
#include "playlist.h"
#include "playlistbrowseritem.h"
#include "playlistloader.h"
#include "pluginmanager.h"
#include "podcastbundle.h"
#include "scriptmanager.h"
#include "scrobbler.h"
#include "statusbar.h"
#include "transferdialog.h"
#include "browserToolBar.h"

#include <qvbuttongroup.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qdom.h>
#include <qfileinfo.h>
#include <qgroupbox.h>
#include <qheader.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qradiobutton.h>
#include <qsimplerichtext.h>
#include <qtimer.h>
#include <qtooltip.h>       //QToolTip::add()

#include <kapplication.h> //kapp
#include <kcombobox.h>
#include <kdirlister.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kprocess.h>
#include <kprogress.h>
#include <kpushbutton.h>
#include <krun.h>
#include <kstandarddirs.h> //locate file
#include <ktabbar.h>
#include <ktempfile.h>
#include <ktoolbarbutton.h> //ctor
#include <kurldrag.h>       //dragObject()
#include <kactioncollection.h>


MediaBrowser *MediaBrowser::s_instance = 0;

QPixmap *MediaItem::s_pixUnknown = 0;
QPixmap *MediaItem::s_pixArtist = 0;
QPixmap *MediaItem::s_pixAlbum = 0;
QPixmap *MediaItem::s_pixFile = 0;
QPixmap *MediaItem::s_pixTrack = 0;
QPixmap *MediaItem::s_pixPodcast = 0;
QPixmap *MediaItem::s_pixPlaylist = 0;
QPixmap *MediaItem::s_pixInvisible = 0;
QPixmap *MediaItem::s_pixStale = 0;
QPixmap *MediaItem::s_pixOrphaned = 0;
QPixmap *MediaItem::s_pixDirectory = 0;
QPixmap *MediaItem::s_pixRootItem = 0;

bool MediaBrowser::isAvailable() //static
{
    if( !MediaBrowser::instance() )
        return false;

    return MediaBrowser::instance()->m_haveDevices;
}

class SpaceLabel : public QLabel {
    public:
    SpaceLabel(QWidget *parent)
        : QLabel(parent)
    {
        m_total = m_used = m_scheduled = 0;
        setBackgroundMode(Qt::NoBackground);
    }

    void paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(e->rect(), colorGroup().brush(QColorGroup::Background));

        if(m_total > 0)
        {
            int used = int(float(m_used)/float(m_total)*width());
            int scheduled = int(float(m_used + m_scheduled)/float(m_total)*width());

            if(m_used > 0)
            {
                QColor blueish(70,120,255);
                if(e->rect().left() < used)
                {
                    int right = used;
                    if(e->rect().right() < right)
                        right = e->rect().right();
                    p.fillRect(e->rect().left(), e->rect().top(),
                            used, e->rect().bottom()+1, QBrush(blueish, Qt::SolidPattern));
                }
            }

            if(m_scheduled > 0)
            {
                QColor sched(70, 230, 120);
                if(m_used + m_scheduled > m_total - m_total/200)
                {
                    sched.setRgb( 255, 120, 120 );
                }
                int left = e->rect().left();
                if(used > left)
                    left = used;
                int right = e->rect().right();
                if(scheduled < right)
                    right = scheduled;
                p.fillRect(left, e->rect().top(), right, e->rect().bottom()+1, QBrush(sched, Qt::SolidPattern));
            }

            if(m_used + m_scheduled < m_total)
            {
                QColor grey(180, 180, 180);
                int left = e->rect().left();
                if(scheduled > left)
                    left = scheduled;
                int right = e->rect().right();
                p.fillRect(left, e->rect().top(), right, e->rect().bottom()+1, colorGroup().brush(QColorGroup::Background));
            }
        }
        QLabel::paintEvent(e);
    }

    KIO::filesize_t m_total;
    KIO::filesize_t m_used;
    KIO::filesize_t m_scheduled;
};

class DummyMediaDevice : public MediaDevice
{
    public:
    DummyMediaDevice() : MediaDevice()
    {
        m_name = i18n( "No Device Available" );
        m_type = "dummy-mediadevice";
        m_medium = Medium( "DummyDevice", "DummyDevice" );
    }
    void init( MediaBrowser *browser ) { MediaDevice::init( browser ); }
    virtual ~DummyMediaDevice() {}
    virtual bool isConnected() { return false; }
    virtual MediaItem* trackExists(const MetaBundle&) { return 0; }
    virtual bool lockDevice(bool) { return true; }
    virtual void unlockDevice() {}
    virtual bool openDevice( bool silent )
    {
        if( !silent )
        {
            //QString msg = i18n( "Sorry, you do not have a supported portable music player." );
            //amaroK::StatusBar::instance()->longMessage( msg, KDE::StatusBar::Sorry );
        }
        return false;
    }
    virtual bool closeDevice() { return false; }
    virtual void synchronizeDevice() {}
    virtual MediaItem* copyTrackToDevice(const MetaBundle&) { return 0; }
    virtual int deleteItemFromDevice(MediaItem*, bool) { return -1; }
};


MediaBrowser::MediaBrowser( const char *name )
        : QVBox( 0, name )
        , m_timer( new QTimer( this ) )
        , m_currentDevice( m_devices.end() )
        , m_quitting( false )
{
    s_instance = this;

    // preload pixmaps used in browser
    KIconLoader iconLoader;
    MediaItem::s_pixUnknown = new QPixmap(iconLoader.loadIcon( amaroK::icon( "unknown" ), KIcon::Toolbar, KIcon::SizeSmall ));
    MediaItem::s_pixTrack = new QPixmap(iconLoader.loadIcon( amaroK::icon( "playlist" ), KIcon::Toolbar, KIcon::SizeSmall ));
    MediaItem::s_pixFile = new QPixmap(iconLoader.loadIcon( amaroK::icon( "sound" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixPodcast = new QPixmap(iconLoader.loadIcon( amaroK::icon( "podcast" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixPlaylist = new QPixmap(iconLoader.loadIcon( amaroK::icon( "playlist" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixRootItem = new QPixmap(iconLoader.loadIcon( amaroK::icon( "files2" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    // history
    // favorites
    // collection
    // folder
    // folder_red
    // player_playlist_2
    // cancel
    // sound
    MediaItem::s_pixArtist = new QPixmap(iconLoader.loadIcon( amaroK::icon( "personal" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixAlbum = new QPixmap(iconLoader.loadIcon( amaroK::icon( "cdrom_unmount" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixInvisible = new QPixmap(iconLoader.loadIcon( amaroK::icon( "cancel" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixStale = new QPixmap(iconLoader.loadIcon( amaroK::icon( "cancel" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixOrphaned = new QPixmap(iconLoader.loadIcon( amaroK::icon( "cancel" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixDirectory = new QPixmap(iconLoader.loadIcon( amaroK::icon( "folder" ), KIcon::Toolbar, KIcon::SizeSmall ) );

    setSpacing( 4 );

    m_toolbar = new Browser::ToolBar( this );
    m_toolbar->setIconText( KToolBar::IconTextRight, false );

    m_toolbar->insertButton( "connect_creating", CONNECT, true, i18n("Connect") );
    QToolTip::add( m_toolbar->getButton(CONNECT), i18n( "Connect media device" ) );

    m_toolbar->insertButton( "player_eject", DISCONNECT, true, i18n("Disconnect") );
    QToolTip::add( m_toolbar->getButton(DISCONNECT), i18n( "Disconnect media device" ) );

    m_toolbar->insertButton( "rebuild", TRANSFER, true, i18n("Transfer") );
    QToolTip::add( m_toolbar->getButton(TRANSFER), i18n( "Transfer tracks to media device" ) );

    m_toolbar->insertLineSeparator();
    m_toolbar->setIconText( KToolBar::IconOnly, false );

    m_toolbar->insertButton( amaroK::icon( "configure" ), CONFIGURE, true, i18n("Configure") );
    QToolTip::add( m_toolbar->getButton(CONFIGURE), i18n( "Configure device" ) );

    m_deviceCombo = new KComboBox( this );

    // searching/filtering
    { //<Search LineEdit>
        KToolBar* searchToolBar = new Browser::ToolBar( this );
        KToolBarButton *button = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_searchEdit = new ClickLineEdit( i18n( "Enter search terms here" ), searchToolBar );

        searchToolBar->setStretchableWidget( m_searchEdit );
        m_searchEdit->setFrame( QFrame::Sunken );

        connect( button, SIGNAL( clicked() ), m_searchEdit, SLOT( clear() ) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to filter" ) );
    } //</Search LineEdit>

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_searchEdit, SIGNAL( returnPressed() ), SLOT( slotSetFilter() ) );

    // connect to device manager
    connect( DeviceManager::instance(), SIGNAL( mediumAdded(const Medium *, QString) ),
            SLOT( mediumAdded(const Medium *, QString) ) );
    connect( DeviceManager::instance(), SIGNAL( mediumChanged(const Medium *, QString) ),
            SLOT( mediumChanged(const Medium *, QString) ) );
    connect( DeviceManager::instance(), SIGNAL( mediumRemoved(const Medium *, QString) ),
            SLOT( mediumRemoved(const Medium *, QString) ) );


    // we always have a dummy device
    m_pluginName[ i18n( "Disable" ) ] = "dummy-mediadevice";
    m_pluginAmarokName["dummy-mediadevice"] = i18n( "Disable" );
    m_pluginName[ i18n( "Do not handle" ) ] = "ignore";
    m_pluginAmarokName["ignore"] = i18n( "Do not handle" );
    // query available device plugins
    m_plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'mediadevice'" );
    for( KTrader::OfferList::ConstIterator it = m_plugins.begin(); it != m_plugins.end(); ++it ) {
        // Save name properties in QMap for lookup
        m_pluginName[(*it)->name()] = (*it)->property( "X-KDE-Amarok-name" ).toString();
        m_pluginAmarokName[(*it)->property( "X-KDE-Amarok-name" ).toString()] = (*it)->name();
    }

    m_views = new QVBox( this );
    m_queue = new MediaQueue( this );
    m_progressBox  = new QHBox( this );
    m_progress     = new KProgress( m_progressBox );
    m_cancelButton = new KPushButton( SmallIconSet( amaroK::icon( "cancel" ) ), i18n("Cancel"), m_progressBox );


    m_stats = new SpaceLabel(this);

    m_progressBox->hide();

    MediaDevice *dev = new DummyMediaDevice();
    dev->init( this );
    addDevice( dev );
    activateDevice( 0, false );
    queue()->load( amaroK::saveLocation() + "transferlist.xml" );

    setFocusProxy( m_queue );

    updateStats();

    QMap<QString, Medium*> mmap = DeviceManager::instance()->getMediumMap();

    bool newflag = false;
    //This deals with <strike>auto-detectable</strike> ALL devices!
    for( QMap<QString, Medium*>::Iterator it = mmap.begin();
            it != mmap.end();
            it++ )
    {
        QString handler = amaroK::config( "MediaBrowser" )->readEntry( (*it)->id() );
        //debug() << "[MediaBrowser] (*it)->id() = " << (*it)->id() << ", handler = " << handler << endl;
        if( handler.isEmpty() )
        {
            //this should probably never be the case with a manually added device, unless amarokrc's been messed with
            amaroK::config( "MediaBrowser" )->writeEntry( (*it)->id(), "ignore" );
            newflag = true;
            mediumAdded( *it, (*it)->name(), true );
        }
        //and this definitely shouldn't!
        else if( handler != "deleted" )
            mediumAdded( *it, (*it)->name(), true );
    }

    if ( newflag )
        amaroK::StatusBar::instance()->longMessageThreadSafe(
                i18n("Amarok has detected new portable media devices.\n"
                    "Go to the \"Media Devices\" pane of the configuration\n"
                    "dialog to choose a plugin for these devices.") );

    connect( m_toolbar->getButton(CONNECT),    SIGNAL( clicked() ),        SLOT( connectClicked() ) );
    connect( m_toolbar->getButton(DISCONNECT), SIGNAL( clicked() ),        SLOT( disconnectClicked() ) );
    connect( m_toolbar->getButton(TRANSFER),   SIGNAL( clicked() ),        SLOT( transferClicked() ) );
    connect( m_toolbar->getButton(CONFIGURE),  SIGNAL( clicked() ),        SLOT( config() ) );

    connect( m_deviceCombo,      SIGNAL( activated( int ) ), SLOT( activateDevice( int ) ) );

    connect( m_cancelButton,     SIGNAL( clicked() ),        SLOT( cancelClicked() ) );
    connect( pApp,               SIGNAL( prepareToQuit() ),  SLOT( prepareToQuit() ) );
    connect( CollectionDB::instance(), SIGNAL( tagsChanged( const MetaBundle& ) ),
            SLOT( tagsChanged( const MetaBundle& ) ) );

    m_haveDevices = false;
    QMap<QString,QString> savedDevices = amaroK::config( "MediaBrowser" )->entryMap( "MediaBrowser" );
    for( QMap<QString,QString>::Iterator it = savedDevices.begin();
            it != savedDevices.end();
            ++it )
    {
        if( it.data() != "deleted" && it.data() != "ignore" )
        {
            m_haveDevices = true;
            break;
        }
    }
    emit availabilityChanged( m_haveDevices );
}

bool
MediaBrowser::blockQuit() const
{
    for( QValueList<MediaDevice *>::const_iterator it = m_devices.begin();
            it != m_devices.end();
            ++it )
    {
        if( *it && (*it)->isConnected() )
            return true;
    }

    return false;
}

void
MediaBrowser::tagsChanged( const MetaBundle &bundle )
{
    ItemMap::iterator it = m_itemMap.find( bundle.url().path() );
    if( it != m_itemMap.end() )
    {
        MediaItem *item = *it;
        if( item->device() )
        {
            item->device()->tagsChanged( item, bundle );
        }
        else
        {
            // it's an item on the transfer queue
            item->setBundle( new MetaBundle( bundle ) );

            QString text = item->bundle()->prettyTitle();
            if( item->type() == MediaItem::PODCASTITEM )
            {
                text += " (" + i18n("Podcast") + ")";
            }
            if( item->m_playlistName != QString::null )
            {
                text += " (" + item->m_playlistName + ")";
            }
            item->setText( 0, text);
        }
    }
}

const MetaBundle *
MediaBrowser::getBundle( const KURL &url ) const
{
    if( url.protocol() != "file" )
        return 0;

    ItemMap::const_iterator it = m_itemMap.find( url.path() );
    if( it == m_itemMap.end() )
        return 0;

    if( !(*it)->device() )
        return 0;

    return (*it)->bundle();
}

MediaDevice *
MediaBrowser::currentDevice()
{
    if( m_currentDevice != m_devices.end() )
    {
        return *m_currentDevice;
    }

    return 0;
}

MediaDevice *
MediaBrowser::deviceFromId( const QString &id )
{
    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
                it != m_devices.end();
                it++ )
        {
            if( (*it)->uniqueId() == id )
                return (*it);
        }

    return NULL;
}

void
MediaBrowser::activateDevice( const MediaDevice *dev )
{
    int index = 0;
    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        if( *it == dev )
        {
            activateDevice( index );
            break;
        }
        index++;
    }
}

void
MediaBrowser::activateDevice( int index, bool skipDummy )
{
    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        (*it)->view()->hide();
    }

    if( index < 0 )
    {
        m_currentDevice = m_devices.end();
        return;
    }

    if( skipDummy )
       index++;

    if( (uint)index >= m_devices.count() )
    {
        m_currentDevice = m_devices.end();
        return;
    }

    if( m_currentDevice == m_devices.at( index ) )
    {
        currentDevice()->view()->show();
        return;
    }

    m_currentDevice = m_devices.at( index );
    currentDevice()->view()->show();
    m_deviceCombo->setCurrentItem( index-1 );

    updateButtons();
    updateStats();
}

void
MediaBrowser::addDevice( MediaDevice *device )
{
    m_devices.append( device );

    device->loadConfig();

    if( device->autoConnect() )
    {
        device->connectDevice( true );
        updateButtons();
    }

    updateDevices();
}

void
MediaBrowser::removeDevice( MediaDevice *device )
{
    DEBUG_BLOCK

    debug() << "remove device: type=" << device->deviceType() << endl;

    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        if( *it == device )
        {
            bool current = (it == m_currentDevice);
            m_devices.remove( device );
            if( current )
                activateDevice( 0, false );
            break;
        }
    }

    if( device->isConnected() )
    {
        if( device->disconnectDevice( false /* don't run post-disconnect command */ ) )
            unloadDevicePlugin( device );
        else
        {
            debug() << "Cannot remove device because disconnect failed" << endl;
            amaroK::StatusBar::instance()->longMessage(
                    i18n( "Cannot remove device because disconnect failed" ),
                    KDE::StatusBar::Warning );
        }
    }
    else
        unloadDevicePlugin( device );

    updateDevices();
}

void
MediaBrowser::updateDevices()
{
    m_deviceCombo->clear();
    uint i = 0;
    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        if( m_devices.count() > 1 && dynamic_cast<DummyMediaDevice *>(*it) )
            continue;
        QString name = (*it)->name();
        if( !(*it)->deviceNode().isEmpty() )
        {
            name = i18n( "%1 at %2" ).arg( name, (*it)->deviceNode() );
        }
        if( !(*it)->mountPoint().isEmpty() )
        {
            name += i18n( " (mounted at %1)" ).arg( (*it)->mountPoint() );
        }
        m_deviceCombo->insertItem( name, i );
        if( it == m_currentDevice )
        {
            m_deviceCombo->setCurrentItem( i );
        }
        i++;
    }
    m_deviceCombo->setEnabled( m_devices.count() > 1 );
    m_haveDevices = m_devices.count() > 1;
    emit availabilityChanged( m_haveDevices );
}

QStringList
MediaBrowser::deviceNames()
{
    QStringList list;

    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        QString name = (*it)->name();
        list << name;
    }

    return list;
}

bool
MediaBrowser::deviceSwitch( const QString &name )
{
    int index = 0;
    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        if( (*it)->name() == name )
        {
            activateDevice( index, false );
            return true;
        }
        index++;
    }

    return false;
}

void
MediaBrowser::transcodingFinished( const QString &src, const QString &dst )
{
    KURL srcJob = KURL::fromPathOrURL( m_transcodeSrc );
    KURL srcResult = KURL::fromPathOrURL( src );

    if( srcJob.path() == srcResult.path() )
    {
        m_transcodedUrl = KURL::fromPathOrURL( dst );
        m_waitForTranscode = false;
    }
    else
    {
        debug() << "transcoding for " << src << " finished, "
            << "but we are waiting for " << m_transcodeSrc << endl;
    }
}

KURL
MediaBrowser::transcode( const KURL &src, const QString &filetype )
{
    const ScriptManager* const sm = ScriptManager::instance();

    if( sm->transcodeScriptRunning() == QString::null )
    {
        debug() << "cannot transcode with no transcoder registered" << endl;
        return KURL();
    }

    m_waitForTranscode = true;
    m_transcodeSrc = src.url();
    m_transcodedUrl = KURL();
    ScriptManager::instance()->notifyTranscode( src.url(), filetype );

    while( m_waitForTranscode && sm->transcodeScriptRunning() != QString::null )
    {
        usleep( 10000 );
        kapp->processEvents( 100 );
    }

    return m_transcodedUrl;
}


void
MediaBrowser::slotSetFilterTimeout() //SLOT
{
    m_timer->start( 280, true ); //stops the timer for us first
}

void
MediaBrowser::slotSetFilter() //SLOT
{
    m_timer->stop();

    if( currentDevice() )
        currentDevice()->view()->setFilter( m_searchEdit->text() );
}

void
MediaBrowser::prepareToQuit()
{
    m_waitForTranscode = false;
    m_quitting = true;
    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            ++it )
    {
        if( (*it)->isConnected() )
            (*it)->disconnectDevice( false /* don't unmount */ );
    }
}

MediaBrowser::~MediaBrowser()
{
    debug() << "having to remove " << m_devices.count() << " devices" << endl;
    while( !m_devices.isEmpty() )
    {
        removeDevice( m_devices.last() );
    }

    queue()->save( amaroK::saveLocation() + "transferlist.xml" );

    delete m_deviceCombo;
    delete m_queue;
}


MediaItem::MediaItem( QListView* parent )
: KListViewItem( parent )
{
    init();
}

MediaItem::MediaItem( QListViewItem* parent )
: KListViewItem( parent )
{
    init();
}

MediaItem::MediaItem( QListView* parent, QListViewItem* after )
: KListViewItem( parent, after )
{
    init();
}

MediaItem::MediaItem( QListViewItem* parent, QListViewItem* after )
: KListViewItem( parent, after )
{
    init();
}

MediaItem::~MediaItem()
{
    setBundle( 0 );
}

void
MediaItem::init()
{
    m_bundle=0;
    m_order=0;
    m_type=UNKNOWN;
    m_playlistName=QString::null;
    m_device=0;
    setExpandable( false );
    setDragEnabled( true );
    setDropEnabled( true );
}

void
MediaItem::setBundle( MetaBundle *bundle )
{
    if( m_bundle )
    {
        MediaBrowser::ItemMap::iterator it = MediaBrowser::instance()->m_itemMap.find( url().path() );
        if( it != MediaBrowser::instance()->m_itemMap.end() && *it == this )
            MediaBrowser::instance()->m_itemMap.remove(url().path());
    }
    delete m_bundle;
    m_bundle = bundle;

    if( m_bundle )
    {
        MediaBrowser::ItemMap::iterator it = MediaBrowser::instance()->m_itemMap.find( url().path() );
        if( it == MediaBrowser::instance()->m_itemMap.end() )
            MediaBrowser::instance()->m_itemMap[url().path()] = this;
    }
}

void MediaItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    switch( type() )
    {
    case INVISIBLE:
    case PODCASTSROOT:
    case PLAYLISTSROOT:
    case ORPHANEDROOT:
    case STALEROOT:
        {
            QFont font( p->font() );
            font.setBold( true );
            p->setFont( font );
        }
    default:
        break;
    }

    KListViewItem::paintCell( p, cg, column, width, align );
}

const MetaBundle *
MediaItem::bundle() const
{
    return m_bundle;
}

KURL
MediaItem::url() const
{
    if( bundle() )
        return bundle()->url();
    else
        return KURL();
}

bool
MediaItem::isFileBacked() const
{
    switch( type() )
    {
    case ARTIST:
    case ALBUM:
    case PODCASTSROOT:
    case PODCASTCHANNEL:
    case PLAYLISTSROOT:
    case PLAYLIST:
    case PLAYLISTITEM:
    case INVISIBLEROOT:
    case STALEROOT:
    case STALE:
    case ORPHANEDROOT:
        return false;

    case UNKNOWN:
    case TRACK:
    case ORPHANED:
    case INVISIBLE:
    case PODCASTITEM:
    case DIRECTORY:
        return true;
    }

    return false;
}

long
MediaItem::size() const
{
    if( !isFileBacked() )
        return 0;

    if( bundle() )
        return bundle()->filesize();

    return 0;
}

void
MediaItem::setType( Type type )
{
    m_type=type;

    setDragEnabled(true);
    setDropEnabled(false);

    switch(m_type)
    {
        case UNKNOWN:
            setPixmap(0, *s_pixUnknown);
            break;
        case INVISIBLE:
        case TRACK:
            setPixmap(0, *s_pixFile);
            break;
        case PLAYLISTITEM:
            setPixmap(0, *s_pixTrack);
            setDropEnabled(true);
            break;
        case ARTIST:
            setPixmap(0, *s_pixArtist);
            break;
        case ALBUM:
            setPixmap(0, *s_pixAlbum);
            break;
        case PODCASTSROOT:
            setPixmap(0, *s_pixRootItem);
            break;
        case PODCASTITEM:
        case PODCASTCHANNEL:
            setPixmap(0, *s_pixPodcast);
            break;
        case PLAYLIST:
            setPixmap(0, *s_pixPlaylist);
            setDropEnabled(true);
            break;
        case PLAYLISTSROOT:
            setPixmap(0, *s_pixRootItem);
            setDropEnabled( true );
            break;
        case INVISIBLEROOT:
            setPixmap(0, *s_pixInvisible);
            break;
        case STALEROOT:
        case STALE:
            setPixmap(0, *s_pixStale);
            break;
        case ORPHANEDROOT:
        case ORPHANED:
            setPixmap(0, *s_pixOrphaned);
            break;
        case DIRECTORY:
            setExpandable( true );
            setDropEnabled( true );
            setPixmap(0, *s_pixDirectory);
            break;
    }
}

MediaItem *
MediaItem::lastChild() const
{
    QListViewItem *last = 0;
    for( QListViewItem *it = firstChild();
            it;
            it = it->nextSibling() )
    {
        last = it;
    }

    return dynamic_cast<MediaItem *>(last);
}

bool
MediaItem::isLeafItem() const
{
    switch(type())
    {
        case UNKNOWN:
            return false;

        case INVISIBLE:
        case TRACK:
        case PODCASTITEM:
        case PLAYLISTITEM:
        case STALE:
        case ORPHANED:
            return true;

        case ARTIST:
        case ALBUM:
        case PODCASTSROOT:
        case PODCASTCHANNEL:
        case PLAYLISTSROOT:
        case PLAYLIST:
        case INVISIBLEROOT:
        case STALEROOT:
        case ORPHANEDROOT:
        case DIRECTORY:
            return false;
    }

    return false;
}

MediaItem *
MediaItem::findItem( const QString &key, const MediaItem *after ) const
{
    MediaItem *it = 0;
    if( after )
        it = dynamic_cast<MediaItem *>( after->nextSibling() );
    else
        it = dynamic_cast<MediaItem *>( firstChild() );

    for( ; it; it = dynamic_cast<MediaItem *>(it->nextSibling()))
    {
        if(key == it->text(0))
            return it;
        if(key.isEmpty() && it->text(0).isEmpty())
            return it;
    }
    return 0;
}

int
MediaItem::compare( QListViewItem *i, int col, bool ascending ) const
{
    MediaItem *item = dynamic_cast<MediaItem *>(i);
    if(item && col==0 && item->m_order != m_order)
        return m_order-item->m_order;

    return KListViewItem::compare(i, col, ascending);
}

class MediaItemTip : public QToolTip
{
    public:
    MediaItemTip( QListView *listview )
    : QToolTip( listview->viewport() )
    , m_view( listview )
    {}
    virtual ~MediaItemTip() {}
    protected:
    virtual void maybeTip( const QPoint &p )
    {
        MediaItem *i = dynamic_cast<MediaItem *>(m_view->itemAt( m_view->viewportToContents( p ) ) );
        if( !i )
            return;

        QString text;
        switch( i->type() )
        {
        case MediaItem::TRACK:
            {
                const MetaBundle *b = i->bundle();
                if( b )
                {
                    if( b->track() )
                        text = QString( "%1 - %2 (%3)" )
                            .arg( QString::number(b->track()), b->title(), b->prettyLength() );
                    if( !b->genre().isEmpty() )
                    {
                        if( !text.isEmpty() )
                            text += "<br>";
                        text += QString( "<i>Genre: %1</i>" )
                            .arg( b->genre() );
                    }
                }
            }
            break;
        case MediaItem::PLAYLISTSROOT:
            text = i18n( "Drag items here to create new playlist" );
            break;
        case MediaItem::PLAYLIST:
            text = i18n( "Drag items here to append to this playlist" );
            break;
        case MediaItem::PLAYLISTITEM:
            text = i18n( "Drag items here to insert before this item" );
            break;
        case MediaItem::INVISIBLEROOT:
        case MediaItem::INVISIBLE:
            text = i18n( "Not visible on media device" );
            break;
        case MediaItem::STALEROOT:
        case MediaItem::STALE:
            text = i18n( "In device database, but file is missing" );
            break;
        case MediaItem::ORPHANEDROOT:
        case MediaItem::ORPHANED:
            text = i18n( "File on device, but not in device database" );
            break;
        default:
            break;
        }

        if( !text.isEmpty() && !text.isNull() )
            tip( m_view->itemRect( i ), text );
    }

    QListView *m_view;
};


MediaView::MediaView( QWidget* parent, MediaDevice *device )
    : KListView( parent )
    , m_parent( parent )
    , m_device( device )
{
    hide();
    setSelectionMode( QListView::Extended );
    setItemsMovable( false );
    setShowSortIndicator( true );
    setFullWidth( true );
    setRootIsDecorated( true );
    setDragEnabled( true );
    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );

    header()->hide();
    addColumn( i18n( "Remote Media" ) );

    KActionCollection* ac = new KActionCollection( this );
    KStdAction::selectAll( this, SLOT( selectAll() ), ac, "mediabrowser_select_all" );

    connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );

    connect( this, SIGNAL( itemRenamed( QListViewItem* ) ),
             this,   SLOT( renameItem( QListViewItem* ) ) );

    connect( this, SIGNAL( expanded( QListViewItem* ) ),
             this,   SLOT( slotExpand( QListViewItem* ) ) );

    connect( this, SIGNAL( returnPressed( QListViewItem* ) ),
             this,   SLOT( invokeItem( QListViewItem* ) ) );

    connect( this, SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( invokeItem( QListViewItem* ) ) );

    m_toolTip = new MediaItemTip( this );
}

void
MediaView::invokeItem( QListViewItem *i )
{
    MediaItem *item = dynamic_cast<MediaItem *>( i );
    if( !item )
        return;

    KURL::List urls = nodeBuildDragList( item );
    Playlist::instance()->insertMedia( urls, Playlist::Append );
}

void
MediaView::renameItem( QListViewItem *item )
{
    m_device->renameItem( item );
}

void
MediaView::slotExpand( QListViewItem *item )
{
    m_device->expandItem( item );
}


MediaView::~MediaView()
{
    delete m_toolTip;
}


void
MediaView::startDrag()
{
    KURL::List urls = nodeBuildDragList( 0 );
    KURLDrag* d = new KURLDrag( urls, this );
    d->setPixmap( CollectionDB::createDragPixmap( urls ),
                  QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    d->dragCopy();
}


KURL::List
MediaView::nodeBuildDragList( MediaItem* item, bool onlySelected )
{
    KURL::List items;
    MediaItem* fi;

    if ( !item )
    {
        fi = static_cast<MediaItem*>(firstChild());
    }
    else
        fi = item;

    while ( fi )
    {
        if( fi->isVisible() )
        {
            if ( fi->isSelected() || !onlySelected )
            {
                if( fi->isLeafItem() || fi->type() == MediaItem::DIRECTORY )
                    items += fi->url();
                else
                {
                    if(fi->childCount() )
                        items += nodeBuildDragList( static_cast<MediaItem*>(fi->firstChild()), false );
                }
            }
            else
            {
                if ( fi->childCount() )
                    items += nodeBuildDragList( static_cast<MediaItem*>(fi->firstChild()), true );
            }
        }
        fi = static_cast<MediaItem*>(fi->nextSibling());
    }
    return items;
}

int
MediaView::getSelectedLeaves( MediaItem *parent, QPtrList<MediaItem> *list, bool onlySelected, bool onlyPlayed )
{
    int numFiles = 0;
    if( !list )
        list = new QPtrList<MediaItem>;

    MediaItem *it;
    if( !parent )
        it = dynamic_cast<MediaItem *>(firstChild());
    else
        it = dynamic_cast<MediaItem *>(parent->firstChild());

    for( ; it; it = dynamic_cast<MediaItem*>(it->nextSibling()))
    {
        if( it->isVisible() )
        {
            if( it->childCount() && !( it->type() == MediaItem::DIRECTORY && it->isSelected() ) )
            {
                numFiles += getSelectedLeaves(it, list, onlySelected && !it->isSelected(), onlyPlayed);
            }
            if( it->isSelected() || !onlySelected )
            {
                if( it->type() == MediaItem::TRACK       ||
                    it->type() == MediaItem::DIRECTORY   ||
                    it->type() == MediaItem::PODCASTITEM ||
                    it->type() == MediaItem::INVISIBLE   ||
                    it->type() == MediaItem::ORPHANED     )
                {
                    if( onlyPlayed )
                    {
                        if( it->played() > 0 )
                            numFiles++;
                    }
                    else
                        numFiles++;
                }
                if( ( it->isLeafItem() && (!onlyPlayed || it->played()>0) )
                        || it->type() == MediaItem::DIRECTORY )
                    list->append( it );
            }
        }
    }
    return numFiles;
}


void
MediaView::contentsDragEnterEvent( QDragEnterEvent *e )
{
    QString data;
    QCString subtype;
    QTextDrag::decode( e, data, subtype );

    e->accept( e->source() == viewport()
            || subtype == "amarok-sql"
            || KURLDrag::canDecode( e ) );
}


void
MediaView::contentsDropEvent( QDropEvent *e )
{
    if(e->source() == viewport() || e->source() == this)
    {
        const QPoint p = contentsToViewport( e->pos() );
        MediaItem *item = dynamic_cast<MediaItem *>(itemAt( p ));

        if( !item )
            return;

        QPtrList<MediaItem> items;

        if( item->type() == MediaItem::PLAYLIST )
        {
            MediaItem *list = item;
            MediaItem *after = 0;
            for(MediaItem *it = dynamic_cast<MediaItem *>(item->firstChild());
                    it;
                    it = dynamic_cast<MediaItem *>(it->nextSibling()))
                after = it;

            getSelectedLeaves( 0, &items );
            m_device->addToPlaylist( list, after, items );
        }
        else if( item->type() == MediaItem::PLAYLISTITEM )
        {
            MediaItem *list = dynamic_cast<MediaItem *>(item->parent());
            MediaItem *after = 0;
            for(MediaItem *it = dynamic_cast<MediaItem*>(item->parent()->firstChild());
                    it;
                    it = dynamic_cast<MediaItem *>(it->nextSibling()))
            {
                if(it == item)
                    break;
                after = it;
            }

            getSelectedLeaves( 0, &items );
            m_device->addToPlaylist( list, after, items );
        }
        else if( item->type() == MediaItem::PLAYLISTSROOT )
        {
            QPtrList<MediaItem> items;
            getSelectedLeaves( 0, &items );
            QString base( i18n("New Playlist") );
            QString name = base;
            int i=1;
            while( item->findItem(name) )
            {
                QString num;
                num.setNum(i);
                name = base + " " + num;
                i++;
            }
            MediaItem *pl = m_device->newPlaylist(name, item, items);
            ensureItemVisible(pl);
            rename(pl, 0);
        }
        else if( item->type() == MediaItem::DIRECTORY ||
                    item->type() == MediaItem::TRACK )
        {
            QPtrList<MediaItem> items;
            getSelectedLeaves( 0, &items );
            m_device->addToDirectory( item, items );
        }
    }
    else
    {
        QString data;
        QCString subtype;
        QTextDrag::decode( e, data, subtype );
        KURL::List list;

        if( subtype == "amarok-sql" )
        {
            QStringList values = CollectionDB::instance()->query( data );
            for( QStringList::iterator it = values.begin();
                    it != values.end();
                    it++ )
            {
                it += QueryBuilder::dragFieldCount-2;
                list += KURL::fromPathOrURL( *it );
                it++;
            }
            MediaBrowser::queue()->addURLs( list );
        }
        else if ( KURLDrag::decode( e, list ) )
        {
            MediaBrowser::queue()->addURLs( list );
        }
    }
}


void
MediaView::contentsDragMoveEvent( QDragMoveEvent *e )
{
//    const QPoint p = contentsToViewport( e->pos() );
//    QListViewItem *item = itemAt( p );
    e->accept( e->source() == viewport()
            || e->source() == this
            || (e->source() != m_parent
                && e->source() != MediaBrowser::instance()->m_queue
                && e->source() != MediaBrowser::instance()->m_queue->viewport()
                && KURLDrag::canDecode( e )) );

}


void
MediaView::viewportPaintEvent( QPaintEvent *e )
{
    KListView::viewportPaintEvent( e );

    // Superimpose bubble help:

    if ( !MediaBrowser::instance()->currentDevice() || !MediaBrowser::instance()->currentDevice()->isConnected() )
    {
        QPainter p( viewport() );

        QSimpleRichText t( i18n(
                "<div align=center>"
                  "<h3>Media Device Browser</h3>"
                  "Configure your media device and then "
                  "click the Connect button to access your media device. "
                  "Drag and drop files to enqueue them for transfer."
                "</div>" ), QApplication::font() );

        t.setWidth( width() - 50 );

        const uint w = t.width() + 20;
        const uint h = t.height() + 20;

        p.setBrush( colorGroup().background() );
        p.drawRoundRect( 15, 15, w, h, (8*200)/w, (8*200)/h );
        t.draw( &p, 20, 20, QRect(), colorGroup() );
    }
}

void
MediaView::rmbPressed( QListViewItem *item, const QPoint &p, int i )
{
    if( item )
        m_device->rmbPressed( item, p, i );
    else
        debug() << "tried to call rmbPressed with invalid item" << endl;
}

MediaItem *
MediaView::newDirectory( MediaItem *parent )
{
    bool ok;
    const QString name = KInputDialog::getText(i18n("Add Directory"), i18n("Directory Name:"), QString::null, &ok, this);

    if( ok && !name.isEmpty() )
    {
        m_device->newDirectory( name, parent );
    }

    return 0;
}

void
MediaBrowser::mediumAdded( const Medium *medium, QString /*name*/, bool constructing )
{
    debug() << "mediumAdded: " << (medium? medium->properties():"null") << endl;
    if( medium )
    {
        KConfig *config = amaroK::config( "MediaBrowser" );
        QString handler = config->readEntry( medium->id() );
        if( handler.isEmpty() )
        {
            if( !constructing && medium->isAutodetected() )
            {
                MediumPluginManagerDialog *mpm = new MediumPluginManagerDialog();
                mpm->exec();
            }
        }
        //debug() << "id=" << medium->id() << ", handler=" << handler << endl;
        MediaDevice *device = loadDevicePlugin( handler );
        if( device )
        {
            device->m_medium = *medium;
            addDevice( device );
            if( m_currentDevice == m_devices.begin() || m_currentDevice == m_devices.end() )
                activateDevice( m_devices.count()-1, false );
        }
    }
}

void
MediaBrowser::pluginSelected( const Medium *medium, const QString plugin )
{
    DEBUG_BLOCK
    if( !plugin.isEmpty() )
    {
        debug() << "Medium id is " << medium->id() << " and plugin selected is: " << plugin << endl;
        amaroK::config( "MediaBrowser" )->writeEntry( medium->id(), plugin );

        bool success = true;
        for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
                it != m_devices.end();
                it++ )
        {
            if( (*it)->uniqueId() == medium->id() )
            {
                debug() << "removing " << medium->deviceNode() << endl;
                if( (*it)->isConnected() )
                {
                    if( (*it)->disconnectDevice( false ) )
                        removeDevice( *it );
                    else
                        success = false;
                }
                else
                    removeDevice( *it );
                break;
            }
        }

        if( success )
        {
            mediumAdded( medium, "doesntmatter", false );
        }
        else
        {
            debug() << "Cannot change plugin while operation is in progress" << endl;
            amaroK::StatusBar::instance()->longMessage(
                    i18n( "Cannot change plugin while operation is in progress" ),
                    KDE::StatusBar::Warning );
        }
    }
    else
        debug() << "Medium id is " << medium->id() << " and you opted not to use a plugin" << endl;
}

void
MediaBrowser::showPluginManager()
{
    MediumPluginManagerDialog* mpm = new MediumPluginManagerDialog();
    mpm->exec();
    delete mpm;
}

void
MediaBrowser::mediumChanged( const Medium *medium, QString /*name*/ )
{
    if( medium )
    {
        for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
                it != m_devices.end();
                it++ )
        {
            if( (*it)->uniqueId() == medium->id() )
            {
                (*it)->m_medium = const_cast<Medium *>(medium);
#if 0
                if( (*it)->isConnected() && !medium->isMounted() )
                {
                    amaroK::StatusBar::instance()->longMessage(
                            i18n( "The device %1 was unmounted before it was synchronized. "
                                "In order to avoid data loss, press the \"Disconnect\" button "
                                "before unmounting the device." ).arg( name ),
                            KDE::StatusBar::Warning );
                    //(*it)->disconnectDevice();
                }
#endif
                break;
            }
        }
    }
}

void
MediaBrowser::mediumRemoved( const Medium *medium, QString name )
{
    if( medium )
    {
        for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
                it != m_devices.end();
                it++ )
        {
            if( (*it)->uniqueId() == medium->id() )
            {
                if( (*it)->isConnected() )
                {
                    if( (*it)->disconnectDevice() )
                        removeDevice( *it );
                    amaroK::StatusBar::instance()->longMessage(
                            i18n( "The device %1 was removed before it was disconnected. "
                                "In order to avoid possible data loss, press the \"Disconnect\""
                                "button before disconnecting the device." ).arg( name ),
                            KDE::StatusBar::Warning );
                }
                else
                    removeDevice( *it );
                break;
            }
        }
    }
}

MediaDevice *
MediaBrowser::loadDevicePlugin( const QString &deviceType )
{
    DEBUG_BLOCK

    if( deviceType == "ignore" )
        return 0;

    QString query = "[X-KDE-Amarok-plugintype] == 'mediadevice' and [X-KDE-Amarok-name] == '%1'";
    amaroK::Plugin *plugin = PluginManager::createFromQuery( query.arg( deviceType ) );

    if( plugin )
    {
        debug() << "Returning plugin!" << endl;
        MediaDevice *device = static_cast<MediaDevice *>( plugin );
        device->init( this );
        device->m_type = deviceType;
        return device;
    }

    debug() << "no plugin for " << deviceType << endl;
    return 0;
}

void
MediaBrowser::unloadDevicePlugin( MediaDevice *device )
{
    DEBUG_BLOCK

    if( !device )
        return;

    disconnect( device ); // disconnect all signals

    if( dynamic_cast<DummyMediaDevice *>(device) )
    {
        delete device;
    }
    else
    {
        PluginManager::unload( device );
    }
}

bool
MediaBrowser::config()
{
    if( m_deviceCombo->currentText() == "No Device Selected" )
    {
        showPluginManager();
        return true;
    }

    DeviceConfigureDialog* dcd = new DeviceConfigureDialog( currentDevice()->m_medium );
    dcd->exec();
    bool successful = dcd->successful();
    delete dcd;
    return successful;
}

void
MediaBrowser::configSelectPlugin( int index )
{
    Q_UNUSED( index );

    if( m_currentDevice == m_devices.begin() )
    {
        AmarokConfig::setDeviceType( m_pluginName[m_configPluginCombo->currentText()] );
    }
    else if( currentDevice() )
    {
        KConfig *config = amaroK::config( "MediaBrowser" );
        config->writeEntry( currentDevice()->uniqueId(), m_pluginName[m_configPluginCombo->currentText()] );
    }

    if( !currentDevice() )
        activateDevice( 0, false );

    if( !currentDevice() )
        return;

    if( m_pluginName[m_configPluginCombo->currentText()] != currentDevice()->deviceType() )
    {
        MediaDevice *dev = currentDevice();
        dev->removeConfigElements( m_configBox );
        if( dev->isConnected() )
        {
            dev->disconnectDevice( false );
        }
        unloadDevicePlugin( dev );
        *m_currentDevice = loadDevicePlugin( AmarokConfig::deviceType() );
        if( !*m_currentDevice )
        {
            *m_currentDevice = new DummyMediaDevice();
            if( AmarokConfig::deviceType() != "dummy-mediadevice" )
            {
                QString msg = i18n( "The requested media device could not be loaded" );
                amaroK::StatusBar::instance()->shortMessage( msg );
            }
        }
        dev = currentDevice();
        dev->init( this );
        dev->loadConfig();

        m_configBox->hide();
        dev->addConfigElements( m_configBox );
        m_configBox->show();

        dev->view()->show();

        if( dev->autoConnect() )
        {
            dev->connectDevice( true );
            updateButtons();
        }

        updateDevices();
    }
}

void
MediaBrowser::updateButtons()
{
    if( !m_toolbar->getButton(CONNECT) ||
            !m_toolbar->getButton(DISCONNECT) ||
            !m_toolbar->getButton(TRANSFER) )
        return;

    if( currentDevice() )
    {
        m_toolbar->getButton(CONNECT)->setEnabled( !currentDevice()->isConnected() );
        m_toolbar->getButton(DISCONNECT)->setEnabled( currentDevice()->isConnected() );
        m_toolbar->getButton(TRANSFER)->setEnabled( currentDevice()->isConnected() && m_queue->childCount() > 0 );
    }
    else
    {
        m_toolbar->getButton(CONNECT)->setEnabled( false );
        m_toolbar->getButton(DISCONNECT)->setEnabled( false );
        m_toolbar->getButton(TRANSFER)->setEnabled( false );
    }
}

void
MediaBrowser::updateStats()
{
    if( !m_stats )
        return;

    QString text = i18n( "1 track in queue", "%n tracks in queue", m_queue->childCount() );
    if(m_queue->childCount() > 0)
    {
        text += i18n(" (%1)").arg( KIO::convertSize( m_queue->totalSize() ) );
    }

    KIO::filesize_t total, avail;
    if( currentDevice() && currentDevice()->getCapacity(&total, &avail) )
    {
        text += i18n( " - %1 of %2 available" ).arg( KIO::convertSize( avail ) ).arg( KIO::convertSize( total ) );

        m_stats->m_used = total-avail;
        m_stats->m_total = total;
        m_stats->m_scheduled = m_queue->totalSize();
    }
    else
    {
        m_stats->m_used = 0;
        m_stats->m_total = 0;
        m_stats->m_scheduled = m_queue->totalSize();
    }

    m_stats->setText(text);
    QToolTip::add( m_stats, text );
}


bool
MediaView::setFilter( const QString &filter, MediaItem *parent )
{
    bool advanced = ExpressionParser::isAdvancedExpression( filter );
    QValueList<int> defaultColumns;
    defaultColumns << MetaBundle::Album;
    defaultColumns << MetaBundle::Title;
    defaultColumns << MetaBundle::Artist;

    MediaItem *it;
    if( !parent )
    {
        it = dynamic_cast<MediaItem *>(firstChild());
    }
    else
    {
        it = dynamic_cast<MediaItem *>(parent->firstChild());
    }

    bool childrenVisible = false;
    for( ; it; it = dynamic_cast<MediaItem *>(it->nextSibling()))
    {
        bool visible = true;
        if(it->isLeafItem())
        {
            if( advanced )
            {
                ParsedExpression parsed = ExpressionParser::parse( filter );
                visible = it->bundle() && it->bundle()->matchesParsedExpression( parsed, defaultColumns );
            }
            else
            {
                visible = it->bundle() && it->bundle()->matchesSimpleExpression( filter, defaultColumns );
            }
        }
        else
        {
            visible = setFilter(filter, it);
            if(it->type()==MediaItem::PLAYLISTSROOT || it->type()==MediaItem::PLAYLIST)
            {
                visible = true;
            }
        }
        it->setVisible( visible );
        if(visible)
            childrenVisible = true;
    }

    return childrenVisible;
}

MediaDevice::MediaDevice()
    : amaroK::Plugin()
    , m_hasMountPoint( true )
    , m_autoDeletePodcasts( false )
    , m_syncStats( false )
    , m_transcode( false )
    , m_transcodeAlways( false )
    , m_transcodeRemove( false )
    , m_parent( NULL )
    , m_view( NULL )
    , m_transferDir( QString::null )
    , m_firstSort( QString::null )
    , m_secondSort( QString::null )
    , m_thirdSort( QString::null )
    , m_wait( false )
    , m_requireMount( false )
    , m_cancelled( false )
    , m_transferring( false )
    , m_deleting( false )
    , m_deferredDisconnect( false )
    , m_transferredItem( 0 )
    , m_playlistItem( 0 )
    , m_podcastItem( 0 )
    , m_invisibleItem( 0 )
    , m_staleItem( 0 )
    , m_orphanedItem( 0 )
{
    sysProc = new KShellProcess(); Q_CHECK_PTR(sysProc);
}

void MediaDevice::init( MediaBrowser* parent )
{
    m_parent = parent;
    if( !m_view )
        m_view = new MediaView( m_parent->m_views, this );
    m_view->hide();
}

MediaDevice::~MediaDevice()
{
    delete m_view;
    delete sysProc;
}

bool
MediaDevice::isSpecialItem( MediaItem *item )
{
    return (item == m_playlistItem) ||
        (item == m_podcastItem) ||
        (item == m_invisibleItem) ||
        (item == m_staleItem) ||
        (item == m_orphanedItem);
}

void
MediaDevice::loadConfig()
{
    m_transcode = configBool( "Transcode" );
    m_transcodeAlways = configBool( "TranscodeAlways" );
    m_transcodeRemove = configBool( "TranscodeRemove" );
    m_preconnectcmd = configString( "PreConnectCommand" );
    if( m_preconnectcmd.isEmpty() )
        m_preconnectcmd = configString( "MountCommand" );
    m_postdisconnectcmd = configString( "PostDisconnectCommand" );
    if( m_postdisconnectcmd.isEmpty() )
        m_postdisconnectcmd = configString( "UmountCommand" );
}

QString
MediaDevice::configString( const QString &name, const QString &defValue )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += "_" + uniqueId();
    KConfig *config = amaroK::config( configName );
    return config->readEntry( name, defValue );
}

void
MediaDevice::setConfigString( const QString &name, const QString &value )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += "_" + uniqueId();
    KConfig *config = amaroK::config( configName );
    config->writeEntry( name, value );
}

bool
MediaDevice::configBool( const QString &name, bool defValue )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += "_" + uniqueId();
    KConfig *config = amaroK::config( configName );
    return config->readBoolEntry( name, defValue );
}

void
MediaDevice::setConfigBool( const QString &name, bool value )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += "_" + uniqueId();
    KConfig *config = amaroK::config( configName );
    config->writeEntry( name, value );
}

MediaView *
MediaDevice::view()
{
    return m_view;
}

void
MediaDevice::hideProgress()
{
    m_parent->m_progressBox->hide();
}

void
MediaDevice::updateRootItems()
{
    if(m_podcastItem)
        m_podcastItem->setVisible(m_podcastItem->childCount() > 0);
    if(m_invisibleItem)
        m_invisibleItem->setVisible(m_invisibleItem->childCount() > 0);
    if(m_staleItem)
        m_staleItem->setVisible(m_staleItem->childCount() > 0);
    if(m_orphanedItem)
        m_orphanedItem->setVisible(m_orphanedItem->childCount() > 0);
}

void
MediaQueue::addURL( const KURL& url, MetaBundle *bundle, const QString &playlistName )
{
    if( PlaylistFile::isPlaylistFile( url ) )
    {
        QString name = url.path().section( "/", -1 ).section( ".", 0, -2 ).replace( "_", " " );
        PlaylistFile playlist( url.path() );

        if( playlist.isError() )
        {
            amaroK::StatusBar::instance()->longMessage( i18n( "Failed to load playlist: %1" ).arg( url.path() ),
                    KDE::StatusBar::Sorry );
            return;
        }

        for( BundleList::iterator it = playlist.bundles().begin();
                it != playlist.bundles().end();
                ++it )
        {
            addURL( (*it).url(), 0, name );
        }
        return;
    }
    else if( ContextBrowser::hasContextProtocol( url ) )
    {
        KURL::List urls = ContextBrowser::expandURL( url );

        for( KURL::List::iterator it = urls.begin();
                it != urls.end();
                ++it )
        {
            addURL( *it );
        }
        return;
    }
    else if( url.protocol() == "file" && QFileInfo( url.path() ).isDir() )
    {
        KURL::List urls = amaroK::recursiveUrlExpand( url );
        foreachType( KURL::List, urls )
            addURL( *it );
        return;
    }

    if(!bundle)
        bundle = new MetaBundle( url );

    MediaItem* item = new MediaItem( this, lastItem() );
    item->setExpandable( false );
    item->setDropEnabled( true );
    item->setBundle( bundle );
    if(bundle->podcastBundle() )
    {
        item->setType( MediaItem::PODCASTITEM );
    }
    item->m_playlistName = playlistName;

    QString text = item->bundle()->prettyTitle();
    if( text.isEmpty() || (!item->bundle()->isValidMedia() && !item->bundle()->podcastBundle()) )
        text = item->bundle()->url().prettyURL();
    if( item->m_playlistName != QString::null )
    {
        text += " (" + item->m_playlistName + ")";
    }
    item->setText( 0, text);

    m_parent->updateButtons();
    m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() + 1 );
    itemCountChanged();
}

void
MediaQueue::addURLs( const KURL::List urls, const QString &playlistName )
{
    KURL::List::ConstIterator it = urls.begin();
    for ( ; it != urls.end(); ++it )
        addURL( *it, 0, playlistName );

    URLsAdded();
}

void
MediaQueue::URLsAdded()
{
    m_parent->updateStats();
    if( m_parent->currentDevice()
            && m_parent->currentDevice()->isConnected()
            && m_parent->currentDevice()->asynchronousTransfer()
            && !m_parent->currentDevice()->isTransferring() )
        m_parent->currentDevice()->transferFiles();
}

void
MediaQueue::startDrag()
{
    KURL::List urls;

    for( QListViewItem *it = firstChild(); it; it = it->nextSibling() )
    {
        if( it->isVisible() && it->isSelected() && dynamic_cast<MediaItem *>(it) )
            urls += static_cast<MediaItem *>(it)->url();
    }

    KURLDrag* d = new KURLDrag( urls, this );
    d->setPixmap( CollectionDB::createDragPixmap( urls ),
                  QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    d->dragCopy();
}

QString
MediaDevice::replaceVariables( const QString &cmd )
{
    QString result = cmd;
    result.replace( "%d", deviceNode() );
    result.replace( "%m", mountPoint() );
    return result;
}

int MediaDevice::runPreConnectCommand()
{
    if( m_preconnectcmd.isEmpty() )
        return 0;

    QString cmd = replaceVariables( m_preconnectcmd );

    debug() << "running pre-connect command: [" << cmd << "]" << endl;
    int e=sysCall(cmd);
    debug() << "pre-connect: e=" << e << endl;
    return e;
}

int MediaDevice::runPostDisconnectCommand()
{
    if( m_postdisconnectcmd.isEmpty() )
        return 0;

    QString cmd = replaceVariables( m_postdisconnectcmd );
    debug() << "running post-disconnect command: [" << cmd << "]" << endl;
    int e=sysCall(cmd);
    debug() << "post-disconnect: e=" << e << endl;

    return e;
}

int MediaDevice::sysCall( const QString &command )
{
    if ( sysProc->isRunning() )  return -1;

    sysProc->clearArguments();
    (*sysProc) << command;
    if (!sysProc->start( KProcess::Block, KProcess::AllOutput ))
        kdFatal() << i18n("could not execute %1").arg(command.local8Bit().data()) << endl;

    return (sysProc->exitStatus());
}

void
MediaDevice::abortTransfer()
{
    setCancelled( true );
    cancelTransfer();
}

bool
MediaDevice::kioCopyTrack( const KURL &src, const KURL &dst )
{
    m_wait = true;

    KIO::FileCopyJob *job = KIO::file_copy( src, dst,
            -1 /* permissions */,
            false /* overwrite */,
            false /* resume */,
            false /* show progress */ );
    connect( job, SIGNAL( result( KIO::Job * ) ),
            this,  SLOT( fileTransferred( KIO::Job * ) ) );

    bool tryToRemove = false;
    while ( m_wait )
    {
        if( isCancelled() )
        {
            job->kill( false /* still emit result */ );
            tryToRemove = true;
            m_wait = false;
        }
        else
        {
            usleep(10000);
            kapp->processEvents( 100 );
        }
    }

    if( !tryToRemove )
    {
        if(m_copyFailed)
        {
            tryToRemove = true;
            amaroK::StatusBar::instance()->longMessage(
                    i18n( "Media Device: Copying %1 to %2 failed" )
                    .arg( src.prettyURL(), dst.prettyURL() ),
                    KDE::StatusBar::Error );
        }
        else
        {
            MetaBundle bundle2(dst);
            if(!bundle2.isValidMedia())
            {
                tryToRemove = true;
                // probably s.th. went wrong
                amaroK::StatusBar::instance()->longMessage(
                        i18n( "Media Device: Reading tags from %1 failed" ).arg( dst.prettyURL() ),
                        KDE::StatusBar::Error );
            }
        }
    }

    if( tryToRemove )
    {
        QFile::remove( dst.path() );
        return false;
    }

    return true;
}

void
MediaDevice::fileTransferred( KIO::Job *job )  //SLOT
{
    if(job->error())
    {
        m_copyFailed = true;
        debug() << "file transfer failed: " << job->errorText() << endl;
    }
    else
    {
        m_copyFailed = false;

        // the track just transferred has not yet been removed from the queue
        if( !isCancelled() )
            MediaBrowser::instance()->queue()->takeItem( MediaBrowser::instance()->queue()->firstChild() );
        m_parent->updateStats();
    }

    m_wait = false;
}

void
MediaBrowser::cancelClicked()
{
    DEBUG_BLOCK

    m_waitForTranscode = false;
    if( currentDevice() )
        currentDevice()->abortTransfer();
}

void
MediaBrowser::transferClicked()
{
    m_toolbar->getButton(TRANSFER)->setEnabled( false );
    if( currentDevice()
            && currentDevice()->isConnected()
            && !currentDevice()->isTransferring() )
    {
        if( !currentDevice()->hasTransferDialog() )
            currentDevice()->transferFiles();
        else
        {
            currentDevice()->runTransferDialog();
            //may not work with non-TransferDialog-class object, but maybe some run time introspection could solve it?
            if( currentDevice()->getTransferDialog() &&
              ( reinterpret_cast<TransferDialog *>(currentDevice()->getTransferDialog()))->isAccepted() )
                currentDevice()->transferFiles();
            else
                updateButtons();
        }
    }
    currentDevice()->m_transferDir = currentDevice()->m_medium.mountPoint();
}

void
MediaBrowser::connectClicked()
{
    bool haveToConfig = false;
    // it was just clicked, so isOn() == true.
    if( currentDevice() && !currentDevice()->isConnected() )
    {
        haveToConfig = !currentDevice()->connectDevice();
    }

    haveToConfig |= !currentDevice();
    haveToConfig |= ( currentDevice() && !currentDevice()->isConnected() );

    if ( !currentDevice()->needsManualConfig() )
        haveToConfig = false;

    if( haveToConfig && *m_devices.at( 0 ) == currentDevice() )
    {
        if( config() && currentDevice() && !currentDevice()->isConnected() )
            currentDevice()->connectDevice();
    }

    updateDevices();
    updateButtons();
    updateStats();
}


void
MediaBrowser::disconnectClicked()
{
    m_toolbar->getButton(TRANSFER)->setEnabled( false );
    m_toolbar->getButton(DISCONNECT)->setEnabled( false );

    if( currentDevice() )
    {
        currentDevice()->disconnectDevice( true );
    }

    updateDevices();
    updateButtons();
    updateStats();
}

bool
MediaDevice::connectDevice( bool silent )
{
    if( !lockDevice( true ) )
        return false;

    runPreConnectCommand();
    openDevice( silent );

    if( isConnected()
            && MediaBrowser::instance()->currentDevice() != this
            && !MediaBrowser::instance()->currentDevice()->isConnected() )
    {
        MediaBrowser::instance()->activateDevice( this );
    }
    m_parent->updateStats();
    m_parent->updateButtons();

    if( !isConnected() )
    {
        unlockDevice();
        return false;
    }

    if( m_syncStats )
    {
        syncStatsFromDevice( 0 );
        Scrobbler::instance()->m_submitter->syncComplete();
    }

    // delete podcasts already played
    if( m_autoDeletePodcasts && m_podcastItem )
    {
        QPtrList<MediaItem> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_view->getSelectedLeaves( m_podcastItem, &list, false /* not only selected */, true /* only played */ );

        if(numFiles > 0)
        {
            m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", numFiles ) );

            setProgress( 0, numFiles );

            int numDeleted = deleteItemFromDevice( m_podcastItem, true );
            purgeEmptyItems();
            if( numDeleted < 0 )
            {
                amaroK::StatusBar::instance()->longMessage(
                        i18n( "Failed to purge podcasts already played" ),
                        KDE::StatusBar::Sorry );
            }
            else if( numDeleted > 0 )
            {
                amaroK::StatusBar::instance()->shortMessage(
                        i18n( "Purged 1 podcasts already played",
                            "Purged %n podcasts already played",
                            numDeleted ) );
            }

            synchronizeDevice();

            QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
            m_parent->updateStats();
        }
    }
    unlockDevice();

    updateRootItems();

    if( m_deferredDisconnect )
    {
        m_deferredDisconnect = false;
        disconnectDevice( m_runDisconnectHook );
    }

    amaroK::StatusBar::instance()->shortMessage( i18n( "Device successfully connected" ) );

    return true;
}

bool
MediaDevice::disconnectDevice( bool postDisconnectHook )
{
    DEBUG_BLOCK

    abortTransfer();

    debug() << "disconnecting: hook=" << postDisconnectHook << endl;

    if( !lockDevice( true ) )
    {
        m_runDisconnectHook = postDisconnectHook;
        m_deferredDisconnect = true;
        debug() << "disconnecting: locked" << endl;
        return false;
    }
    debug() << "disconnecting: ok" << endl;

    if( m_syncStats )
    {
        syncStatsToDevice();
    }

    closeDevice();
    unlockDevice();

    m_parent->updateStats();

    bool result = true;
    if( postDisconnectHook && runPostDisconnectCommand() != 0 )
    {
        amaroK::StatusBar::instance()->longMessage(
                i18n( "Post-disconnect command failed, before removing device, please make sure that it is safe to do so." ),
                KDE::StatusBar::Information );
        result = false;
    }
    else
        amaroK::StatusBar::instance()->shortMessage( i18n( "Device successfully disconnected" ) );

    return result;
}

void
MediaDevice::syncStatsFromDevice( MediaItem *root )
{
    MediaItem *it = static_cast<MediaItem *>( m_view->firstChild() );
    if( root )
    {
        it = static_cast<MediaItem *>( root->firstChild() );
    }

    for( ; it; it = static_cast<MediaItem *>( it->nextSibling() ) )
    {
        switch( it->type() )
        {
        case MediaItem::TRACK:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                for( int i=0; i<it->recentlyPlayed(); i++ )
                {
                    // submit to last.fm
                    if( bundle->length() > 30
                            && bundle->artist() != QString::null && bundle->artist() != "" && bundle->artist() != i18n( "Unknown" )
                            && bundle->title() != QString::null && bundle->title() != "" && bundle->title() != i18n( "Unknown" ) )
                    {
                        // don't submit tracks shorter than 30 sec or w/o artist/title
                        debug() << "scrobbling " << bundle->artist() << " - " << bundle->title() << endl;
                        SubmitItem *sit = new SubmitItem( bundle->artist(), bundle->album(), bundle->title(), bundle->length(), false /* fake time */ );
                        Scrobbler::instance()->m_submitter->submitItem( sit );
                    }

                    // increase Amarok playcount
                    QString url = CollectionDB::instance()->getURL( *bundle );
                    if( url != QString::null )
                    {
                        QDateTime t = it->playTime();
                        CollectionDB::instance()->addSongPercentage( url, 100, "mediadevice", t.isValid() ? &t : 0 );
                        debug() << "played " << url << endl;
                    }
                }

                if( it->ratingChanged() )
                {
                    // copy rating from media device to Amarok
                    QString url = CollectionDB::instance()->getURL( *bundle );
                    debug() << "rating changed " << url << ": " << it->rating()/10 << endl;
                    if( url != QString::null )
                    {
                        CollectionDB::instance()->setSongRating( url, it->rating()/10 );
                        it->setRating( it->rating() ); // prevent setting it again next time
                    }
                }
            }
            break;

        default:
            syncStatsFromDevice( it );
            break;
        }
    }
}

void
MediaDevice::syncStatsToDevice( MediaItem *root )
{
    MediaItem *it = static_cast<MediaItem *>( m_view->firstChild() );
    if( root )
    {
        it = static_cast<MediaItem *>( root->firstChild() );
    }

    for( ; it; it = static_cast<MediaItem *>( it->nextSibling() ) )
    {
        switch( it->type() )
        {
        case MediaItem::TRACK:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                QString url = CollectionDB::instance()->getURL( *bundle );

                if( url != QString::null )
                {
                    // copy Amarok rating to device
                    int rating = CollectionDB::instance()->getSongRating( url )*10;
                    it->setRating( rating );
                }
            }
            break;

        default:
            syncStatsToDevice( it );
            break;
        }
    }
}

void
MediaDevice::transferFiles()
{
    if( !lockDevice( true ) )
    {
        return;
    }

    setCancelled( false );

    m_transferring = true;
    m_parent->m_toolbar->getButton(MediaBrowser::TRANSFER)->setEnabled( false );

    setProgress( 0, m_parent->m_queue->childCount() + 1 /* for synchronizing */ );

    // ok, let's copy the stuff to the device

    MediaItem *playlist = 0;

    KURL::List existing, unplayable;
    unsigned transcodeFail = 0;
    MediaItem *after = 0; // item after which to insert into playlist
    // iterate through items
    while( (m_transferredItem = static_cast<MediaItem *>(m_parent->m_queue->firstChild())) != 0 )
    {
        if( isCancelled() )
            break;
        debug() << "Transferring: " << m_transferredItem->url().path() << endl;

        const MetaBundle *bundle = m_transferredItem->bundle();
        if(!bundle)
        {
            delete m_transferredItem;
            m_transferredItem = 0;
            m_parent->m_queue->itemCountChanged();
            continue;
        }

        bool transcoding = false;
        MediaItem *item = trackExists( *bundle );
        if( item && m_transferredItem->m_playlistName == QString::null )
        {
            amaroK::StatusBar::instance()->shortMessage( i18n( "Track already on media device: %1" ).
                    arg( m_transferredItem->url().prettyURL() ),
                    KDE::StatusBar::Sorry );
            existing += m_transferredItem->url();
            delete m_transferredItem;
            m_transferredItem = 0;
            m_parent->m_queue->itemCountChanged();
            setProgress( progress() + 1 );
            continue;
        }
        else if( !item ) // the item does not exist already on the media device
        {
            if( m_transcode &&
                    ( !isPlayable( *bundle ) || m_transcodeAlways ) )
            {
                QString preferred = supportedFiletypes().isEmpty() ? "mp3" : supportedFiletypes().first();
                debug() << "transcoding " << bundle->url() << " to " << preferred << endl;
                KURL transcoded = MediaBrowser::instance()->transcode( bundle->url(), preferred );
                if( transcoded.isEmpty() )
                {
                    debug() << "transcoding failed" << endl;
                    transcodeFail++;
                }
                else
                {
                    transcoding = true;
                    bundle = new MetaBundle( transcoded );
                }
            }

            if( !isPlayable( *bundle ) )
            {
                amaroK::StatusBar::instance()->shortMessage( i18n( "Track not playable on media device: %1" ).arg( bundle->url().path() ),
                        KDE::StatusBar::Sorry );
                unplayable += m_transferredItem->url();
                delete m_transferredItem;
                m_transferredItem = 0;
                m_parent->m_queue->itemCountChanged();
                if( transcoding )
                {
                    delete bundle;
                    bundle = 0;
                }
                setProgress( progress() + 1 );
                continue;
            }
            item = copyTrackToDevice( *bundle );
        }

        if( !item ) // copyTrackToDevice() failed
        {
            if( !isCancelled() )
                amaroK::StatusBar::instance()->longMessage(
                        i18n( "Failed to copy track to media device: %1" ).arg( bundle->url().path() ),
                        KDE::StatusBar::Sorry );
            if( transcoding )
            {
                delete bundle;
                bundle = 0;
            }
            m_transferredItem = 0;
            break;
        }

        if( transcoding )
        {
            if( m_transcodeRemove )
                QFile( bundle->url().path() ).remove();

            delete bundle;
            bundle = 0;
        }

        int rating = CollectionDB::instance()->getSongRating( m_transferredItem->bundle()->url().path() ) * 10;
        item->setRating( rating );

        if( playlist )
        {
            QPtrList<MediaItem> items;
            items.append(item);
            addToPlaylist(playlist, after, items);
            after = item;
        }

        if( m_playlistItem && m_transferredItem->m_playlistName!=QString::null )
        {
            MediaItem *pl = m_playlistItem->findItem( m_transferredItem->m_playlistName );
            if( !pl )
            {
                QPtrList<MediaItem> items;
                pl = newPlaylist( m_transferredItem->m_playlistName, m_playlistItem, items );
            }
            if( pl )
            {
                QPtrList<MediaItem> items;
                items.append( item );
                addToPlaylist( pl, pl->lastChild(), items );
            }
        }

        //synchronizeDevice();

        delete m_transferredItem;
        m_transferredItem = 0;
        setProgress( progress() + 1 );
        m_parent->m_queue->itemCountChanged();

        kapp->processEvents( 100 );
    }
    synchronizeDevice();
    unlockDevice();
    fileTransferFinished();

    QString msg;
    if( unplayable.count() > 0 )
    {
        msg = i18n( "One track not playable on media device",
                "%n tracks not playable on media device", unplayable.count() );
    }
    if( existing.count() > 0 )
    {
        if( msg.isEmpty() )
            msg = i18n( "One track already on media device",
                    "%n tracks already on media device", existing.count() );
        else
            msg += i18n( ", one track already on media device",
                    ", %n tracks already on media device", existing.count() );
    }
    if( transcodeFail > 0 )
    {
        if( msg.isEmpty() )
            msg = i18n( "One track was not transcoded",
                    "%n tracks were not transcoded", transcodeFail );
        else
            msg += i18n( ", one track was not transcoded",
                    ", %n tracks were not transcoded", transcodeFail );

        const ScriptManager* const sm = ScriptManager::instance();
        if( sm->transcodeScriptRunning() == QString::null )
            msg += i18n( " (no transcode script running)" );
    }

    if( unplayable.count() + existing.count() > 0 )
    {
        QString longMsg = i18n( "The following tracks were not transferred: ");
        for( KURL::List::Iterator it = existing.begin();
                it != existing.end();
                it++ )
        {
            longMsg += "<br>" + (*it).prettyURL();
        }
        for( KURL::List::Iterator it = unplayable.begin();
                it != unplayable.end();
                it++ )
        {
            longMsg += "<br>" + (*it).prettyURL();
        }
        amaroK::StatusBar::instance()->shortLongMessage( msg, longMsg, KDE::StatusBar::Sorry );
    }
    else if( !msg.isEmpty() )
    {
        amaroK::StatusBar::instance()->shortMessage( msg, KDE::StatusBar::Sorry );
    }

    m_parent->updateButtons();
    m_transferring = false;

    if( m_deferredDisconnect )
    {
        m_deferredDisconnect = false;
        disconnectDevice( m_runDisconnectHook );
    }
}

int
MediaDevice::progress() const
{
    return m_parent->m_progress->progress();
}

void
MediaDevice::setProgress( const int progress, const int total )
{
    if( total != -1 )
        m_parent->m_progress->setTotalSteps( total );
    m_parent->m_progress->setProgress( progress );
    m_parent->m_progressBox->show();
}

void
MediaDevice::fileTransferFinished()  //SLOT
{
    m_parent->updateStats();
    m_parent->m_progressBox->hide();
    m_parent->m_toolbar->getButton(MediaBrowser::TRANSFER)->setEnabled( false );
    m_wait = false;
}


int
MediaDevice::deleteFromDevice(MediaItem *item, bool onlyPlayed, bool recursing)
{
    MediaItem* fi = item;
    int count = 0;

    if ( !recursing )
    {
        if( !lockDevice( true ) )
            return 0;

        setCancelled( false );

        m_deleting = true;

        QPtrList<MediaItem> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_view->getSelectedLeaves(item, &list, true /* only selected */, onlyPlayed);

        m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", numFiles ) );
        if(numFiles > 0)
        {
            int button = KMessageBox::warningContinueCancel( m_parent,
                    i18n( "<p>You have selected 1 track to be <b>irreversibly</b> deleted.",
                        "<p>You have selected %n tracks to be <b>irreversibly</b> deleted.",
                        numFiles
                        ),
                    QString::null,
                    KGuiItem(i18n("&Delete"),"editdelete") );

            if ( button != KMessageBox::Continue )
            {
                m_parent->updateStats();
                m_deleting = false;
                unlockDevice();
                return 0;
            }

            if(!isTransferring())
            {
                setProgress( 0, numFiles + 1 /* sync is one step */ );
            }

        }
        // don't return if numFiles==0: playlist items might be to delete

        if( !fi )
            fi = static_cast<MediaItem*>(m_view->firstChild());
    }

    while( fi )
    {
        MediaItem *next = static_cast<MediaItem*>(fi->nextSibling());

        if( isCancelled() )
        {
            break;
        }

        if( !fi->isVisible() )
        {
            fi = next;
            continue;
        }

        if( fi->isSelected() )
        {
            int ret = deleteItemFromDevice(fi, onlyPlayed);
            if( ret >= 0 && count >= 0 )
                count += ret;
            else
                count = -1;
        }
        else
        {
            if( fi->childCount() )
            {
                int ret = deleteFromDevice( static_cast<MediaItem*>(fi->firstChild()), onlyPlayed, true );
                if( ret >= 0 && count >= 0 )
                    count += ret;
                else
                    count = -1;
            }
        }
        m_parent->updateStats();

        fi = next;
    }

    if(!recursing)
    {
        purgeEmptyItems();
        synchronizeDevice();
        m_deleting = false;
        unlockDevice();

        if(!isTransferring())
        {
            QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
        }

        if( m_deferredDisconnect )
        {
            m_deferredDisconnect = false;
            disconnectDevice( m_runDisconnectHook );
        }
    }
    m_parent->updateStats();

    return count;
}

void
MediaDevice::purgeEmptyItems( MediaItem *root )
{
    MediaItem *it = 0;
    if( root )
    {
        it = static_cast<MediaItem *>(root->firstChild());
    }
    else
    {
        it = static_cast<MediaItem *>(m_view->firstChild());
    }

    MediaItem *next = 0;
    for( ; it; it=next )
    {
        next = static_cast<MediaItem *>(it->nextSibling());
        purgeEmptyItems( it );
        if( it->childCount() == 0 &&
                (it->type() == MediaItem::ARTIST ||
                 it->type() == MediaItem::ALBUM ||
                 it->type() == MediaItem::PODCASTCHANNEL) )
            delete it;
    }
}

void
MediaQueue::save( const QString &path )
{
    QFile file( path );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument newdoc;
    QDomElement transferlist = newdoc.createElement( "playlist" );
    transferlist.setAttribute( "product", "Amarok" );
    transferlist.setAttribute( "version", APP_VERSION );
    newdoc.appendChild( transferlist );

    for( const MediaItem *item = static_cast<MediaItem *>( firstChild() );
            item;
            item = static_cast<MediaItem *>( item->nextSibling() ) )
    {
        QDomElement i = newdoc.createElement("item");
        i.setAttribute("url", item->url().url());

        if( item->bundle() )
        {
            QDomElement attr = newdoc.createElement( "Title" );
            QDomText t = newdoc.createTextNode( item->bundle()->title() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Artist" );
            t = newdoc.createTextNode( item->bundle()->artist() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Album" );
            t = newdoc.createTextNode( item->bundle()->album() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Year" );
            t = newdoc.createTextNode( QString::number( item->bundle()->year() ) );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Comment" );
            t = newdoc.createTextNode( item->bundle()->comment() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Genre" );
            t = newdoc.createTextNode( item->bundle()->genre() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Track" );
            t = newdoc.createTextNode( QString::number( item->bundle()->track() ) );
            attr.appendChild( t );
            i.appendChild( attr );
        }

        if(item->type() == MediaItem::PODCASTITEM)
        {
            i.setAttribute( "podcast", "1" );
        }

        if(item->type() == MediaItem::PODCASTITEM
                && item->bundle()->podcastBundle())
        {
            PodcastEpisodeBundle *peb = item->bundle()->podcastBundle();
            QDomElement attr = newdoc.createElement( "PodcastDescription" );
            QDomText t = newdoc.createTextNode( peb->description() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastAuthor" );
            t = newdoc.createTextNode( peb->author() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastRSS" );
            t = newdoc.createTextNode( peb->parent().url() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastURL" );
            t = newdoc.createTextNode( peb->url().url() );
            attr.appendChild( t );
            i.appendChild( attr );
        }

        if(item->m_playlistName != QString::null)
        {
            i.setAttribute( "playlist", item->m_playlistName );
        }

        transferlist.appendChild( i );
    }

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << newdoc.toString();
}


void
MediaQueue::load( const QString& filename )
{
    QFile file( filename );
    if( !file.open( IO_ReadOnly ) ) {
        return;
    }

    clearItems();

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    QString er;
    int l, c;
    if( !d.setContent( stream.read(), &er, &l, &c ) ) { // return error values
        amaroK::StatusBar::instance()->longMessageThreadSafe( i18n(
                //TODO add a link to the path to the playlist
                "The XML in the transferlist was invalid. Please report this as a bug to the Amarok "
                "developers. Thank you." ), KDE::StatusBar::Error );
        error() << "[TRANSFERLISTLOADER]: Error loading xml file: " << filename << "(" << er << ")"
                << " at line " << l << ", column " << c << endl;
        return;
    }

    QValueList<QDomNode> nodes;
    const QString ITEM( "item" ); //so we don't construct this QString all the time
    for( QDomNode n = d.namedItem( "playlist" ).firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        if( n.nodeName() != ITEM ) continue;

        QDomElement elem = n.toElement();
        if( !elem.isNull() )
            nodes += n;

        if( !elem.hasAttribute( "url" ) )
        {
            continue;
        }
        KURL url(elem.attribute("url"));

        bool podcast = elem.hasAttribute( "podcast" );
        PodcastEpisodeBundle peb;
        if( url.isLocalFile() )
            peb.setLocalURL( url );
        MetaBundle *bundle = new MetaBundle( url );
        for(QDomNode node = elem.firstChild();
                !node.isNull();
                node = node.nextSibling())
        {
            if(node.firstChild().isNull())
                continue;

            if(node.nodeName() == "Title" )
                bundle->setTitle(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "Artist" )
                bundle->setArtist(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "Album" )
                bundle->setAlbum(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "Year" )
                bundle->setYear(node.firstChild().toText().nodeValue().toUInt());
            else if(node.nodeName() == "Genre" )
                bundle->setGenre(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "Comment" )
                bundle->setComment(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "PodcastDescription" )
                peb.setDescription( node.firstChild().toText().nodeValue() );
            else if(node.nodeName() == "PodcastAuthor" )
                peb.setAuthor( node.firstChild().toText().nodeValue() );
            else if(node.nodeName() == "PodcastRSS" )
                peb.setParent( KURL::fromPathOrURL( node.firstChild().toText().nodeValue() ) );
            else if(node.nodeName() == "PodcastURL" )
                peb.setURL( KURL::fromPathOrURL( node.firstChild().toText().nodeValue() ) );
        }

        QString playlist = elem.attribute( "playlist" );
        if( podcast )
        {
            bundle->setPodcastBundle( peb );
        }
        addURL( url, bundle, playlist );
    }

    URLsAdded();
}

bool
MediaDevice::isPlayable( const MetaBundle &bundle )
{
    if( supportedFiletypes().isEmpty() )
        return true;

    QString type = bundle.url().path().section( ".", -1 ).lower();
    return supportedFiletypes().contains( type );
}

bool
MediaDevice::isPreferredFormat( const MetaBundle &bundle )
{
    if( supportedFiletypes().isEmpty() )
        return true;

    QString type = bundle.url().path().section( ".", -1 ).lower();
    return ( type == supportedFiletypes().first() );
}


MediaQueue::MediaQueue(MediaBrowser *parent)
    : KListView( parent ), m_parent( parent )
{
    setFixedHeight( 200 );
    setSelectionMode( QListView::Extended );
    setItemsMovable( true );
    setDragEnabled( true );
    setShowSortIndicator( false );
    setSorting( -1 );
    setFullWidth( true );
    setRootIsDecorated( false );
    setDropVisualizer( true );     //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );
    addColumn( i18n( "Transfer Queue" ) );

    itemCountChanged();

    KActionCollection* ac = new KActionCollection( this );
    KStdAction::selectAll( this, SLOT( selectAll() ), ac, "MediaQueue" );

    connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             SLOT( slotShowContextMenu( QListViewItem*, const QPoint&, int ) ) );
}

void
MediaQueue::dragEnterEvent( QDragEnterEvent *e )
{
    KListView::dragEnterEvent( e );

    QString data;
    QCString subtype;
    QTextDrag::decode( e, data, subtype );

    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && (subtype == "amarok-sql" || KURLDrag::canDecode( e )) );
}


void
MediaQueue::dropEvent( QDropEvent *e )
{
    KListView::dropEvent( e );

    QString data;
    QCString subtype;
    QTextDrag::decode( e, data, subtype );
    KURL::List list;

    if( subtype == "amarok-sql" )
    {
        QStringList values = CollectionDB::instance()->query( data );
        for( QStringList::iterator it = values.begin();
                it != values.end();
                it++ )
        {
            it += QueryBuilder::dragFieldCount-2;
            list += KURL::fromPathOrURL( *it );
            it++;
        }
        addURLs( list );
    }
    else if ( KURLDrag::decode( e, list ) )
    {
        addURLs( list );
    }
}

void
MediaQueue::contentsDragEnterEvent( QDragEnterEvent *e )
{
    KListView::contentsDragEnterEvent( e );

    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && KURLDrag::canDecode( e ) );
}


void
MediaQueue::contentsDropEvent( QDropEvent *e )
{
    KListView::contentsDropEvent( e );

    KURL::List list;
    if ( KURLDrag::decode( e, list ) )
        addURLs( list );
}

void
MediaQueue::contentsDragMoveEvent( QDragMoveEvent *e )
{
    KListView::contentsDragMoveEvent( e );

    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && KURLDrag::canDecode( e ) );
}

MediaItem*
MediaQueue::findPath( QString path )
{
    for( QListViewItem *item = firstChild();
            item;
            item = item->nextSibling())
    {
        if(static_cast<MediaItem *>(item)->url().path() == path)
            return static_cast<MediaItem *>(item);
    }

    return 0;
}

KIO::filesize_t
MediaQueue::totalSize() const
{
    KIO::filesize_t total = 0;
    for( QListViewItem *it = firstChild();
            it;
            it = it->nextSibling())
    {
        MediaItem *item = static_cast<MediaItem *>(it);

        if( item && ( !m_parent->currentDevice()
                || !m_parent->currentDevice()->isConnected()
                || !m_parent->currentDevice()->trackExists(*item->bundle()) ) )
            total += ((item->size()+1023)/1024)*1024;
    }

    return total;
}

void
MediaQueue::removeSelected()
{
    QPtrList<QListViewItem>  selected = selectedItems();

    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        if( !m_parent->currentDevice()
                || !m_parent->currentDevice()->isTransferring()
                || item != m_parent->currentDevice()->transferredItem() )
        {
            delete item;
            if( m_parent->currentDevice() && m_parent->currentDevice()->isTransferring() )
            {
                MediaBrowser::instance()->m_progress->setTotalSteps( MediaBrowser::instance()->m_progress->totalSteps() - 1 );
            }
        }
    }

    MediaBrowser::instance()->updateStats();
    MediaBrowser::instance()->updateButtons();
    itemCountChanged();
}

void
MediaQueue::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Key_Delete )
    {
        removeSelected();
    }
}

void
MediaQueue::itemCountChanged()
{
    if( childCount() == 0 )
        hide();
    else if( !isShown() )
        show();
}

void
MediaQueue::slotShowContextMenu( QListViewItem* item, const QPoint& point, int )
{
    if( !childCount() )
        return;

    KPopupMenu menu( this );

    enum Actions { REMOVE_SELECTED, CLEAR_ALL, START_TRANSFER };

    if( item )
        menu.insertItem( SmallIconSet( amaroK::icon( "remove_from_playlist" ) ), i18n( "&Remove From Queue" ), REMOVE_SELECTED );

    menu.insertItem( SmallIconSet( amaroK::icon( "playlist_clear" ) ), i18n( "&Clear Queue" ), CLEAR_ALL );
    menu.insertItem( SmallIconSet( amaroK::icon( "playlist_refresh" ) ), i18n( "&Start Transfer" ), START_TRANSFER );
    menu.setItemEnabled( START_TRANSFER,
            MediaBrowser::instance()->currentDevice() &&
            MediaBrowser::instance()->currentDevice()->isConnected() );

    switch( menu.exec( point ) )
    {
        case REMOVE_SELECTED:
            removeSelected();
            break;
        case CLEAR_ALL:
            clearItems();
            break;
        case START_TRANSFER:
            MediaBrowser::instance()->transferClicked();
            break;
    }
}

void
MediaQueue::clearItems()
{
    clear();
    itemCountChanged();
    if(m_parent)
    {
        m_parent->updateStats();
        m_parent->updateButtons();
    }
}


#include "mediabrowser.moc"
