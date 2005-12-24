// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information


#define DEBUG_PREFIX "MediaBrowser"

#include <config.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "browserToolBar.h"
#include "clicklineedit.h"
#include "collectiondb.h"
#include "colorgenerator.h"
#include "contextbrowser.h"
#include "debug.h"
#include "devicemanager.h"
#include "mediabrowser.h"
#include "medium.h"
#include "mediumpluginchooser.h"
#include "metabundle.h"
#include "playlist.h"
#include "playlistloader.h"
#include "pluginmanager.h"
#include "scrobbler.h"
#include "statusbar.h"

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
#include <qregexp.h>
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
#include <klocale.h>
#include <kmessagebox.h>
#include <kmountpoint.h>
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

    return true;
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
                if(m_used + m_scheduled > m_total - m_total/1000)
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

    unsigned long m_total;
    unsigned long m_used;
    unsigned long m_scheduled;
};

class DummyMediaDevice : public MediaDevice
{
    public:
    DummyMediaDevice() : MediaDevice() { m_name = "Do-Nothing Device"; }
    void init( MediaBrowser *browser ) { MediaDevice::init( browser ); }
    virtual ~DummyMediaDevice() {}
    virtual bool isConnected() { return false; }
    virtual void cancelTransfer() {}
    virtual void addToPlaylist(MediaItem*, MediaItem*, QPtrList<MediaItem>) {}
    virtual void addToDirectory(MediaItem*, QPtrList<MediaItem>) {}
    virtual MediaItem* newDirectory(const QString&, MediaItem*) { return 0; }
    virtual MediaItem* newPlaylist(const QString&, MediaItem*, QPtrList<MediaItem>) { return 0; }
    virtual MediaItem* trackExists(const MetaBundle&) { return 0; }
    virtual void lockDevice(bool) {}
    virtual void unlockDevice() {}
    virtual bool openDevice( bool )
    {
        QString msg = i18n( "Sorry, you do not have a supported portable music player." );
        amaroK::StatusBar::instance()->longMessage( msg, KDE::StatusBar::Sorry );
        return false;
    }
    virtual bool closeDevice() { return false; }
    virtual void synchronizeDevice() {}
    virtual MediaItem* copyTrackToDevice(const MetaBundle&, const PodcastInfo*) { return 0; }
    virtual int deleteItemFromDevice(MediaItem*, bool) { return -1; }
    virtual bool getCapacity( unsigned long *, unsigned long * ) { return false; }
};


MediaBrowser::MediaBrowser( const char *name )
        : QVBox( 0, name )
        , m_timer( new QTimer( this ) )
        , m_currentDevice( m_devices.end() )
{
    s_instance = this;

    // preload pixmaps used in browser
    KIconLoader iconLoader;
    MediaItem::s_pixUnknown = new QPixmap(iconLoader.loadIcon( "unknown", KIcon::Toolbar, KIcon::SizeSmall ));
    MediaItem::s_pixTrack = new QPixmap(iconLoader.loadIcon( "player_playlist_2", KIcon::Toolbar, KIcon::SizeSmall ));
    MediaItem::s_pixFile = new QPixmap(iconLoader.loadIcon( "sound", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixPodcast = new QPixmap(iconLoader.loadIcon( "favorites", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixPlaylist = new QPixmap(iconLoader.loadIcon( "player_playlist_2", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixRootItem = new QPixmap(iconLoader.loadIcon( "folder_red", KIcon::Toolbar, KIcon::SizeSmall ) );
    // history
    // favorites
    // collection
    // folder
    // folder_red
    // player_playlist_2
    // cancel
    // sound
    MediaItem::s_pixArtist = new QPixmap(iconLoader.loadIcon( "personal", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixAlbum = new QPixmap(iconLoader.loadIcon( "cdrom_unmount", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixInvisible = new QPixmap(iconLoader.loadIcon( "cancel", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixStale = new QPixmap(iconLoader.loadIcon( "cancel", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixOrphaned = new QPixmap(iconLoader.loadIcon( "cancel", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixDirectory = new QPixmap(iconLoader.loadIcon( "folder", KIcon::Toolbar, KIcon::SizeSmall ) );

    setSpacing( 4 );

    // searching/filtering
    { //<Search LineEdit>
        KToolBar* searchToolBar = new Browser::ToolBar( this );
        KToolBarButton *button = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_searchEdit = new ClickLineEdit( i18n( "Filter here..." ), searchToolBar );

        searchToolBar->setStretchableWidget( m_searchEdit );
        m_searchEdit->setFrame( QFrame::Sunken );

        connect( button, SIGNAL( clicked() ), m_searchEdit, SLOT( clear() ) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to filter collection" ) ); //TODO text is wrong
    } //</Search LineEdit>

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_searchEdit, SIGNAL( returnPressed() ), SLOT( slotSetFilter() ) );

    m_deviceCombo = new KComboBox( this );

    // connect to device manager
    connect( DeviceManager::instance(), SIGNAL( mediumAdded(const Medium *, QString) ),
            SLOT( mediumAdded(const Medium *, QString) ) );
    connect( DeviceManager::instance(), SIGNAL( mediumChanged(const Medium *, QString) ),
            SLOT( mediumChanged(const Medium *, QString) ) );
    connect( DeviceManager::instance(), SIGNAL( mediumRemoved(const Medium *, QString) ),
            SLOT( mediumRemoved(const Medium *, QString) ) );


    // query available device plugins
    m_pluginName[ i18n( "Disable" ) ] = "dummy-mediadevice";
    m_pluginAmarokName["dummy-mediadevice"] = i18n( "Disable" );
    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'mediadevice'" );
    KTrader::OfferList::ConstIterator end( offers.end() );
    for( KTrader::OfferList::ConstIterator it = offers.begin(); it != end; ++it ) {
        debug() << "possible properties: " << (*it)->propertyNames() << endl;
        // Save name properties in QMap for lookup
        m_pluginName[(*it)->name()] = (*it)->property( "X-KDE-amaroK-name" ).toString();
        m_pluginAmarokName[(*it)->property( "X-KDE-amaroK-name" ).toString()] = (*it)->name();
        QStringList supports = (*it)->property( "X-KDE-amaroK-compatible" ).toStringList();
        for( uint i = 0; i < supports.count(); i++ )
        {
            m_pluginSupports[supports[i]] = (*it)->property( "X-KDE-amaroK-name" ).toString();
            debug() << (*it)->name() << " supports " << supports[i] << endl;
        }
    }

    m_views = new QVBox( this );
    //m_view = new MediaView( this );
    m_queue = new MediaQueue( this );
    m_progressBox  = new QHBox( this );
    m_progress     = new KProgress( m_progressBox );
    m_cancelButton = new KPushButton( SmallIconSet("cancel"), i18n("Cancel"), m_progressBox );

    QHBox* hb = new QHBox( this );
    hb->setSpacing( 1 );
    m_connectButton  = new KPushButton( SmallIconSet( "connect_creating" ), i18n( "Connect"), hb );
    m_disconnectButton  = new KPushButton( SmallIconSet( "connect_no" ), i18n( "Disconnect"), hb );
    m_transferButton = new KPushButton( SmallIconSet( "rebuild" ), i18n( "Transfer" ), hb );

    m_playlistButton = new KPushButton( KGuiItem( QString::null, "player_playlist_2" ), hb );
    m_playlistButton->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
    m_playlistButton->setToggleButton( true );

    m_configButton   = new KPushButton( KGuiItem( QString::null, "configure" ), hb );
    m_configButton->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred ); // too big!
    connect( m_configButton,   SIGNAL( clicked() ), this,                    SLOT( config() ) );

    m_stats = new SpaceLabel(this);

    QToolTip::add( m_connectButton,  i18n( "Connect media device" ) );
    QToolTip::add( m_disconnectButton,  i18n( "Disconnect media device" ) );
    QToolTip::add( m_transferButton, i18n( "Transfer tracks to media device" ) );
    QToolTip::add( m_playlistButton, i18n( "Append transferred items to playlist \"New amaroK additions\"" ) );
    QToolTip::add( m_configButton,   i18n( "Configure media device" ) );

    m_transferButton->setDisabled( true );

    m_progressBox->setFixedHeight( m_transferButton->sizeHint().height() );
    m_progressBox->hide();

    MediaDevice *dev = new DummyMediaDevice();
    dev->init( this );
    addDevice( dev );
    activateDevice( 0 );
    queue()->load( amaroK::saveLocation() + "transferlist.xml" );

    setFocusProxy( m_queue );

    updateStats();

    Medium::List devices = DeviceManager::instance()->getDeviceList();
    for( Medium::List::iterator it = devices.begin();
            it != devices.end();
            it++ )
    {
        if ( (*it).fsType() == "smbfs" || (*it).fsType() == "ext2" ||
                (*it).fsType() == "ext3" || (*it).fsType() == "reiserfs" ||
                (*it).fsType() == "xfs" || (*it).fsType() == "jfs" ||
                (*it).fsType() == "ntfs" || (*it).fsType() == "udf" ||
                (*it).fsType() == "iso9660" || (*it).fsType() == "nfs" )
            continue;

        mediumAdded( &*it, (*it).name() );
    }

    connect( m_deviceCombo, SIGNAL( activated( int ) ), SLOT( activateDevice( int ) ) );
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


void
MediaBrowser::activateDevice( int index )
{
    if( index < 0 || index >= m_devices.count() )
        return;

    if( m_currentDevice == m_devices.at( index ) )
        return;

    if( m_currentDevice != m_devices.end() )
    {
        disconnect( *m_currentDevice );
        (*m_currentDevice)->view()->hide();
    }

    m_currentDevice = m_devices.at( index );
    m_deviceCombo->setCurrentItem( index );
    (*m_currentDevice)->view()->show();

    connect( m_cancelButton,     SIGNAL( clicked() ), currentDevice(), SLOT( abortTransfer() ) );
    connect( m_connectButton,    SIGNAL( clicked() ), currentDevice(), SLOT( connectClicked() ) );
    connect( m_disconnectButton, SIGNAL( clicked() ), currentDevice(), SLOT( disconnectClicked() ) );
    connect( m_transferButton,   SIGNAL( clicked() ), currentDevice(), SLOT( transferFiles() ) );

    updateButtons();
    updateStats();
    if( currentDevice()->m_hasPlaylists )
        m_playlistButton->show();
    else
        m_playlistButton->hide();
}

void
MediaBrowser::addDevice( MediaDevice *device )
{
    m_devices.append( device );

    if( device->autoConnect() )
    {
        m_connectButton->setOn( true );
        device->connectClicked( true );
        updateButtons();
    }

    updateDevices();
}

void
MediaBrowser::removeDevice( MediaDevice *device )
{
    DEBUG_BLOCK

    debug() << "remove device: type=" << device->deviceType() << endl;

    if( device->isConnected() )
        device->synchronizeDevice();
    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        if( *it == device )
        {
            if( it == m_currentDevice )
                m_currentDevice = (it == m_devices.begin()) ? m_devices.end() : m_devices.begin();
            m_devices.remove( device );
            break;
        }
    }
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
        m_deviceCombo->insertItem( (*it)->name(), i );
        if( it == m_currentDevice )
        {
            m_deviceCombo->setCurrentItem( i );
        }
        i++;
    }
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

    currentDevice()->view()->setFilter( m_searchEdit->text() );
}

MediaBrowser::~MediaBrowser()
{
    queue()->save( amaroK::saveLocation() + "transferlist.xml" );

    debug() << "have to remove " << m_devices.count() << " devices" << endl;

    QValueList<MediaDevice *>::iterator next = m_devices.end();
    for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it = next )
    {
        next = it;
        next++;
        if( (*it)->isConnected() )
            (*it)->synchronizeDevice();
        removeDevice( *it );
    }

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
    delete m_bundle;
    delete m_podcastInfo;
}

void
MediaItem::init()
{
    m_bundle=0;
    m_podcastInfo=0;
    m_order=0;
    m_type=UNKNOWN;
    m_playlistName=QString::null;
    setExpandable( false );
    setDragEnabled( true );
    setDropEnabled( true );
    m_size=-1;
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
            font.setPointSize( font.pointSize() + 1 );
            p->setFont( font );
        }
    default:
        break;
    }

    KListViewItem::paintCell( p, cg, column, width, align );
}

void MediaItem::setUrl( const QString& url )
{
    m_url.setPath( url );
}

const MetaBundle *
MediaItem::bundle() const
{
    if( !m_bundle )
        m_bundle = new MetaBundle( url() );
    return m_bundle;
}

MetaBundle *
MediaItem::bundle()
{
    if( !m_bundle )
        m_bundle = new MetaBundle( url() );
    return m_bundle;
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
    if(m_size < 0)
    {
        if(!isFileBacked())
        {
            m_size = 0;
        }
        else
        {
            QFile file( url().path() );
            m_size = file.size();
        }
    }

    return m_size;
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
        case PODCASTITEM:
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
    }
    return 0;
}

int
MediaItem::compare( QListViewItem *i, int col, bool ascending ) const
{
    MediaItem *item = dynamic_cast<MediaItem *>(i);
    if(item && col==0 && item->m_order != m_order)
        return ascending ? m_order-item->m_order : item->m_order-m_order;

    return KListViewItem::compare(i, col, ascending);
}

class MediaItemTip : public QToolTip
{
    public:
    MediaItemTip( QListView *listview )
    : QToolTip( listview->viewport() )
    , m_listView( listview )
    {}
    virtual ~MediaItemTip() {}
    protected:
    virtual void maybeTip( const QPoint &p )
    {
        MediaItem *i = dynamic_cast<MediaItem *>(m_listView->itemAt( m_listView->viewportToContents( p ) ) );
        if( !i )
            return;

        QString text;
        switch( i->type() )
        {
        case MediaItem::TRACK:
            {
                const MetaBundle *b = i->bundle();
                text = "";
                if( b->track() )
                    text = QString( "%1 - %2 (%3:%4)<br>" )
                        .arg( b->track() )
                        .arg( b->title() )
                        .arg( b->length()/60 )
                        .arg( b->length()%60 );
                if( !b->genre().isEmpty() )
                    text += QString( "<i>Genre: %1</i>" )
                        .arg( b->genre() );
            }
            break;
        case MediaItem::PODCASTITEM:
            {
                const PodcastInfo *p = i->podcastInfo();
                if( p )
                {
                    text += p->description;
                    if( !p->webpage.isEmpty() )
                        text += i18n( "<br>Webpage: %1<br>" ).arg( p->webpage );
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
            tip( m_listView->itemRect( i ), text );
    }

    QListView *m_listView;
};


MediaView::MediaView( QWidget* parent, MediaDevice *device )
    : KListView( parent )
    , m_parent( parent )
    , m_device( device )
{
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
    debug() << urls.first().path() << endl;
    KURLDrag* d = new KURLDrag( urls, this );
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
                if( fi->isLeafItem() )
                {
                    items += fi->url().path();
                }
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
            if( it->childCount() )
            {
                numFiles += getSelectedLeaves(it, list, onlySelected && !it->isSelected(), onlyPlayed);
            }
            else if( it->isSelected() || !onlySelected )
            {
                if( it->type() == MediaItem::TRACK       ||
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
                if( it->isLeafItem() && (!onlyPlayed || it->played()>0) )
                    list->append( it );
            }
        }
    }

    return numFiles;
}


void
MediaView::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() == viewport() || KURLDrag::canDecode( e ) );
}


void
MediaView::contentsDropEvent( QDropEvent *e )
{
    const QPoint p = contentsToViewport( e->pos() );
    MediaItem *item = dynamic_cast<MediaItem *>(itemAt( p ));

    if(e->source() == viewport() || e->source() == this)
    {
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
        else if( item->type() == MediaItem::DIRECTORY )
        {
            debug() << "Dropping items into directory: " << item->text(0) << endl;
            QPtrList<MediaItem> items;
            getSelectedLeaves( 0, &items );
            m_device->addToDirectory( item, items );
        }
    }
    else
    {
        KURL::List list;
        if ( KURLDrag::decode( e, list ) )
        {
            KURL::List::ConstIterator it = list.begin();
            for ( ; it != list.end(); ++it )
            {
                MediaBrowser::queue()->addURL( *it );
            }
        }
    }

    MediaBrowser::queue()->URLsAdded();
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

    if ( !MediaBrowser::instance()->m_connectButton->isOn() && !childCount() )
    {
        QPainter p( viewport() );

        QSimpleRichText t( i18n(
                "<div align=center>"
                  "<h3>Media Device Browser</h3>"
                  "Click the Connect button to access your mounted media device. "
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
    m_device->rmbPressed( this, item, p, i );
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
MediaBrowser::mediumAdded( const Medium *medium, QString name )
{
    debug() << "mediumAdded: " << (medium? medium->properties():"null") << endl;
    KConfig *config = amaroK::config( "MediaBrowser" );
    KGuiItem ignoreButton( "Ignore" );
    MediumPluginChooser *mpc;
    QString handler;
    if( medium )
    {
        handler = config->readEntry( medium->id() );
        if ( !handler.isEmpty() )
            debug() << "handler = " << handler << endl;
        else
        {
            debug() << "handler null for " << medium->id() << endl;
            mpc = new MediumPluginChooser( medium, ignoreButton );
            //mpc->show();
            //mpc->raise();
        }
        debug() << "label=" << medium->label() << endl;
        debug() << "supported by " << m_pluginSupports[medium->label()] << endl;
        MediaDevice *device = loadDevicePlugin( m_pluginSupports[medium->label()] );
        if( device )
        {
            device->m_uniqueId = medium->name();
            addDevice( device );
            if( m_currentDevice == m_devices.begin()
                    || m_currentDevice == m_devices.end() )
                activateDevice( m_devices.count()-1 );
        }
    }
}

void
MediaBrowser::mediumChanged( const Medium *medium, QString name )
{
    debug() << "mediumChanged: " << (medium? medium->properties():"null") << endl;
    if( medium )
    {
        for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
                it != m_devices.end();
                it++ )
        {
            if( (*it)->uniqueId() == medium->name() )
            {
                debug() << "changing state for " << medium->name() << endl;
                if( (*it)->isConnected() && !medium->isMounted() )
                {
                    amaroK::StatusBar::instance()->longMessage(
                            i18n( "The device %1 was unmounted before it was synchronized. "
                                "In order to avoid data loss, press the \"Disconnect\" button "
                                "before unmounting the device." ).arg( name ),
                            KDE::StatusBar::Warning );
                    removeDevice( *it );
                }
                break;
            }
        }
    }
}

void
MediaBrowser::mediumRemoved( const Medium *medium, QString name )
{
    debug() << "mediumRemoved: " << (medium? medium->properties():"null") << endl;
    if( medium )
    {
        for( QValueList<MediaDevice *>::iterator it = m_devices.begin();
                it != m_devices.end();
                it++ )
        {
            if( (*it)->uniqueId() == medium->name() )
            {
                debug() << "removing " << medium->name() << endl;
                if( (*it)->isConnected() )
                {
                    amaroK::StatusBar::instance()->longMessage(
                            i18n( "The device %1 was removed before it was synchronized. "
                                "In order to avoid data loss, press the \"Disconnect\" button "
                                "before disconnecting the device." ).arg( name ),
                            KDE::StatusBar::Warning );
                }
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

    QString query = "[X-KDE-amaroK-plugintype] == 'mediadevice' and [X-KDE-amaroK-name] == '%1'";
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

void
MediaBrowser::config()
{
    KDialogBase dialog( this, 0, false );
    kapp->setTopWidget( &dialog );
    dialog.setCaption( kapp->makeStdCaption( i18n("Configure Media Device") ) );
    dialog.showButtonApply( false );
    QVBox *box = dialog.makeVBoxMainWidget();

    QComboBox *devices = new QComboBox( box );
    int index = 0;
    for( QMap<QString,QString>::iterator it = m_pluginAmarokName.begin();
            it != m_pluginAmarokName.end();
            ++it )
    {
        devices->insertItem( *it );
        if( *it == m_pluginAmarokName[currentDevice()->deviceType()] )
        {
            devices->setCurrentItem( index );
        }
        index++;
    }

    QLabel    *mntpntLabel = 0, *mntLabel = 0, *umntLabel = 0;
    QLineEdit *mntpnt      = 0, *mntcmd   = 0, *umntcmd   = 0;
    QCheckBox *syncStats   = 0, *autoDeletePodcasts = 0;

    if( currentDevice()->m_requireMount )
    {
        mntpntLabel = new QLabel( box );
        mntpntLabel->setText( i18n( "&Mount point:" ) );
        mntpnt = new QLineEdit( currentDevice()->m_mntpnt, box );
        mntpntLabel->setBuddy( mntpnt );
        QToolTip::add( mntpnt, i18n( "Set the mount point of your device here, when empty autodetection is tried." ) );

        mntLabel = new QLabel( box );
        mntLabel->setText( i18n( "&Mount command:" ) );
        mntcmd = new QLineEdit( currentDevice()->m_mntcmd, box );
        mntLabel->setBuddy( mntcmd );
        QToolTip::add( mntcmd, i18n( "Set the command to mount your device here, empty commands are not executed." ) );

        umntLabel = new QLabel( box );
        umntLabel->setText( i18n( "&Unmount command:" ) );
        umntcmd = new QLineEdit( currentDevice()->m_umntcmd, box );
        umntLabel->setBuddy( umntcmd );
        QToolTip::add( umntcmd, i18n( "Set the command to unmount your device here, empty commands are not executed." ) );
    }

    if( currentDevice()->m_hasPodcast )
    {
        autoDeletePodcasts = new QCheckBox( box );
        autoDeletePodcasts->setText( i18n( "&Automatically delete podcasts" ) );
        QToolTip::add( autoDeletePodcasts, i18n( "Automatically delete podcast shows already played on connect" ) );
        autoDeletePodcasts->setChecked( currentDevice()->m_autoDeletePodcasts );
    }

    if( currentDevice()->m_hasStats )
    {
        syncStats = new QCheckBox( box );
        syncStats->setText( i18n( "&Synchronize with amaroK statistics" ) );
        QToolTip::add( syncStats, i18n( "Synchronize with amaroK statistics and submit tracks played to last.fm" ) );
        syncStats->setChecked( currentDevice()->m_syncStats );
    }

    if ( dialog.exec() != QDialog::Rejected )
    {
        if( currentDevice()->m_requireMount )
        {
            currentDevice()->setMountPoint( mntpnt->text() );
            currentDevice()->setMountCommand( mntcmd->text() );
            currentDevice()->setUmountCommand( umntcmd->text() );
        }

        if( currentDevice()->m_hasPodcast )
            currentDevice()->setAutoDeletePodcasts( autoDeletePodcasts->isChecked() );

        if( currentDevice()->m_hasStats )
            currentDevice()->setSyncStats( syncStats->isChecked() );

        AmarokConfig::setDeviceType( m_pluginName[devices->currentText()] );
        if( m_pluginName[devices->currentText()] != currentDevice()->deviceType() )
        {
            QString msg = i18n( "The requested media device could not be loaded" );

            amaroK::StatusBar::instance()->shortMessage( msg );
        }

        if( currentDevice()->m_hasPlaylists )
            m_playlistButton->show();
        else
            m_playlistButton->hide();
    }
}

QString
MediaBrowser::prettySize( unsigned long size )
{
    if(size < 1000)
        return QString("%1 KB").arg(size);
    else if(size < 10*1024)
        return QString("%1.%2 MB").arg(size/1024).arg((size%1024)*10/1024);
    else if(size < 1000*1024)
        return QString("%1 MB").arg(size/(1024));
    else if(size < 10*1024*1024)
        return QString("%1.%2 GB").arg(size/(1024*1024)).arg((size%(1024*1024))*10/(1024*1024));
    else
        return QString("%1 GB").arg(size/(1024*1024));
}

void
MediaBrowser::updateButtons()
{
    if( m_disconnectButton )
        m_disconnectButton->setEnabled( currentDevice()
                && currentDevice()->isConnected() );
    if( m_transferButton )
        m_transferButton->setEnabled( currentDevice()
                && currentDevice()->isConnected()
                && m_queue->childCount() > 0 );
}

void
MediaBrowser::updateStats()
{
    if( !m_stats )
        return;

    QString text = i18n( "1 track in queue", "%n tracks in queue", m_queue->childCount() );
    if(m_queue->childCount() > 0)
    {
        text += " (" + prettySize( m_queue->totalSize() ) + ")";
    }

    unsigned long total, avail;
    if(currentDevice() && currentDevice()->getCapacity(&total, &avail))
    {
        text += " - " + i18n( "%1 of %2 available" ).arg( prettySize( avail ) ).arg( prettySize( total ) );

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
}


bool
MediaView::setFilter( const QString &filter, MediaItem *parent )
{
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
            visible = it->match(filter);
        }
        else
        {
            if(it->type()==MediaItem::PLAYLISTSROOT || it->type()==MediaItem::PLAYLIST)
            {
                visible = true;
                setFilter(filter, it);
            }
            else
                visible = setFilter(filter, it);
        }
        it->setVisible( visible );
        if(visible)
            childrenVisible = true;
    }

    return childrenVisible;
}

bool
MediaItem::match( const QString &filter ) const
{
    if(filter.isNull() || filter.isEmpty())
        return true;

    if(text(0).lower().contains(filter.lower()))
        return true;

    QListViewItem *p = parent();
    if(p && p->text(0).lower().contains(filter.lower()))
        return true;

    if(p)
    {
        p = p->parent();
        if(p && p->text(0).lower().contains(filter.lower()))
            return true;
    }

    return false;
}


MediaDevice::MediaDevice()
    : amaroK::Plugin()
    , m_parent( NULL )
    , m_listview( NULL )
    , m_wait( false )
    , m_requireMount( false )
    , m_hasPodcast( false )
    , m_hasStats( false )
    , m_hasPlaylists( false )
    , m_cancelled( false )
    , m_transferring( false )
    , m_transferredItem( 0 )
    , m_playlistItem( 0 )
    , m_podcastItem( 0 )
    , m_invisibleItem( 0 )
    , m_staleItem( 0 )
    , m_orphanedItem( 0 )
{
    sysProc = new KShellProcess(); Q_CHECK_PTR(sysProc);

    m_mntpnt = AmarokConfig::mountPoint();
    m_mntcmd = AmarokConfig::mountCommand();
    m_umntcmd = AmarokConfig::umountCommand();
    m_autoDeletePodcasts = AmarokConfig::autoDeletePodcasts();
    m_syncStats = AmarokConfig::syncStats();
}

void MediaDevice::init( MediaBrowser* parent )
{
    m_parent = parent;
    m_listview = new MediaView( m_parent->m_views, this );
    m_listview->hide();
}

MediaDevice::~MediaDevice()
{
    delete sysProc;
}

MediaView *
MediaDevice::view()
{
    return m_listview;
}

void
MediaDevice::giveItemsToView()
{
    for( QListViewItem *it = m_rootItems.take();
            it;
            it = m_rootItems.take() )
    {
        m_listview->insertItem( it );
    }
}

void
MediaDevice::takeItemsFromView()
{
    QListViewItem *next = 0;
    for( QListViewItem *it = m_listview->firstChild();
            it;
            it = next )
    {
        next = it->nextSibling();
        m_rootItems.append( it );
        m_listview->takeItem( it );
    }
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
MediaQueue::addURL( const KURL& url, MetaBundle *bundle, PodcastInfo *podcastInfo, const QString &playlistName )
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
            addURL( (*it).url(), 0, 0, name );
        }
        return;
    }
    else if( url.protocol() == "album"
            || url.protocol() == "compilation"
            || url.protocol() == "fetchcover" )
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

    if(!bundle)
        bundle = new MetaBundle( url );

    else
    {
        MediaItem* item = new MediaItem( this, lastItem() );
        item->setExpandable( false );
        item->setDropEnabled( true );
        item->setUrl( url.path() );
        item->m_bundle = bundle;
        item->m_podcastInfo = podcastInfo;
        if( podcastInfo )
            item->m_type = MediaItem::PODCASTITEM;
        item->m_playlistName = playlistName;

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

        m_parent->updateStats();
        m_parent->updateButtons();
        m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() + 1 );
        itemCountChanged();
    }
}

void
MediaQueue::addURLs( const KURL::List urls, const QString &playlistName )
{
    KURL::List::ConstIterator it = urls.begin();
    for ( ; it != urls.end(); ++it )
        addURL( *it, 0, false, playlistName );

    URLsAdded();
}

void
MediaQueue::URLsAdded()
{
    if( m_parent->currentDevice()
            && m_parent->currentDevice()->isConnected()
            && m_parent->currentDevice()->asynchronousTransfer()
            && !m_parent->currentDevice()->isTransferring() )
        m_parent->currentDevice()->transferFiles();
}


void MediaDevice::setMountPoint(const QString & mntpnt)
{
    AmarokConfig::setMountPoint( mntpnt );
    m_mntpnt = mntpnt;          //Update mount point
}

void MediaDevice::setMountCommand(const QString & mnt)
{
    AmarokConfig::setMountCommand( mnt );
    m_mntcmd = mnt;             //Update for mount()
}

void MediaDevice::setUmountCommand(const QString & umnt)
{
    AmarokConfig::setUmountCommand( umnt );
    m_umntcmd = umnt;        //Update for umount()
}

void MediaDevice::setAutoDeletePodcasts( bool value )
{
    AmarokConfig::setAutoDeletePodcasts( value );
    m_autoDeletePodcasts = value; //Update
}

void MediaDevice::setSyncStats( bool value )
{
    AmarokConfig::setSyncStats( value );
    m_syncStats = value; //Update
}

int MediaDevice::mount()
{
    debug() << "mounting" << endl;
    QString cmdS=m_mntcmd;

    debug() << "attempting mount with command: [" << cmdS << "]" << endl;
    int e=sysCall(cmdS);
    debug() << "mount-cmd: e=" << e << endl;
    return e;
}

int MediaDevice::umount()
{
    debug() << "umounting" << endl;
    QString cmdS=m_umntcmd;

    debug() << "attempting umount with command: [" << cmdS << "]" << endl;
    int e=sysCall(cmdS);
    debug() << "umount-cmd: e=" << e << endl;

    return e;
}

int MediaDevice::sysCall(const QString & command)
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
    m_cancelled = true;
    cancelTransfer();
}

void
MediaDevice::connectClicked( bool silent )
{
    // it was just clicked, so isOn() == true.
    connectDevice( silent );

    if( isConnected() )
    {
        m_parent->m_connectButton->setOn( true );

        // delete podcasts already played
        if( m_autoDeletePodcasts && m_podcastItem )
        {
            QPtrList<MediaItem> list;
            //NOTE we assume that currentItem is the main target
            int numFiles  = m_listview->getSelectedLeaves( m_podcastItem, &list, false /* not only selected */, true /* only played */ );

            if(numFiles > 0)
            {
                m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", numFiles ) );

                setProgress( 0, numFiles );

                lockDevice(true);

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
                unlockDevice();

                QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
                m_parent->updateStats();
            }
        }
    }

    m_parent->updateButtons();
    m_parent->updateStats();
}


void
MediaDevice::disconnectClicked( bool silent )
{
    if ( m_parent->m_queue->childCount() != 0 && isConnected() )
    {
        KGuiItem transfer = KGuiItem(i18n("&Transfer"),"rebuild");
        KGuiItem disconnect = KGuiItem(i18n("Disconnect immediately"),"connect_no");
        int button = KMessageBox::warningYesNo( m_parent,
                i18n( "There are tracks queued for transfer."
                    " Would you like to transfer them before disconnecting?"),
                i18n( "Media Device Browser" ),
                transfer, disconnect);

        if ( button == KMessageBox::Yes )
        {
            transferFiles();
        }
    }

    m_parent->m_transferButton->setEnabled( false );
    disconnectDevice();

    m_parent->updateButtons();
    m_parent->updateStats();
}

bool
MediaDevice::connectDevice( bool silent )
{
    if ( !m_mntcmd.isEmpty() && m_requireMount )
    {
        mount();
    }
    openDevice( silent );
    m_parent->updateStats();
    m_parent->updateButtons();

    if( !isConnected() )
        return false;

    if( m_syncStats )
    {
        syncStatsFromDevice( 0 );
    }

    updateRootItems();

    return true;
}

bool
MediaDevice::disconnectDevice()
{
    if( m_syncStats )
    {
        syncStatsToDevice();
    }

    closeDevice();
    m_parent->updateStats();
    m_parent->m_connectButton->setOn( false );

    if( !m_requireMount || (!m_umntcmd.isEmpty() && umount() == 0) ) // umount was successful or no umount needed
    {
        amaroK::StatusBar::instance()->shortMessage( i18n( "Device successfully disconnected" ) );
        return true;
    }
    else
    {
        amaroK::StatusBar::instance()->longMessage( i18n( "Please unmount device before removal." ),
                KDE::StatusBar::Information );
        return false;
    }
}

void
MediaDevice::syncStatsFromDevice( MediaItem *root )
{
    MediaItem *it = static_cast<MediaItem *>( m_listview->firstChild() );
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
                MetaBundle *bundle = it->bundle();
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

                    // increase amaroK playcount
                    QString url = CollectionDB::instance()->getURL( *bundle );
                    if( url != QString::null )
                    {
                        QDateTime t = it->playTime();
                        CollectionDB::instance()->addSongPercentage( url, 100, t.isValid() ? &t : 0 );
                        debug() << "played " << url << endl;
                    }
                }

                if( it->ratingChanged() )
                {
                    // copy rating from media device to amaroK
                    QString url = CollectionDB::instance()->getURL( *bundle );
                    if( url != QString::null )
                    {
                        CollectionDB::instance()->setSongRating( url, it->rating()/20 );
                        debug() << "rating changed " << url << ": " << it->rating()/20 << endl;
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
    MediaItem *it = static_cast<MediaItem *>( m_listview->firstChild() );
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
                MetaBundle *bundle = it->bundle();
                QString url = CollectionDB::instance()->getURL( *bundle );

                if( url != QString::null )
                {
                    // copy amaroK rating to device
                    int rating = CollectionDB::instance()->getSongRating( url )*20;
                    it->setRating( rating );
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
MediaDevice::transferFiles()
{
    m_transferring = true;
    m_parent->m_transferButton->setEnabled( false );

    setProgress( 0, m_parent->m_queue->childCount() );

    // ok, let's copy the stuff to the device
    lockDevice( true );

    MediaItem *playlist = 0;
    if(m_playlistItem && m_parent->m_playlistButton->isOn())
    {
        QString name = i18n("New amaroK additions");
        playlist = m_playlistItem->findItem( name );
        if(!playlist)
        {
            QPtrList<MediaItem> items;
            playlist = newPlaylist(name, m_playlistItem, items);
        }
    }

    MediaItem *after = 0; // item after which to insert into playlist
    // iterate through items
    while( (m_transferredItem = static_cast<MediaItem *>(m_parent->m_queue->firstChild())) != 0 )
    {
        debug() << "Transferring: " << m_transferredItem->url().path() << endl;

        MetaBundle *bundle = m_transferredItem->bundle();
        if(!bundle)
        {
            m_transferredItem->m_bundle = new MetaBundle( m_transferredItem->url() );
            bundle = m_transferredItem->m_bundle;
        }

        MediaItem *item = trackExists( *bundle );

        if( item && m_transferredItem->m_playlistName == QString::null )
        {
            amaroK::StatusBar::instance()->longMessage( i18n( "Track already exists on media device: %1" ).
                    arg( m_transferredItem->url().url() ),
                    KDE::StatusBar::Sorry );
            continue;
        }
        else
        {
            if( !isPlayable( *bundle ) )
            {
                amaroK::StatusBar::instance()->longMessage( i18n( "Track is not playable on media device: %1" ).arg( item->url().path() ),
                        KDE::StatusBar::Sorry );
                continue;
            }
            item = copyTrackToDevice( *bundle, m_transferredItem->podcastInfo() );
        }

        if( !item )
            break;

        int rating = CollectionDB::instance()->getSongRating( bundle->url().path() ) * 20;
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

        delete m_transferredItem;
        m_transferredItem = 0;
        m_parent->m_queue->itemCountChanged();
    }
    synchronizeDevice();
    unlockDevice();
    fileTransferFinished();

    m_parent->updateButtons();
    m_transferring = false;
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
    m_parent->m_transferButton->setDisabled( true );
    m_wait = false;
}


int
MediaDevice::deleteFromDevice(MediaItem *item, bool onlyPlayed, bool recursing)
{
    MediaItem* fi = item;
    int count = 0;

    if ( !recursing )
    {
        QPtrList<MediaItem> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_listview->getSelectedLeaves(item, &list, true /* only selected */, onlyPlayed);

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
                return 0;
            }

            if(!isTransferring())
            {
                setProgress( 0, numFiles );
            }

        }
        // don't return if numFiles==0: playlist items might be to delete

        lockDevice( true );
        if( !fi )
            fi = static_cast<MediaItem*>(m_listview->firstChild());
    }

    while( fi )
    {
        MediaItem *next = static_cast<MediaItem*>(fi->nextSibling());

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
        unlockDevice();

        if(!isTransferring())
        {
            QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
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
        it = static_cast<MediaItem *>(m_listview->firstChild());
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
    transferlist.setAttribute( "product", "amaroK" );
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
                && item->podcastInfo())
        {
            QDomElement attr = newdoc.createElement( "PodcastDescription" );
            QDomText t = newdoc.createTextNode( item->podcastInfo()->description );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastAuthor" );
            t = newdoc.createTextNode( item->podcastInfo()->author );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastRSS" );
            t = newdoc.createTextNode( item->podcastInfo()->rss );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastWebpage" );
            t = newdoc.createTextNode( item->podcastInfo()->webpage );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastURL" );
            t = newdoc.createTextNode( item->podcastInfo()->url );
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
        debug() << "failed to restore media device transfer list" << endl;
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
                "The XML in the transferlist was invalid. Please report this as a bug to the amaroK "
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

        PodcastInfo *info = NULL;
        if(elem.hasAttribute( "podcast" ))
        {
            info = new PodcastInfo();
        }

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
            else if(info && node.nodeName() == "PodcastDescription" )
                info->description = node.firstChild().toText().nodeValue();
            else if(info && node.nodeName() == "PodcastAuthor" )
                info->author = node.firstChild().toText().nodeValue();
            else if(info && node.nodeName() == "PodcastRSS" )
                info->rss = node.firstChild().toText().nodeValue();
            else if(info && node.nodeName() == "PodcastWebpage" )
                info->webpage = node.firstChild().toText().nodeValue();
            else if(info && node.nodeName() == "PodcastURL" )
                info->url = node.firstChild().toText().nodeValue();
        }

        QString playlist = elem.attribute( "playlist" );
        addURL( url, bundle, info, playlist );
    }

    //URLsAdded();
}

bool
MediaDevice::isPlayable( const MetaBundle &bundle )
{
    if( supportedFiletypes().isEmpty() )
        return true;

    QString type = bundle.url().path().section( ".", -1 ).lower();
    return supportedFiletypes().contains( type );
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

    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && KURLDrag::canDecode( e ) );
}


void
MediaQueue::dropEvent( QDropEvent *e )
{
    KListView::dropEvent( e );

    KURL::List list;
    if ( KURLDrag::decode( e, list ) )
        addURLs( list );
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

    //    const QPoint p = contentsToViewport( e->pos() );
    //    QListViewItem *item = itemAt( p );
    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && KURLDrag::canDecode( e ) );

#if 0
    const QPoint p = contentsToViewport( e->pos() );
    MediaItem *item = dynamic_cast<MediaItem *>( itemAt( p ) );
    if( item )
        if(p.y() - itemRect( item ).top() < (item->height()/2))
            item = dynamic_cast<MediaItem *>(item->itemAbove());
#endif
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

unsigned
MediaQueue::totalSize() const
{
    unsigned total = 0;
    for( QListViewItem *it = firstChild();
            it;
            it = it->nextSibling())
    {
        MediaItem *item = static_cast<MediaItem *>(it);

        if(!m_parent->currentDevice()->isConnected() || !m_parent->currentDevice()->trackExists(*item->bundle()) )
            total += (item->size()+1023)/1024;
    }

    return total;
}

void
MediaQueue::removeSelected()
{
    QPtrList<QListViewItem>  selected = selectedItems();

    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        if( !m_parent->currentDevice()->isTransferring() || item != m_parent->currentDevice()->transferredItem() )
        {
            delete item;
            if( m_parent->currentDevice()->isTransferring() )
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

    enum Actions { REMOVE_SELECTED, CLEAR_ALL };

    if( item )
        menu.insertItem( SmallIconSet( "edittrash" ), i18n( "&Remove From Queue" ), REMOVE_SELECTED );

    menu.insertItem( SmallIconSet( "view_remove" ), i18n( "&Clear Queue" ), CLEAR_ALL );

    switch( menu.exec( point ) )
    {
        case REMOVE_SELECTED:
            removeSelected();
            break;
        case CLEAR_ALL:
            clearItems();
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
