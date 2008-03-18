// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// (c) 2006 T.R.Shashwath <trshash84@gmail.com>
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
#include "editfilterdialog.h"
#include "deviceconfiguredialog.h"
#include "mediadevicemanager.h"
#include "expression.h"
#include "hintlineedit.h"
#include "mediabrowser.h"
#include "medium.h"
#include "mediumpluginmanager.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "playlist.h"
#include "playlistbrowser.h"
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
#include <qobjectlist.h>
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
#include <kmultipledrag.h>
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
QPixmap *MediaItem::s_pixTransferFailed = 0;
QPixmap *MediaItem::s_pixTransferBegin = 0;
QPixmap *MediaItem::s_pixTransferEnd = 0;

bool MediaBrowser::isAvailable() //static
{
    if( !MediaBrowser::instance() )
        return false;

    return true;

    //to re-enable hiding, uncomment this and get rid of the return true above:
    //return MediaBrowser::instance()->m_haveDevices;
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
            //Amarok::StatusBar::instance()->longMessage( msg, KDE::StatusBar::Sorry );
        }
        return false;
    }
    virtual bool closeDevice() { return false; }
    virtual void synchronizeDevice() {}
    virtual MediaItem* copyTrackToDevice(const MetaBundle&) { return 0; }
    virtual int deleteItemFromDevice(MediaItem*, int) { return -1; }
};


MediaBrowser::MediaBrowser( const char *name )
        : QVBox( 0, name )
        , m_timer( new QTimer( this ) )
        , m_currentDevice( m_devices.end() )
        , m_waitForTranscode( false )
        , m_quitting( false )
{
    s_instance = this;

    // preload pixmaps used in browser
    KIconLoader iconLoader;
    MediaItem::s_pixUnknown = new QPixmap(iconLoader.loadIcon( Amarok::icon( "unknown" ), KIcon::Toolbar, KIcon::SizeSmall ));
    MediaItem::s_pixTrack = new QPixmap(iconLoader.loadIcon( Amarok::icon( "playlist" ), KIcon::Toolbar, KIcon::SizeSmall ));
    MediaItem::s_pixFile = new QPixmap(iconLoader.loadIcon( Amarok::icon( "sound" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixPodcast = new QPixmap(iconLoader.loadIcon( Amarok::icon( "podcast" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixPlaylist = new QPixmap(iconLoader.loadIcon( Amarok::icon( "playlist" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixRootItem = new QPixmap(iconLoader.loadIcon( Amarok::icon( "files2" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    // history
    // favorites
    // collection
    // folder
    // folder_red
    // player_playlist_2
    // cancel
    // sound
    MediaItem::s_pixArtist = new QPixmap(iconLoader.loadIcon( Amarok::icon( "personal" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixAlbum = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cdrom_unmount" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixInvisible = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cancel" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixStale = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cancel" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixOrphaned = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cancel" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixDirectory = new QPixmap(iconLoader.loadIcon( Amarok::icon( "folder" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixTransferBegin = new QPixmap(iconLoader.loadIcon( Amarok::icon( "play" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixTransferEnd = new QPixmap(iconLoader.loadIcon( Amarok::icon( "stop" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixTransferFailed = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cancel" ), KIcon::Toolbar, KIcon::SizeSmall ) );

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

   // m_toolbar->setIconText( KToolBar::IconTextRight, true );
    m_toolbar->insertButton( Amarok::icon( "add_playlist" ), CUSTOM, SIGNAL( clicked() ), this, SLOT( customClicked() ), true, "custom" );
    QToolTip::add( m_toolbar->getButton(TRANSFER), i18n( "Transfer tracks to media device" ) );

    m_toolbar->setIconText( KToolBar::IconOnly, false );

    m_toolbar->insertButton( Amarok::icon( "configure" ), CONFIGURE, true, i18n("Configure") );
    QToolTip::add( m_toolbar->getButton(CONFIGURE), i18n( "Configure device" ) );


    m_deviceCombo = new KComboBox( this );

    // searching/filtering
    { //<Search LineEdit>
        KToolBar* searchToolBar = new Browser::ToolBar( this );
        KToolBarButton *button = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_searchEdit = new ClickLineEdit( i18n( "Enter search terms here" ), searchToolBar );
        KPushButton *filterButton = new KPushButton("...", searchToolBar, "filter");
        searchToolBar->setStretchableWidget( m_searchEdit );
        m_searchEdit->setFrame( QFrame::Sunken );

        connect( button, SIGNAL( clicked() ), m_searchEdit, SLOT( clear() ) );
        connect( filterButton, SIGNAL( clicked() ), SLOT( slotEditFilter() ) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to search" ) );
        QToolTip::add( filterButton, i18n( "Click to edit filter" ) );
    } //</Search LineEdit>

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_searchEdit, SIGNAL( returnPressed() ), SLOT( slotSetFilter() ) );

    // connect to device manager
    connect( MediaDeviceManager::instance(), SIGNAL( mediumAdded(const Medium *, QString) ),
            SLOT( mediumAdded(const Medium *, QString) ) );
    connect( MediaDeviceManager::instance(), SIGNAL( mediumChanged(const Medium *, QString) ),
            SLOT( mediumChanged(const Medium *, QString) ) );
    connect( MediaDeviceManager::instance(), SIGNAL( mediumRemoved(const Medium *, QString) ),
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
    m_cancelButton = new KPushButton( SmallIconSet( Amarok::icon( "cancel" ) ), i18n("Cancel"), m_progressBox );


    m_stats = new SpaceLabel(this);

    m_progressBox->hide();

    MediaDevice *dev = new DummyMediaDevice();
    dev->init( this );
    addDevice( dev );
    activateDevice( 0, false );
    queue()->load( Amarok::saveLocation() + "transferlist.xml" );
    queue()->computeSize();

    setFocusProxy( m_queue );

    updateStats();

    QMap<QString, Medium*> mmap = MediaDeviceManager::instance()->getMediumMap();

    bool newflag = false;
    //This deals with <strike>auto-detectable</strike> ALL devices!
    for( QMap<QString, Medium*>::Iterator it = mmap.begin();
            it != mmap.end();
            it++ )
    {
        QString handler = Amarok::config( "MediaBrowser" )->readEntry( (*it)->id() );
        //debug() << "[MediaBrowser] (*it)->id() = " << (*it)->id() << ", handler = " << handler << endl;
        if( handler.isEmpty() )
        {
            //this should probably never be the case with a manually added device, unless amarokrc's been messed with
            Amarok::config( "MediaBrowser" )->writeEntry( (*it)->id(), "ignore" );
            newflag = true;
            mediumAdded( *it, (*it)->name(), true );
        }
        //and this definitely shouldn't!
        else if( handler != "deleted" )
            mediumAdded( *it, (*it)->name(), true );
    }

    if ( newflag )
        Amarok::StatusBar::instance()->longMessageThreadSafe(
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
    QMap<QString,QString> savedDevices = Amarok::config( "MediaBrowser" )->entryMap( "MediaBrowser" );
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
    m_itemMapMutex.lock();
    debug() << "tags changed for " << bundle.url().url() << endl;
    ItemMap::iterator it = m_itemMap.find( bundle.url().url() );
    if( it != m_itemMap.end() )
    {
        MediaItem *item = *it;
        m_itemMapMutex.unlock();
        if( item->device() )
        {
            item->device()->tagsChanged( item, bundle );
        }
        else
        {
            // it's an item on the transfer queue
            item->setBundle( new MetaBundle( bundle ) );

            QString text = item->bundle()->prettyTitle();
            if( text.isEmpty() || (!item->bundle()->isValidMedia() && !item->bundle()->podcastBundle()) )
                text = item->bundle()->url().prettyURL();
            if( !item->m_playlistName.isNull() )
            {
                text += " (" + item->m_playlistName + ')';
            }
            item->setText( 0, text);
        }
    }
    else
    {
        m_itemMapMutex.unlock();
    }
}

bool
MediaBrowser::getBundle( const KURL &url, MetaBundle *bundle ) const
{
    QMutexLocker locker( &m_itemMapMutex );
    ItemMap::const_iterator it = m_itemMap.find( url.url() );
    if( it == m_itemMap.end() )
        return false;

    if( bundle )
        *bundle = QDeepCopy<MetaBundle>( *(*it)->bundle() );

    return true;
}

KURL
MediaBrowser::getProxyUrl( const KURL& daapUrl ) const
{
    DEBUG_BLOCK
    KURL url;
    MediaDevice* dc = dynamic_cast<MediaDevice*>( queryList( "DaapClient" )->getFirst() );
    if( dc )
        url = dc->getProxyUrl( daapUrl );
    return url;
}

MediaDevice *
MediaBrowser::currentDevice() const
{
    QValueList<MediaDevice *>::const_iterator current = m_currentDevice;
    if( current != m_devices.constEnd() )
    {
        return *m_currentDevice;
    }

    return 0;
}

MediaDevice *
MediaBrowser::deviceFromId( const QString &id ) const
{
    for( QValueList<MediaDevice *>::const_iterator it = m_devices.constBegin();
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
    if( currentDevice() && currentDevice()->customAction() )
    {
        currentDevice()->customAction()->unplug( m_toolbar );
        m_toolbar->hide();
        m_toolbar->show();
    }

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
        updateButtons();
        queue()->computeSize();
        updateStats();
        return;
    }

    m_currentDevice = m_devices.at( index );
    if( currentDevice() )
    {
        currentDevice()->view()->show();
        if( currentDevice()->customAction() )
        {
            m_toolbar->setIconText( KToolBar::IconTextRight, false );
            currentDevice()->customAction()->plug( m_toolbar );
            m_toolbar->hide();
            m_toolbar->show();
        }
    }
    m_deviceCombo->setCurrentItem( index-1 );

    updateButtons();
    queue()->computeSize();
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
            Amarok::StatusBar::instance()->longMessage(
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
        if( (*it)->hasMountPoint() && !(*it)->mountPoint().isEmpty() )
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
MediaBrowser::deviceNames() const
{
    QStringList list;

    for( QValueList<MediaDevice *>::const_iterator it = m_devices.constBegin();
            it != m_devices.constEnd();
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
            << "but we are waiting for " << m_transcodeSrc << " -- aborting" << endl;
        m_waitForTranscode = false;
    }
}

KURL
MediaBrowser::transcode( const KURL &src, const QString &filetype )
{
    const ScriptManager* const sm = ScriptManager::instance();

    if( sm->transcodeScriptRunning().isEmpty() )
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
MediaBrowser::slotSetFilter( const QString &text )
{
    m_searchEdit->setText( text );
    slotSetFilter();
}

void
MediaBrowser::slotEditFilter()
{
    EditFilterDialog *fd = new EditFilterDialog( this, true, m_searchEdit->text() );
    connect( fd, SIGNAL(filterChanged(const QString &)), SLOT(slotSetFilter(const QString &)) );
    if( fd->exec() )
        m_searchEdit->setText( fd->filter() );
    delete fd;
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

    queue()->save( Amarok::saveLocation() + "transferlist.xml" );

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
    m_playlistName = QString::null;
    m_device=0;
    m_flags=0;
    setExpandable( false );
    setDragEnabled( true );
    setDropEnabled( true );
}

void
MediaItem::setBundle( MetaBundle *bundle )
{
    MediaBrowser::instance()->m_itemMapMutex.lock();
    if( m_bundle )
    {
	QString itemUrl = url().url();
        MediaBrowser::ItemMap::iterator it = MediaBrowser::instance()->m_itemMap.find( itemUrl );
        if( it != MediaBrowser::instance()->m_itemMap.end() && *it == this )
            MediaBrowser::instance()->m_itemMap.remove( itemUrl );
    }
    delete m_bundle;
    m_bundle = bundle;

    if( m_bundle )
    {
	QString itemUrl = url().url();
        MediaBrowser::ItemMap::iterator it = MediaBrowser::instance()->m_itemMap.find( itemUrl );
        if( it == MediaBrowser::instance()->m_itemMap.end() )
            MediaBrowser::instance()->m_itemMap[itemUrl] = this;
    }
    MediaBrowser::instance()->m_itemMapMutex.unlock();
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

void
MediaItem::setFailed( bool failed )
{
    if( failed )
    {
        m_flags &= ~MediaItem::Transferring;
        m_flags |= MediaItem::Failed;
        setPixmap(0, *MediaItem::s_pixTransferFailed);
    }
    else
    {
        m_flags &= ~MediaItem::Failed;
        if( m_type == PODCASTITEM )
            setPixmap(0, *s_pixPodcast);
        else if( m_type == PLAYLIST )
            setPixmap(0, *s_pixPlaylist);
        else
            setPixmap(0, QPixmap() );
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
    else if( item && item->type() == MediaItem::ARTIST )
    {
        QString key1 = key( col, ascending );
        if( key1.startsWith( "the ", false ) )
            key1 = key1.mid( 4 );
        QString key2 = i->key( col, ascending );
        if( key2.startsWith( "the ", false ) )
            key2 = key2.mid( 4 );

       return key1.localeAwareCompare( key2 );
    }

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

    connect( this, SIGNAL( contextMenuRequested( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );

    connect( this, SIGNAL( itemRenamed( QListViewItem* ) ),
             this,   SLOT( renameItem( QListViewItem* ) ) );

    connect( this, SIGNAL( expanded( QListViewItem* ) ),
             this,   SLOT( slotExpand( QListViewItem* ) ) );

    connect( this, SIGNAL( returnPressed( QListViewItem* ) ),
             this,   SLOT( invokeItem( QListViewItem* ) ) );

    connect( this, SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( invokeItem( QListViewItem*, const QPoint &, int ) ) );

    m_toolTip = new MediaItemTip( this );
}

void
MediaView::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Key_Delete )
        m_device->deleteFromDevice();
    else
        KListView::keyPressEvent( e );
}

void
MediaView::invokeItem( QListViewItem* i, const QPoint& point, int column ) //SLOT
{
    if( column == -1 )
        return;

    QPoint p = mapFromGlobal( point );
    if ( p.x() > header()->sectionPos( header()->mapToIndex( 0 ) ) + treeStepSize() * ( i->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin()
            || p.x() < header()->sectionPos( header()->mapToIndex( 0 ) ) )
        invokeItem( i );
}


void
MediaView::invokeItem( QListViewItem *i )
{
    MediaItem *item = dynamic_cast<MediaItem *>( i );
    if( !item )
        return;

    KURL::List urls = nodeBuildDragList( item );
    Playlist::instance()->insertMedia( urls, Playlist::DefaultOptions );
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


QDragObject *
MediaView::dragObject()
{
    KURL::List urls = nodeBuildDragList( 0 );
    KMultipleDrag *md = new KMultipleDrag( viewport() );
    md->addDragObject( KListView::dragObject() );
    KURLDrag* ud = new KURLDrag( urls, viewport() );
    md->addDragObject( ud );
    md->setPixmap( CollectionDB::createDragPixmap( urls ),
                  QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    return md;
}


KURL::List
MediaView::nodeBuildDragList( MediaItem* item, int flags )
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
            if ( fi->isSelected() || !(flags & OnlySelected ) )
            {
                if( fi->isLeafItem() || fi->type() == MediaItem::DIRECTORY )
                    items += fi->url();
                else
                {
                    if(fi->childCount() )
                        items += nodeBuildDragList( static_cast<MediaItem*>(fi->firstChild()), None );
                }
            }
            else
            {
                if ( fi->childCount() )
                    items += nodeBuildDragList( static_cast<MediaItem*>(fi->firstChild()), OnlySelected );
            }
        }
        fi = static_cast<MediaItem*>(fi->nextSibling());
    }
    return items;
}

int
MediaView::getSelectedLeaves( MediaItem *parent, QPtrList<MediaItem> *list, int flags )
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
                int f = flags;
                if( it->isSelected() )
                    f &= ~OnlySelected;
                numFiles += getSelectedLeaves(it, list, f );
            }
            if( it->isSelected() || !(flags & OnlySelected) )
            {
                if( it->type() == MediaItem::TRACK       ||
                    it->type() == MediaItem::DIRECTORY   ||
                    it->type() == MediaItem::PODCASTITEM ||
                    it->type() == MediaItem::PLAYLISTITEM||
                    it->type() == MediaItem::INVISIBLE   ||
                    it->type() == MediaItem::ORPHANED     )
                {
                    if( flags & OnlyPlayed )
                    {
                        if( it->played() > 0 )
                            numFiles++;
                    }
                    else
                        numFiles++;
                }
                if( ( it->isLeafItem() && (!(flags & OnlyPlayed) || it->played()>0) )
                        || it->type() == MediaItem::DIRECTORY )
                    list->append( it );
            }
        }
    }
    return numFiles;
}


bool
MediaView::acceptDrag( QDropEvent *e ) const
{
    if( e->source() == MediaBrowser::queue()->viewport() )
        return false;

    QString data;
    QCString subtype;
    QTextDrag::decode( e, data, subtype );

    return e->source() == viewport()
        || subtype == "amarok-sql"
        || KURLDrag::canDecode( e );
}

void
MediaView::contentsDropEvent( QDropEvent *e )
{
    cleanDropVisualizer();
    cleanItemHighlighter();

    if(e->source() == viewport())
    {
        const QPoint p = contentsToViewport( e->pos() );
        MediaItem *item = dynamic_cast<MediaItem *>(itemAt( p ));

        if( !item && MediaBrowser::instance()->currentDevice()->m_type != "generic-mediadevice" )
            return;

        QPtrList<MediaItem> items;

        if( !item || item->type() == MediaItem::DIRECTORY ||
                    item->type() == MediaItem::TRACK )
        {
            QPtrList<MediaItem> items;
            getSelectedLeaves( 0, &items );
            m_device->addToDirectory( item, items );
        }
        else if( item->type() == MediaItem::PLAYLIST )
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
                name = base + ' ' + num;
                i++;
            }
            MediaItem *pl = m_device->newPlaylist(name, item, items);
            ensureItemVisible(pl);
            rename(pl, 0);
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
            QString playlist = data.section( "\n", 0, 0 );
            QString query = data.section( "\n", 1 );
            QStringList values = CollectionDB::instance()->query( query );
            list = CollectionDB::instance()->URLsFromSqlDrag( values );
            MediaBrowser::queue()->addURLs( list, playlist );
        }
        else if ( KURLDrag::decode( e, list ) )
        {
            MediaBrowser::queue()->addURLs( list );
        }
    }
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
    MediaBrowser::instance()->updateButtons();
}

void
MediaView::rmbPressed( QListViewItem *item, const QPoint &p, int i )
{
    if( m_device->isConnected() )
        m_device->rmbPressed( item, p, i );
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
MediaBrowser::mediumAdded( const Medium *medium, QString /*name*/, bool /*constructing*/ )
{
    debug() << "mediumAdded: " << (medium? medium->properties():"null") << endl;
    if( medium )
    {
        QString handler = Amarok::config( "MediaBrowser" )->readEntry( medium->id() );
        if( handler.isEmpty() )
        {
            //Some people complained about the dialog, boohoo
            //Just disable it for now I guess
            /*if( !constructing && medium->isAutodetected() )
            {
                MediumPluginManagerDialog *mpm = new MediumPluginManagerDialog();
                mpm->exec();
            }*/
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
        Amarok::config( "MediaBrowser" )->writeEntry( medium->id(), plugin );

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
            Amarok::StatusBar::instance()->longMessage(
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
                if( !(*it)->isConnected() && medium->isMounted() )
                    (*it)->connectDevice();
#if 0
                else if( (*it)->isConnected() && !medium->isMounted() )
                {
                    Amarok::StatusBar::instance()->longMessage(
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
                    Amarok::StatusBar::instance()->longMessage(
                            i18n( "The device %1 was removed before it was disconnected. "
                                "In order to avoid possible data loss, press the \"Disconnect\" "
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
    Amarok::Plugin *plugin = PluginManager::createFromQuery( query.arg( deviceType ) );

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
        KConfig *config = Amarok::config( "MediaBrowser" );
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
                Amarok::StatusBar::instance()->shortMessage( msg );
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
            !m_toolbar->getButton(TRANSFER) ) //TODO add CUSTOM
        return;

    if( currentDevice() )
    {
        if( currentDevice()->m_transfer )
            m_toolbar->showItem( TRANSFER );
        else
            m_toolbar->hideItem( TRANSFER );

        if( currentDevice()->m_customButton )
            m_toolbar->showItem( CUSTOM );
        else
            m_toolbar->hideItem( CUSTOM );

        if( currentDevice()->m_configure )
            m_toolbar->showItem( CONFIGURE );
        else
            m_toolbar->hideItem( CONFIGURE );

        m_toolbar->getButton(CONNECT)->setEnabled( !currentDevice()->isConnected() );
        m_toolbar->getButton(DISCONNECT)->setEnabled( currentDevice()->isConnected() );
        m_toolbar->getButton(TRANSFER)->setEnabled( currentDevice()->isConnected() && m_queue->childCount() > 0 );
        m_toolbar->getButton( CUSTOM )->setEnabled( true );
    }
    else
    {
        m_toolbar->getButton( CONNECT )->setEnabled( false );
        m_toolbar->getButton( DISCONNECT )->setEnabled( false );
        m_toolbar->getButton( TRANSFER )->setEnabled( false );
        m_toolbar->getButton( CUSTOM )->setEnabled( false );
    }
}

void
MediaBrowser::updateStats()
{
    if( !m_stats )
        return;

    KIO::filesize_t queued = m_queue->totalSize();

    QString text = i18n( "1 track in queue", "%n tracks in queue", m_queue->childCount() );
    if(m_queue->childCount() > 0)
    {
        text += i18n(" (%1)").arg( KIO::convertSize( queued ) );
    }

    KIO::filesize_t total, avail;
    if( currentDevice() && currentDevice()->getCapacity(&total, &avail) )
    {
        text += i18n( " - %1 of %2 available" ).arg( KIO::convertSize( avail ) ).arg( KIO::convertSize( total ) );

        m_stats->m_used = total-avail;
        m_stats->m_total = total;
        m_stats->m_scheduled = queued;
    }
    else
    {
        m_stats->m_used = 0;
        m_stats->m_total = 0;
        m_stats->m_scheduled = queued;
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

    bool root = false;
    MediaItem *it;
    if( !parent )
    {
        root = true;
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
            else if(it->type()==MediaItem::DIRECTORY)
            {
                bool match = true;
                QStringList list = QStringList::split( " ", filter );
                for( QStringList::iterator i = list.begin();
                        i != list.end();
                        ++i )
                {
                    if( !(*it).text(0).contains( *i ) )
                    {
                        match = false;
                        break;
                    }
                }
                if( match )
                    visible = true;
            }
        }
        if( filter.isEmpty() )
            visible = true;
        it->setVisible( visible );
        if(visible)
            childrenVisible = true;
    }

    if( root && m_device )
        m_device->updateRootItems();

    return childrenVisible;
}

MediaDevice::MediaDevice()
    : Amarok::Plugin()
    , m_hasMountPoint( true )
    , m_autoDeletePodcasts( false )
    , m_syncStats( false )
    , m_transcode( false )
    , m_transcodeAlways( false )
    , m_transcodeRemove( false )
    , sysProc ( 0 )
    , m_parent( 0 )
    , m_view( 0 )
    , m_wait( false )
    , m_requireMount( false )
    , m_canceled( false )
    , m_transferring( false )
    , m_deleting( false )
    , m_deferredDisconnect( false )
    , m_scheduledDisconnect( false )
    , m_transfer( true )
    , m_configure( true )
    , m_customButton( false )
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
    if( m_requireMount && m_postdisconnectcmd.isEmpty() )
        m_postdisconnectcmd = "kdeeject -q %d";
}

QString
MediaDevice::configString( const QString &name, const QString &defValue )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += '_' + uniqueId();
    KConfig *config = Amarok::config( configName );
    return config->readEntry( name, defValue );
}

void
MediaDevice::setConfigString( const QString &name, const QString &value )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += '_' + uniqueId();
    KConfig *config = Amarok::config( configName );
    config->writeEntry( name, value );
}

bool
MediaDevice::configBool( const QString &name, bool defValue )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += '_' + uniqueId();
    KConfig *config = Amarok::config( configName );
    return config->readBoolEntry( name, defValue );
}

void
MediaDevice::setConfigBool( const QString &name, bool value )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += '_' + uniqueId();
    KConfig *config = Amarok::config( configName );
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
MediaQueue::syncPlaylist( const QString &name, const QString &query, bool loading )
{
    MediaItem* item = new MediaItem( this, lastItem() );
    item->setType( MediaItem::PLAYLIST );
    item->setExpandable( false );
    item->setData( query );
    item->m_playlistName = name;
    item->setText( 0, name );
    item->m_flags |= MediaItem::SmartPlaylist;
    m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() + 1 );
    itemCountChanged();
    if( !loading )
        URLsAdded();
}

void
MediaQueue::syncPlaylist( const QString &name, const KURL &url, bool loading )
{
    MediaItem* item = new MediaItem( this, lastItem() );
    item->setType( MediaItem::PLAYLIST );
    item->setExpandable( false );
    item->setData( url.url() );
    item->m_playlistName = name;
    item->setText( 0, name );
    m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() + 1 );
    itemCountChanged();
    if( !loading )
        URLsAdded();
}

BundleList
MediaDevice::bundlesToSync( const QString &name, const KURL &url )
{
    BundleList bundles;
    if( !PlaylistFile::isPlaylistFile( url ) )
    {
        Amarok::StatusBar::instance()->longMessage( i18n( "Not a playlist file: %1" ).arg( url.path() ),
                KDE::StatusBar::Sorry );
        return bundles;
    }

    PlaylistFile playlist( url.path() );
    if( playlist.isError() )
    {
        Amarok::StatusBar::instance()->longMessage( i18n( "Failed to load playlist: %1" ).arg( url.path() ),
                KDE::StatusBar::Sorry );
        return bundles;
    }

    for( BundleList::iterator it = playlist.bundles().begin();
            it != playlist.bundles().end();
            ++it )
    {
        bundles += MetaBundle( (*it).url() );
    }
    preparePlaylistForSync( name, bundles );
    return bundles;
}

BundleList
MediaDevice::bundlesToSync( const QString &name, const QString &query )
{
    const QStringList values = CollectionDB::instance()->query( query );

    BundleList bundles;
    for( for_iterators( QStringList, values ); it != end; ++it )
        bundles += CollectionDB::instance()->bundleFromQuery( &it );
    preparePlaylistForSync( name, bundles );
    return bundles;
}

void
MediaDevice::preparePlaylistForSync( const QString &name, const BundleList &bundles )
{
    if( ! m_playlistItem ) // might be syncing a new playlist from the playlist browser
        return;
    MediaItem *pl = m_playlistItem->findItem( name );
    if( pl )
    {
        MediaItem *next = 0;
        for( MediaItem *it = static_cast<MediaItem *>(pl->firstChild());
                it;
                it = next )
        {
            next = static_cast<MediaItem *>(it->nextSibling());
            const MetaBundle *bundle = (*it).bundle();
            if( !bundle )
                continue;
            if( isOnOtherPlaylist( name, *bundle ) )
                continue;
            if( isInBundleList( bundles, *bundle ) )
                continue;
            deleteItemFromDevice( it );
        }
        deleteItemFromDevice( pl, None );
    }
    purgeEmptyItems();
}

bool
MediaDevice::bundleMatch( const MetaBundle &b1, const MetaBundle &b2 )
{
    if( b1.track() != b2.track() )
        return false;
    if( b1.title() != b2.title() )
        return false;
    if( b1.album() != b2.album() )
        return false;
    if( b1.artist() != b2.artist() )
        return false;
#if 0
    if( b1.discNumber() != b2.discNumber() )
        return false;
    if( b1.composer() != b2.composer() )
        return false;
#endif

    return true;
}

bool
MediaDevice::isInBundleList( const BundleList &bundles, const MetaBundle &b )
{
    for( BundleList::const_iterator it = bundles.begin();
            it != bundles.end();
            ++it )
    {
        if( bundleMatch( b, *it ) )
            return true;
    }

    return false;
}

bool
MediaDevice::isOnOtherPlaylist( const QString &playlistToAvoid, const MetaBundle &bundle )
{
    for( MediaItem *it = static_cast<MediaItem *>(m_playlistItem->firstChild());
            it;
            it = static_cast<MediaItem *>(it->nextSibling()) )
    {
        if( it->text( 0 )  == playlistToAvoid )
            continue;
        if( isOnPlaylist( *it, bundle ) )
            return true;
    }

    return false;
}

bool
MediaDevice::isOnPlaylist( const MediaItem &playlist, const MetaBundle &bundle )
{
    for( MediaItem *it = static_cast<MediaItem *>(playlist.firstChild());
            it;
            it = static_cast<MediaItem *>(it->nextSibling()) )
    {
        const MetaBundle *b = (*it).bundle();
        if( !b )
            continue;
        if( bundleMatch( *b, bundle ) )
            return true;
    }

    return false;
}

void
MediaQueue::addURL( const KURL& url2, MetaBundle *bundle, const QString &playlistName )
{
    KURL url = Amarok::mostLocalURL( url2 );

    if( PlaylistFile::isPlaylistFile( url ) )
    {
        QString name = url.path().section( "/", -1 ).section( ".", 0, -2 ).replace( "_", " " );
        PlaylistFile playlist( url.path() );

        if( playlist.isError() )
        {
            Amarok::StatusBar::instance()->longMessage( i18n( "Failed to load playlist: %1" ).arg( url.path() ),
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
        KURL::List urls = Amarok::recursiveUrlExpand( url );
        foreachType( KURL::List, urls )
            addURL( *it );
        return;
    }

    if( playlistName.isNull() )
    {
        for( MediaItem *it = static_cast<MediaItem *>(firstChild());
                it;
                it = static_cast<MediaItem *>(it->nextSibling()) )
        {
            if( it->url() == url )
            {
                Amarok::StatusBar::instance()->shortMessage(
                        i18n( "Track already queued for transfer: %1" ).arg( url.url() ) );
                return;
            }
        }
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
    if( !item->m_playlistName.isNull() )
    {
        text += " (" + item->m_playlistName + ')';
    }
    item->setText( 0, text);

    m_parent->updateButtons();
    m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() + 1 );
    addItemToSize( item );
    itemCountChanged();
}

void
MediaQueue::addURL( const KURL &url, MediaItem *item )
{
    DEBUG_BLOCK
    MediaItem *newitem = new MediaItem( this, lastItem() );
    newitem->setExpandable( false );
    newitem->setDropEnabled( true );
    MetaBundle *bundle = new MetaBundle( *item->bundle() );
    KURL filepath(url);
    filepath.addPath( bundle->filename() );
    bundle->setUrl( filepath );
    newitem->m_device = item->m_device;
    if(bundle->podcastBundle() )
    {
        item->setType( MediaItem::PODCASTITEM );
    }
    QString text = item->bundle()->prettyTitle();
    if( text.isEmpty() || (!item->bundle()->isValidMedia() && !item->bundle()->podcastBundle()) )
        text = item->bundle()->url().prettyURL();
    if( item->m_playlistName != QString::null )
    {
        text += " (" + item->m_playlistName + ')';
    }
    newitem->setText( 0, text);
    newitem->setBundle( bundle );
    m_parent->updateButtons();
    m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() + 1 );
    addItemToSize( item );
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
    m_parent->updateButtons();
    if( m_parent->currentDevice()
            && m_parent->currentDevice()->isConnected()
            && m_parent->currentDevice()->asynchronousTransfer()
            && !m_parent->currentDevice()->isTransferring() )
        m_parent->currentDevice()->transferFiles();

    save( Amarok::saveLocation() + "transferlist.xml" );
}

void
MediaDevice::copyTrackFromDevice( MediaItem *item )
{
    debug() << "copyTrackFromDevice: not copying " << item->url() << ": not implemented" << endl;
}

QDragObject *
MediaQueue::dragObject()
{
    KURL::List urls;
    for( QListViewItem *it = firstChild(); it; it = it->nextSibling() )
    {
        if( it->isVisible() && it->isSelected() && dynamic_cast<MediaItem *>(it) )
            urls += static_cast<MediaItem *>(it)->url();
    }

    KMultipleDrag *md = new KMultipleDrag( viewport() );
    QDragObject *d = KListView::dragObject();
    KURLDrag* urldrag = new KURLDrag( urls, viewport() );
    md->addDragObject( d );
    md->addDragObject( urldrag );
    md->setPixmap( CollectionDB::createDragPixmap( urls ),
                  QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    return md;
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
    setCanceled( true );
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
        if( isCanceled() )
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
            Amarok::StatusBar::instance()->longMessage(
                    i18n( "Media Device: Copying %1 to %2 failed" )
                    .arg( src.prettyURL(), dst.prettyURL() ),
                    KDE::StatusBar::Error );
        }
        else
        {
            MetaBundle bundle2(dst);
            if(!bundle2.isValidMedia() && bundle2.filesize()==MetaBundle::Undetermined)
            {
                tryToRemove = true;
                // probably s.th. went wrong
                Amarok::StatusBar::instance()->longMessage(
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
    if( currentDevice() && currentDevice()->isTransferring() )
    {
        int action = KMessageBox::questionYesNoCancel( MediaBrowser::instance(),
                i18n( "Transfer in progress. Finish or stop after current track?" ),
                i18n( "Stop Transfer?" ),
                KGuiItem(i18n("&Finish"), "goto"),
                KGuiItem(i18n("&Stop"), "player_eject") );
        if( action == KMessageBox::Cancel )
        {
            return;
        }
        else if( action == KMessageBox::Yes )
        {
            currentDevice()->scheduleDisconnect();
            return;
        }
    }

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

void
MediaBrowser::customClicked()
{
    if( currentDevice() )
        currentDevice()->customClicked();
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
            && MediaBrowser::instance()->currentDevice()
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
        int numFiles  = m_view->getSelectedLeaves( m_podcastItem, &list, MediaView::OnlyPlayed );

        if(numFiles > 0)
        {
            m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", numFiles ) );

            setProgress( 0, numFiles );

            int numDeleted = deleteItemFromDevice( m_podcastItem, true );
            purgeEmptyItems();
            if( numDeleted < 0 )
            {
                Amarok::StatusBar::instance()->longMessage(
                        i18n( "Failed to purge podcasts already played" ),
                        KDE::StatusBar::Sorry );
            }
            else if( numDeleted > 0 )
            {
                Amarok::StatusBar::instance()->shortMessage(
                        i18n( "Purged 1 podcasts already played",
                            "Purged %n podcasts already played",
                            numDeleted ) );
            }

            synchronizeDevice();

            QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
            m_parent->queue()->computeSize();
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

    Amarok::StatusBar::instance()->shortMessage( i18n( "Device successfully connected" ) );

    m_parent->updateDevices();

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
        Amarok::StatusBar::instance()->longMessage(
                i18n( "Post-disconnect command failed, before removing device, please make sure that it is safe to do so." ),
                KDE::StatusBar::Information );
        result = false;
    }
    else
        Amarok::StatusBar::instance()->shortMessage( i18n( "Device successfully disconnected" ) );

    m_parent->updateDevices();

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

    kapp->processEvents( 100 );

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
                            && !bundle->artist().isEmpty() && bundle->artist() != i18n( "Unknown" )
                            && !bundle->title().isEmpty() && bundle->title() != i18n( "Unknown" ) )
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
        case MediaItem::PODCASTITEM:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                if( it->played() || it->recentlyPlayed() )
                {
                    if( PodcastEpisodeBundle *peb = bundle->podcastBundle() )
                    {
                        debug() << "marking podcast episode as played: " << peb->url() << endl;
                        if( PlaylistBrowser::instance() )
                        {
                            PodcastEpisode *p = PlaylistBrowser::instance()->findPodcastEpisode( peb->url(), peb->parent() );
                            if ( p )
                                p->setListened();
                            else
                                debug() << "did not find podcast episode: " << peb->url() << " from " << peb->parent() << endl;
                        }
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
MediaItem::syncStatsFromPath( const QString &url )
{
    if( url.isEmpty() )
        return;

    // copy Amarok rating, play count and last played time to device
    int rating = CollectionDB::instance()->getSongRating( url )*10;
    if( rating )
        setRating( rating );
    int playcount = CollectionDB::instance()->getPlayCount( url );
    if( playcount > played() )
        setPlayCount( playcount );
    QDateTime lastplay = CollectionDB::instance()->getLastPlay( url );
    if( lastplay > playTime() )
        setLastPlayed( lastplay.toTime_t() );
}

void
MediaDevice::syncStatsToDevice( MediaItem *root )
{
    MediaItem *it = static_cast<MediaItem *>( m_view->firstChild() );
    if( root )
    {
        it = static_cast<MediaItem *>( root->firstChild() );
    }

    kapp->processEvents( 100 );

    for( ; it; it = static_cast<MediaItem *>( it->nextSibling() ) )
    {
        switch( it->type() )
        {
        case MediaItem::TRACK:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                QString url = CollectionDB::instance()->getURL( *bundle );
                it->syncStatsFromPath( url );
            }
            break;

        case MediaItem::PODCASTITEM:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                if( PodcastEpisodeBundle *peb = bundle->podcastBundle() )
                {
                    if( PlaylistBrowser::instance() )
                    {
                        PodcastEpisode *p = PlaylistBrowser::instance()->findPodcastEpisode( peb->url(), peb->parent() );
                        if( p )
                            it->setListened( !p->isNew() );
                    }
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

    setCanceled( false );

    m_transferring = true;
    m_parent->m_toolbar->getButton(MediaBrowser::TRANSFER)->setEnabled( false );

    setProgress( 0, m_parent->m_queue->childCount() );

    // ok, let's copy the stuff to the device

    KURL::List existing, unplayable;
    unsigned transcodeFail = 0;
    // iterate through items
    MediaItem *next = static_cast<MediaItem *>(m_parent->m_queue->firstChild());
    while( next )
    {
        MediaItem *transferredItem = next;
        transferredItem->setFailed( false );
        transferredItem->m_flags |= MediaItem::Transferring;
        next = static_cast<MediaItem *>( transferredItem->nextSibling() );

        if( transferredItem->device() )
        {
            transferredItem->device()->copyTrackFromDevice( transferredItem );
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            setProgress( progress() + 1 );
            m_parent->m_queue->itemCountChanged();
            kapp->processEvents( 100 );
            continue;
        }

        BundleList bundles;
        if( transferredItem->type() == MediaItem::PLAYLIST )
        {
            if( transferredItem->flags() & MediaItem::SmartPlaylist )
                bundles = bundlesToSync( transferredItem->text( 0 ), transferredItem->data() );
            else
                bundles = bundlesToSync( transferredItem->text( 0 ), KURL::fromPathOrURL( transferredItem->data() ) );
        }
        else if( transferredItem->bundle() )
            bundles += *transferredItem->bundle();
        else
        {
            // this should not happen
            debug() << "invalid item in transfer queue" << endl;
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            m_parent->m_queue->itemCountChanged();
            continue;
        }

        if( bundles.count() > 1 )
            setProgress( progress(), MediaBrowser::instance()->m_progress->totalSteps() + bundles.count() - 1 );

        QString playlist = transferredItem->m_playlistName;
        for( BundleList::const_iterator it = bundles.begin();
                it != bundles.end();
                ++it )
        {
            if( isCanceled() )
                break;

            const MetaBundle *bundle = &(*it);

            bool transcoding = false;
            MediaItem *item = trackExists( *bundle );
            if( item && playlist.isEmpty() )
            {
                Amarok::StatusBar::instance()->shortMessage( i18n( "Track already on media device: %1" ).
                        arg( (*it).url().prettyURL() ),
                        KDE::StatusBar::Sorry );
                existing += (*it).url();
                setProgress( progress() + 1 );
                continue;
            }
            else if( !item ) // the item does not yet exist on the media device
            {
                if( m_transcode && ( !isPlayable( *bundle ) || m_transcodeAlways ) )
                {
                    QString preferred = supportedFiletypes().isEmpty() ? "mp3" : supportedFiletypes().first();
                    debug() << "transcoding " << bundle->url() << " to " << preferred << endl;
                    KURL transcoded = MediaBrowser::instance()->transcode( bundle->url(), preferred );
                    if( isCanceled() )
                        break;
                    if( transcoded.isEmpty() )
                    {
                        debug() << "transcoding failed" << endl;
                        transcodeFail++;
                    }
                    else
                    {
                        transcoding = true;
                        MetaBundle *transcodedBundle = new MetaBundle( transcoded );
                        transcodedBundle->setArtist( bundle->artist() );
                        transcodedBundle->setTitle( bundle->title() );
                        transcodedBundle->setComposer( bundle->composer() );
                        transcodedBundle->setAlbum( bundle->album() );
                        transcodedBundle->setGenre( bundle->genre() );
                        transcodedBundle->setComment( bundle->comment() );
                        transcodedBundle->setYear( bundle->year() );
                        transcodedBundle->setDiscNumber( bundle->discNumber() );
                        transcodedBundle->setTrack( bundle->track() );
                        if( bundle->podcastBundle() )
                        {
                            transcodedBundle->setPodcastBundle( *bundle->podcastBundle() );
                            transcodedBundle->copyFrom( *bundle->podcastBundle() );
                            //change the extension on the localUrl in the podcastbundle
                            //to make sure it ends up with the correct extension
                            //in the generic mediadevice.
                            KURL localUrl = transcodedBundle->podcastBundle()->localUrl();
                            QString filename = QFileInfo( localUrl.path() ).baseName() + '.' + preferred;
                            localUrl.setFileName( filename );
                            transcodedBundle->podcastBundle()->setLocalURL( localUrl );
                        }
                        bundle = transcodedBundle;
                    }
                }

                if( !isPlayable( *bundle ) )
                {
                    Amarok::StatusBar::instance()->shortMessage( i18n( "Track not playable on media device: %1" ).arg( bundle->url().path() ),
                            KDE::StatusBar::Sorry );
                    unplayable += (*it).url();
                    transferredItem->setFailed();
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
                if( !isCanceled() )
                {
                    Amarok::StatusBar::instance()->longMessage(
                            i18n( "Failed to copy track to media device: %1" ).arg( bundle->url().path() ),
                            KDE::StatusBar::Sorry );
                    transferredItem->setFailed();
                }
            }

            if( transcoding )
            {
                if( m_transcodeRemove )
                    QFile( bundle->url().path() ).remove();

                delete bundle;
                bundle = 0;
            }

            if( isCanceled() )
                break;

            if( !item )
            {
                setProgress( progress() + 1 );
                continue;
            }

            item->syncStatsFromPath( (*it).url().path() );

            if( m_playlistItem && !playlist.isEmpty() )
            {
                MediaItem *pl = m_playlistItem->findItem( playlist );
                if( !pl )
                {
                    QPtrList<MediaItem> items;
                    pl = newPlaylist( playlist, m_playlistItem, items );
                }
                if( pl )
                {
                    QPtrList<MediaItem> items;
                    items.append( item );
                    addToPlaylist( pl, pl->lastChild(), items );
                }
            }

            setProgress( progress() + 1 );
        }

        transferredItem->m_flags &= ~MediaItem::Transferring;

        if( isCanceled() )
        {
            m_parent->updateStats();
            break;
        }

        if( !(transferredItem->flags() & MediaItem::Failed) )
        {
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            m_parent->m_queue->itemCountChanged();
        }
        m_parent->updateStats();

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
        if( !sm->transcodeScriptRunning().isEmpty() )
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
        Amarok::StatusBar::instance()->shortLongMessage( msg, longMsg, KDE::StatusBar::Sorry );
    }
    else if( !msg.isEmpty() )
    {
        Amarok::StatusBar::instance()->shortMessage( msg, KDE::StatusBar::Sorry );
    }

    m_parent->updateButtons();
    m_parent->queue()->save( Amarok::saveLocation() + "transferlist.xml" );
    m_transferring = false;

    if( m_deferredDisconnect )
    {
        m_deferredDisconnect = false;
        disconnectDevice( m_runDisconnectHook );
    }
    else if( m_scheduledDisconnect )
    {
        disconnectDevice( true );
    }
    m_scheduledDisconnect = false;
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
    m_parent->m_toolbar->getButton(MediaBrowser::TRANSFER)->setEnabled( isConnected() && m_parent->queue()->childCount() > 0 );
    m_wait = false;
}


int
MediaDevice::deleteFromDevice(MediaItem *item, int flags )
{
    MediaItem* fi = item;
    int count = 0;

    if ( !(flags & Recursing) )
    {
        if( !lockDevice( true ) )
            return 0;

        setCanceled( false );

        m_deleting = true;

        QPtrList<MediaItem> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_view->getSelectedLeaves(item, &list, MediaView::OnlySelected | ((flags & OnlyPlayed) ? MediaView::OnlyPlayed : MediaView::None) );

        m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", numFiles ) );
        if( numFiles > 0 && (flags & DeleteTrack) )
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
                m_parent->queue()->computeSize();
                m_parent->updateStats();
                m_deleting = false;
                unlockDevice();
                return 0;
            }

            if(!isTransferring())
            {
                setProgress( 0, numFiles );
            }

        }
        // don't return if numFiles==0: playlist items might be to delete

        if( !fi )
            fi = static_cast<MediaItem*>(m_view->firstChild());
    }

    while( fi )
    {
        MediaItem *next = static_cast<MediaItem*>(fi->nextSibling());

        if( isCanceled() )
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
            int ret = deleteItemFromDevice(fi, flags);
            if( ret >= 0 && count >= 0 )
                count += ret;
            else
                count = -1;
        }
        else
        {
            if( fi->childCount() )
            {
                int ret = deleteFromDevice( static_cast<MediaItem*>(fi->firstChild()), flags | Recursing );
                if( ret >= 0 && count >= 0 )
                    count += ret;
                else
                    count = -1;
            }
        }
        m_parent->updateStats();

        fi = next;
    }

    if(!(flags & Recursing))
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
    m_parent->queue()->computeSize();
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

        if(item->type() == MediaItem::PLAYLIST)
        {
            i.setAttribute( "playlistdata", item->data() );
            if( item->flags() & MediaItem::SmartPlaylist )
                i.setAttribute( "smartplaylist", "1" );
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
        Amarok::StatusBar::instance()->longMessageThreadSafe( i18n(
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

        if( podcast )
        {
            bundle->setPodcastBundle( peb );
        }

        QString playlist = elem.attribute( "playlist" );
        QString playlistdata = elem.attribute( "playlistdata" );
        if( !playlistdata.isEmpty() )
        {
            QString smart = elem.attribute( "smartplaylist" );
            if( smart.isEmpty() )
                syncPlaylist( playlist, KURL::fromPathOrURL( playlistdata ), true );
            else
                syncPlaylist( playlist, playlistdata, true );
        }
        else
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

    connect( this, SIGNAL( contextMenuRequested( QListViewItem*, const QPoint&, int ) ),
            SLOT( slotShowContextMenu( QListViewItem*, const QPoint&, int ) ) );
    connect( this, SIGNAL( dropped(QDropEvent*, QListViewItem*, QListViewItem*) ),
            SLOT( slotDropped(QDropEvent*, QListViewItem*, QListViewItem*) ) );
}

bool
MediaQueue::acceptDrag( QDropEvent *e ) const
{
    QString data;
    QCString subtype;
    QTextDrag::decode( e, data, subtype );

    return e->source() == viewport()
        || subtype == "amarok-sql"
        || KURLDrag::canDecode( e );
}

void
MediaQueue::slotDropped( QDropEvent* e, QListViewItem* parent, QListViewItem* after)
{
    if( e->source() != viewport() )
    {
        QString data;
        QCString subtype;
        QTextDrag::decode( e, data, subtype );
        KURL::List list;

        if( subtype == "amarok-sql" )
        {
            QString playlist = data.section( "\n", 0, 0 );
            QString query = data.section( "\n", 1 );
            QStringList values = CollectionDB::instance()->query( query );
            list = CollectionDB::instance()->URLsFromSqlDrag( values );
            addURLs( list, playlist );
        }
        else if ( KURLDrag::decode( e, list ) )
        {
            addURLs( list );
        }
    }
    else if( QListViewItem *i = currentItem() )
    {
        moveItem( i, parent, after );
    }
}

void
MediaQueue::dropProxyEvent( QDropEvent *e )
{
    slotDropped( e, 0, 0 );
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

void
MediaQueue::computeSize() const
{
    m_totalSize = 0;
    for( QListViewItem *it = firstChild();
            it;
            it = it->nextSibling())
    {
        MediaItem *item = static_cast<MediaItem *>(it);

        if( item && item->bundle() &&
                ( !m_parent->currentDevice()
                  || !m_parent->currentDevice()->isConnected()
                  || !m_parent->currentDevice()->trackExists(*item->bundle()) ) )
            m_totalSize += ((item->size()+1023)/1024)*1024;
    }
}

KIO::filesize_t
MediaQueue::totalSize() const
{
    return m_totalSize;
}

void
MediaQueue::addItemToSize( const MediaItem *item ) const
{
    if( item && item->bundle() &&
            ( !m_parent->currentDevice()
              || !m_parent->currentDevice()->isConnected()
              || !m_parent->currentDevice()->trackExists(*item->bundle()) ) )
        m_totalSize += ((item->size()+1023)/1024)*1024;
}

void
MediaQueue::subtractItemFromSize( const MediaItem *item, bool unconditionally ) const
{
    if( item && item->bundle() &&
            ( !m_parent->currentDevice()
              || !m_parent->currentDevice()->isConnected()
              || (unconditionally || !m_parent->currentDevice()->trackExists(*item->bundle())) ) )
        m_totalSize -= ((item->size()+1023)/1024)*1024;
}

void
MediaQueue::removeSelected()
{
    QPtrList<QListViewItem>  selected = selectedItems();

    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        if( !(static_cast<MediaItem *>(item)->flags() & MediaItem::Transferring) )
        {
            subtractItemFromSize( static_cast<MediaItem *>(item) );
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
        removeSelected();
    else
        KListView::keyPressEvent( e );
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
        menu.insertItem( SmallIconSet( Amarok::icon( "remove_from_playlist" ) ), i18n( "&Remove From Queue" ), REMOVE_SELECTED );

    menu.insertItem( SmallIconSet( Amarok::icon( "playlist_clear" ) ), i18n( "&Clear Queue" ), CLEAR_ALL );
    menu.insertItem( SmallIconSet( Amarok::icon( "playlist_refresh" ) ), i18n( "&Start Transfer" ), START_TRANSFER );
    menu.setItemEnabled( START_TRANSFER,
            MediaBrowser::instance()->currentDevice() &&
            MediaBrowser::instance()->currentDevice()->isConnected() &&
            MediaBrowser::instance()->currentDevice()->m_transfer );

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
        computeSize();
        m_parent->updateStats();
        m_parent->updateButtons();
    }
}


#include "mediabrowser.moc"
