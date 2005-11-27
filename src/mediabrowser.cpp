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
#include "colorgenerator.h"
#include "debug.h"
#include "mediabrowser.h"
#include "metabundle.h"
#include "statusbar.h"

#ifdef HAVE_LIBGPOD
#include "gpodmediadevice/gpodmediadevice.h"
#endif

#ifdef HAVE_IFP
#include "ifpmediadevice/ifpmediadevice.h"
#endif

#include <qdatetime.h>
#include <qgroupbox.h>
#include <qimage.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qpainter.h>
#include <qregexp.h>
#include <qsimplerichtext.h>
#include <qtimer.h>
#include <qtooltip.h>       //QToolTip::add()
#include <qfileinfo.h>
#include <qdir.h>
#include <qdom.h>

#include <kapplication.h> //kapp
#include <kdirlister.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmountpoint.h>
#include <kpopupmenu.h>
#include <kprogress.h>
#include <kpushbutton.h>
#include <krun.h>
#include <kstandarddirs.h> //locate file
#include <ktabbar.h>
#include <ktempfile.h>
#include <ktoolbarbutton.h> //ctor
#include <kurldrag.h>       //dragObject()
#include <kprocess.h>


MediaDevice *MediaDevice::s_instance = 0;

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

bool MediaBrowser::isAvailable() //static
{
    // perhaps the user should configure if he wants to use a media device?
#if defined(HAVE_LIBGPOD)
    return true;
#elif defined(HAVE_IFP)
    return true;
#else
    return false;
#endif
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
    DummyMediaDevice( MediaDeviceView *view, MediaDeviceList *list ) : MediaDevice( view, list ) {}
    virtual ~DummyMediaDevice() {}
    virtual bool isConnected() { return false; }
    virtual void addToPlaylist(MediaItem*, MediaItem*, QPtrList<MediaItem>) {}
    virtual void addToDirectory(MediaItem*, QPtrList<MediaItem>) {}
    virtual MediaItem* newDirectory(const QString&, MediaItem*) { return 0; }
    virtual MediaItem* newPlaylist(const QString&, MediaItem*, QPtrList<MediaItem>) { return 0; }
    virtual MediaItem* trackExists(const MetaBundle&) { return 0; }
    virtual void lockDevice(bool) {}
    virtual void unlockDevice() {}
    virtual bool openDevice(bool) { return false; }
    virtual bool closeDevice() { return false; }
    virtual void synchronizeDevice() {}
    virtual MediaItem* copyTrackToDevice(const MetaBundle&, bool) { return 0; }
    virtual bool deleteItemFromDevice(MediaItem*, bool) { return false; }
    virtual bool getCapacity( unsigned long *, unsigned long * ) { return false; }
};


MediaBrowser::MediaBrowser( const char *name )
        : QVBox( 0, name )
        , m_timer( new QTimer( this ) )
{
    KIconLoader iconLoader;
    MediaItem::s_pixUnknown = new QPixmap(iconLoader.loadIcon( "unknown", KIcon::Toolbar, KIcon::SizeSmall ));
    MediaItem::s_pixTrack = new QPixmap(iconLoader.loadIcon( "player_playlist_2", KIcon::Toolbar, KIcon::SizeSmall ));
    MediaItem::s_pixFile = new QPixmap(iconLoader.loadIcon( "sound", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixPodcast = new QPixmap(iconLoader.loadIcon( "favorites", KIcon::Toolbar, KIcon::SizeSmall ) );
    MediaItem::s_pixPlaylist = new QPixmap(iconLoader.loadIcon( "player_playlist_2", KIcon::Toolbar, KIcon::SizeSmall ) );
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

    m_view = new MediaDeviceView( this );

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_searchEdit, SIGNAL( returnPressed() ), SLOT( slotSetFilter() ) );

    setFocusProxy( m_view ); //default object to get focus
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

    m_view->setFilter( m_searchEdit->text() );
}

MediaBrowser::~MediaBrowser()
{
    delete m_view;
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
}

void
MediaItem::init()
{
    m_bundle=0;
    m_order=0;
    m_type=UNKNOWN;
    m_playlistName=QString::null;
    setExpandable( false );
    setDragEnabled( true );
    setDropEnabled( true );
    m_size=-1;
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
        case PODCASTCHANNEL:
            setPixmap(0, *s_pixPodcast);
            break;
        case PLAYLISTSROOT:
        case PLAYLIST:
            setPixmap(0, *s_pixPlaylist);
            setDropEnabled(true);
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
MediaItem::findItem( const QString &key ) const
{
    for(MediaItem *it = dynamic_cast<MediaItem *>(firstChild());
            it;
            it = dynamic_cast<MediaItem *>(it->nextSibling()))
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


MediaDeviceList::MediaDeviceList( MediaDeviceView* parent )
    : KListView( parent )
    , m_parent( parent )
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

    addColumn( i18n( "Remote Media" ) );

    connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );

    connect( this, SIGNAL( itemRenamed( QListViewItem* ) ),
             this,   SLOT( renameItem( QListViewItem* ) ) );

    connect( this, SIGNAL( expanded( QListViewItem* ) ),
             this,   SLOT( slotExpand( QListViewItem* ) ) );
}

void
MediaDeviceList::renameItem( QListViewItem *item )
{
    if(m_parent && m_parent->m_device)
        m_parent->m_device->renameItem( item );
}

void
MediaDeviceList::slotExpand( QListViewItem *item )
{
    if(m_parent && m_parent->m_device)
        m_parent->m_device->expandItem( item );
}


MediaDeviceList::~MediaDeviceList()
{}


void
MediaDeviceList::startDrag()
{
    KURL::List urls = nodeBuildDragList( 0 );
    debug() << urls.first().path() << endl;
    KURLDrag* d = new KURLDrag( urls, this );
    d->dragCopy();
}


KURL::List
MediaDeviceList::nodeBuildDragList( MediaItem* item, bool onlySelected )
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
MediaDeviceList::getSelectedLeaves( MediaItem *parent, QPtrList<MediaItem> *list, bool onlySelected, bool onlyPlayed )
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
MediaDeviceList::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() == viewport() || KURLDrag::canDecode( e ) );
}


void
MediaDeviceList::contentsDropEvent( QDropEvent *e )
{
    if(e->source() == viewport() || e->source() == this)
    {
        const QPoint p = contentsToViewport( e->pos() );
        MediaItem *item = dynamic_cast<MediaItem *>(itemAt( p ));
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
            m_parent->m_device->addToPlaylist( list, after, items );
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
            m_parent->m_device->addToPlaylist( list, after, items );
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
            MediaItem *pl = m_parent->m_device->newPlaylist(name, item, items);
            ensureItemVisible(pl);
            rename(pl, 0);
        }
        else if( item->type() == MediaItem::DIRECTORY )
        {
            debug() << "Dropping items into directory: " << item->text(0) << endl;
            QPtrList<MediaItem> items;
            getSelectedLeaves( 0, &items );
            m_parent->m_device->addToDirectory( item, items );
        }
    }
    else
    {

        KURL::List list;
        if ( KURLDrag::decode( e, list ) )
        {
            KURL::List::Iterator it = list.begin();
            for ( ; it != list.end(); ++it )
            {
                MediaDevice::instance()->addURL( *it );
            }
        }
    }

    MediaDevice::instance()->URLsAdded();
}


void
MediaDeviceList::contentsDragMoveEvent( QDragMoveEvent *e )
{
//    const QPoint p = contentsToViewport( e->pos() );
//    QListViewItem *item = itemAt( p );
    e->accept( e->source() == viewport()
            || e->source() == this
            || (e->source() != m_parent
                && e->source() != m_parent->m_device->m_transferList
                && e->source() != m_parent->m_device->m_transferList->viewport()
                && KURLDrag::canDecode( e )) );

}


void
MediaDeviceList::viewportPaintEvent( QPaintEvent *e )
{
    KListView::viewportPaintEvent( e );

    // Superimpose bubble help:

    if ( !m_parent->m_connectButton->isOn() && !childCount() )
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
MediaDeviceList::rmbPressed( QListViewItem *item, const QPoint &p, int i )
{
    m_parent->m_device->rmbPressed( this, item, p, i );
}

MediaItem *
MediaDeviceList::newDirectory( MediaItem *parent )
{
    bool ok;
    const QString name = KInputDialog::getText(i18n("Add Directory"), i18n("Directory Name:"), QString::null, &ok, this);

    if( ok && !name.isEmpty() )
    {
        return m_parent->m_device->newDirectory( name, parent );
    }

    return 0;
}

MediaDeviceView::MediaDeviceView( MediaBrowser* parent )
    : QVBox( parent )
    , m_stats( 0 )
    , m_device( 0 )
    , m_deviceList( new MediaDeviceList( this ) )
    , m_parent( parent )
{
#if defined(HAVE_LIBGPOD)
    debug() << "Loading iPod device!" << endl;
    m_device = new GpodMediaDevice( this, m_deviceList );
    m_device->setDeviceType( MediaDevice::IPOD );
    m_device->setRequireMount( true );
#elif defined(HAVE_IFP)
    debug() << "Loading IFP device!" << endl;
    m_device = new IfpMediaDevice( this, m_deviceList );
    m_device->setDeviceType( MediaDevice::IFP );
    m_device->setRequireMount( false );
#else
    debug() << "Loading dummy device!" << endl;
    m_device = new DummyMediaDevice( this, m_deviceList );
    m_device->setDeviceType( MediaDevice::DUMMY );
#endif
    m_progress = new KProgress( this );

    QHBox* hb = new QHBox( this );
    hb->setSpacing( 1 );
    m_connectButton  = new KPushButton( SmallIconSet( "connect_creating" ), i18n( "Connect"), hb );
    m_transferButton = new KPushButton( SmallIconSet( "rebuild" ), i18n( "Transfer" ), hb );

    m_playlistButton = new KPushButton( KGuiItem( QString::null, "player_playlist_2" ), hb );
    m_playlistButton->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
    m_playlistButton->setToggleButton( true );
    if( m_device->deviceType() == MediaDevice::IFP )
        m_playlistButton->hide();

    m_configButton   = new KPushButton( KGuiItem( QString::null, "configure" ), hb );
    m_configButton->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred ); // too big!

    m_stats = new SpaceLabel(this);
    updateStats();

    QToolTip::add( m_connectButton,  i18n( "Connect media device" ) );
    QToolTip::add( m_transferButton, i18n( "Transfer tracks to media device" ) );
    QToolTip::add( m_playlistButton, i18n( "Append transferred items to playlist \"New amaroK additions\"" ) );
    QToolTip::add( m_configButton,   i18n( "Configure media device" ) );

    m_connectButton->setToggleButton( true );
    m_transferButton->setDisabled( true );

    m_progress->setFixedHeight( m_transferButton->sizeHint().height() );
    m_progress->hide();

    connect( m_connectButton,  SIGNAL( clicked() ), MediaDevice::instance(), SLOT( connectDevice() ) );
    connect( m_transferButton, SIGNAL( clicked() ), MediaDevice::instance(), SLOT( transferFiles() ) );
    connect( m_configButton,   SIGNAL( clicked() ), MediaDevice::instance(), SLOT( config() ) );
    connect( m_device->m_transferList, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( slotShowContextMenu( QListViewItem*, const QPoint&, int ) ) );

    m_device->loadTransferList( amaroK::saveLocation() + "transferlist.xml" );

    if( m_device->autoConnect() )
    {
        m_connectButton->setOn( true );
        m_device->connectDevice( true );
        m_connectButton->setOn( m_device->isConnected() );
        m_transferButton->setEnabled( m_device->m_transferList->childCount() != 0 );
    }

    setFocusProxy( m_device->m_transferList );
}

QString
MediaDeviceView::prettySize( unsigned long size )
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
MediaDeviceView::updateStats()
{
    if( !m_stats )
        return;

    QString text = i18n( "1 track in queue", "%n tracks in queue", m_device->m_transferList->childCount() );
    if(m_device->m_transferList->childCount() > 0)
    {
        text += " (" + prettySize( m_device->m_transferList->totalSize() ) + ")";
    }

    unsigned long total, avail;
    if(m_device->getCapacity(&total, &avail))
    {
        text += " - " + i18n( "%1 of %2 available" ).arg( prettySize( avail ) ).arg( prettySize( total ) );

        m_stats->m_used = total-avail;
        m_stats->m_total = total;
        m_stats->m_scheduled = m_device->m_transferList->totalSize();
    }
    else
    {
        m_stats->m_used = 0;
        m_stats->m_total = 0;
        m_stats->m_scheduled = m_device->m_transferList->totalSize();
    }

    m_stats->setText(text);
}

void
MediaDeviceView::slotShowContextMenu( QListViewItem* item, const QPoint& point, int )
{
    if( !m_device->m_transferList->childCount() )
        return;

    KPopupMenu menu( this );

    enum Actions { REMOVE_SELECTED, CLEAR_ALL };

    if( item )
        menu.insertItem( SmallIconSet( "edittrash" ), i18n( "&Remove From Queue" ), REMOVE_SELECTED );

    menu.insertItem( SmallIconSet( "view_remove" ), i18n( "&Clear Queue" ), CLEAR_ALL );

    switch( menu.exec( point ) )
    {
        case REMOVE_SELECTED:
            m_device->removeSelected();
            break;
        case CLEAR_ALL:
            m_device->clearItems();
            break;
    }
}


MediaDeviceView::~MediaDeviceView()
{
    m_device->saveTransferList( amaroK::saveLocation() + "transferlist.xml" );

    m_device->closeDevice();

    delete m_deviceList;
    delete m_device;
}

bool
MediaDeviceView::setFilter( const QString &filter, MediaItem *parent )
{
    MediaItem *it;
    if( !parent )
    {
        it = dynamic_cast<MediaItem *>(m_deviceList->firstChild());
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
            visible = match(it, filter);
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
MediaDeviceView::match( const MediaItem *it, const QString &filter )
{
    if(filter.isNull() || filter.isEmpty())
        return true;

    if(it->text(0).lower().contains(filter.lower()))
        return true;

    QListViewItem *p = it->parent();
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


MediaDevice::MediaDevice( MediaDeviceView* parent, MediaDeviceList *listview )
    : m_parent( parent )
    , m_listview( listview )
    , m_wait( false )
    , m_requireMount( false )
    , m_transferring( false )
    , m_transferredItem( 0 )
    , m_transferList( new MediaDeviceTransferList( parent ) )
    , m_playlistItem( 0 )
    , m_podcastItem( 0 )
    , m_invisibleItem( 0 )
    , m_staleItem( 0 )
    , m_orphanedItem( 0 )
{
    s_instance = this;

    sysProc = new KShellProcess(); Q_CHECK_PTR(sysProc);

    m_mntpnt = AmarokConfig::mountPoint();
    m_mntcmd = AmarokConfig::mountCommand();
    m_umntcmd = AmarokConfig::umountCommand();
    m_autoDeletePodcasts = AmarokConfig::autoDeletePodcasts();
    debug() << "auto delete podcasts: " << m_autoDeletePodcasts << endl;
}

MediaDevice::~MediaDevice()
{
    delete m_transferList;
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
MediaDevice::addURL( const KURL& url, MetaBundle *bundle, bool isPodcast, const QString &playlistName )
{
    if(!bundle)
        bundle = new MetaBundle( url );
    if ( !playlistName.isNull() || (!trackExists( *bundle ) && !m_transferList->findPath( url.path() )) )
    {
        MediaItem* item = new MediaItem( m_transferList, m_transferList->lastItem() );
        item->setExpandable( false );
        item->setDropEnabled( true );
        item->setUrl( url.path() );
        item->m_bundle = bundle;
        item->m_playlistName = playlistName;
        if(isPodcast)
            item->m_type = MediaItem::PODCASTITEM;

        QString text = item->bundle()->prettyTitle();
        if(item->type() == MediaItem::PODCASTITEM)
        {
            text += " (" + i18n("Podcast") + ")";
        }
        if( item->m_playlistName != QString::null )
        {
            text += " (" + item->m_playlistName + ")";
        }
        item->setText( 0, text);

        m_parent->updateStats();
        m_parent->m_transferButton->setEnabled( m_parent->m_device->isConnected() || m_parent->m_deviceList->childCount() != 0 );
        m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() + 1 );
    } else
        amaroK::StatusBar::instance()->longMessage( i18n( "Track already exists on media device: %1" ).arg( url.path().local8Bit() ),
                                                    KDE::StatusBar::Sorry );
    m_transferList->itemCountChanged();
}

void
MediaDevice::addURLs( const KURL::List urls, const QString &playlistName )
{
        KURL::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
            addURL( *it, 0, false, playlistName );

        URLsAdded();
}

void
MediaDevice::URLsAdded()
{
    if( isConnected() && asynchronousTransfer() && !isTransferring() )
        transferFiles();
}

void
MediaDevice::clearItems()
{
    m_transferList->clear();
    m_transferList->itemCountChanged();
    if(m_parent)
        m_parent->updateStats();

    if(m_parent && m_parent->m_transferButton)
        m_parent->m_transferButton->setEnabled( false );
}

void
MediaDevice::removeSelected()
{
    QPtrList<QListViewItem>  selected = m_transferList->selectedItems();

    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        if( !isTransferring() || item != transferredItem() )
        {
            delete item;
            if( isTransferring() )
            {
                m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() - 1 );
            }
        }
    }

    m_parent->m_transferButton->setEnabled( m_transferList->childCount() != 0 && isConnected() );
    m_parent->updateStats();
    m_transferList->itemCountChanged();
}

void
MediaDevice::config()
{
    KDialogBase dialog( m_parent, 0, false );
    kapp->setTopWidget( &dialog );
    dialog.setCaption( kapp->makeStdCaption( i18n("Configure Media Device") ) );
    dialog.showButtonApply( false );
    QVBox *box = dialog.makeVBoxMainWidget();

    QLabel *mntpntLabel = NULL, *mntLabel = NULL, *umntLabel = NULL;
    QLineEdit *mntpnt = NULL, *mntcmd = NULL, *umntcmd = NULL;
    if( m_requireMount )
    {
        mntpntLabel = new QLabel( box );
        mntpntLabel->setText( i18n( "&Mount point:" ) );
        mntpnt = new QLineEdit( m_mntpnt, box );
        mntpntLabel->setBuddy( mntpnt );
        QToolTip::add( mntpnt, i18n( "Set the mount point of your device here, when empty autodetection is tried." ) );

        mntLabel = new QLabel( box );
        mntLabel->setText( i18n( "&Mount command:" ) );
        mntcmd = new QLineEdit( m_mntcmd, box );
        mntLabel->setBuddy( mntcmd );
        QToolTip::add( mntcmd, i18n( "Set the command to mount your device here, empty commands are not executed." ) );

        umntLabel = new QLabel( box );
        umntLabel->setText( i18n( "&Unmount command:" ) );
        umntcmd = new QLineEdit( m_umntcmd, box );
        umntLabel->setBuddy( umntcmd );
        QToolTip::add( umntcmd, i18n( "Set the command to unmount your device here, empty commands are not executed." ) );
    }

    QHBox *hbox = new QHBox( box );
    QCheckBox *autoDeletePodcasts = new QCheckBox( hbox );
    QLabel *autoDeleteLabel = new QLabel( hbox );
    autoDeleteLabel->setBuddy( autoDeletePodcasts );
    autoDeleteLabel->setText( i18n( "Automatically delete podcasts" ) );
    QToolTip::add( autoDeletePodcasts, i18n( "Automatically delete podcast shows already played on connect" ) );
    autoDeletePodcasts->setChecked( m_autoDeletePodcasts );

    if ( dialog.exec() != QDialog::Rejected )
    {
        if( m_requireMount )
        {
            setMountPoint( mntpnt->text() );
            setMountCommand( mntcmd->text() );
            setUmountCommand( umntcmd->text() );
        }
        setAutoDeletePodcasts( autoDeletePodcasts->isChecked() );
    }
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
MediaDevice::connectDevice( bool silent )
{
    if ( m_parent->m_connectButton->isOn() )
    {
        if ( !m_mntcmd.isEmpty() && m_requireMount )
        {
            mount();
        }
        openDevice( silent );
        m_parent->updateStats();

        if( isConnected() || m_parent->m_deviceList->childCount() != 0 )
        {
            m_parent->m_connectButton->setOn( true );
            if ( m_transferList->childCount() != 0 )
            {
                m_parent->m_transferButton->setEnabled( true );
                m_parent->m_stats->setText( i18n( "Checking device for duplicate files." ) );
                KURL::List urls;
                MediaItem *next = 0;
                for( MediaItem *cur = static_cast<MediaItem *>(m_transferList->firstChild());
                                cur; cur = next )
                {
                    next = dynamic_cast<MediaItem *>( cur->nextSibling() );
                    if ( cur->m_playlistName == QString::null && trackExists( *cur->bundle() ) )
                    {
                        delete cur;
                        m_transferList->itemCountChanged();
                    }
                }
                m_parent->updateStats();
            }

            // delete podcasts already played
            if( m_autoDeletePodcasts && m_podcastItem )
            {
                QPtrList<MediaItem> list;
                //NOTE we assume that currentItem is the main target
                int numFiles  = m_parent->m_deviceList->getSelectedLeaves(m_podcastItem, &list, false /* not only selected */, true /* only played */ );

                if(numFiles > 0)
                {
                    m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", numFiles ) );

                    setProgress( 0, numFiles );

                    lockDevice(true);

                    deleteItemFromDevice(m_podcastItem, true);

                    synchronizeDevice();
                    unlockDevice();

                    QTimer::singleShot( 1500, m_parent->m_progress, SLOT(hide()) );
                    m_parent->updateStats();
                }
            }

            updateRootItems();
        }
        else
        {
            m_parent->m_connectButton->setOn( false );
        }
    }
    else
    {
        if ( m_transferList->childCount() != 0 && isConnected() )
        {
            KGuiItem transfer = KGuiItem(i18n("&Transfer"),"rebuild");
            KGuiItem disconnect = KGuiItem(i18n("Disconnect immediately"),"connect_no");
            int button = KMessageBox::warningYesNo( m_parent->m_parent,
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

        closeDevice();
        m_parent->updateStats();
        m_parent->m_connectButton->setOn( false );

        if( !m_requireMount || (!m_umntcmd.isEmpty() && umount() == 0) ) // umount was successful or no umount needed
            amaroK::StatusBar::instance()->shortMessage( i18n( "Device successfully disconnected" ) );
        else
            amaroK::StatusBar::instance()->longMessage( i18n( "Please unmount device before removal." ),
                                                        KDE::StatusBar::Information );
    }
}


void
MediaDevice::transferFiles()
{
    m_transferring = true;
    m_parent->m_transferButton->setEnabled( false );

    setProgress( 0, m_transferList->childCount() );

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
    while( (m_transferredItem = static_cast<MediaItem *>(m_transferList->firstChild())) != 0 )
    {
        debug() << "Transferring: " << m_transferredItem->url().path() << endl;

        MetaBundle *bundle = m_transferredItem->bundle();
        if(!bundle)
        {
            m_transferredItem->m_bundle = new MetaBundle( m_transferredItem->url() );
            bundle = m_transferredItem->m_bundle;
        }

        MediaItem *item = trackExists( *bundle );

        if( !item )
            item = copyTrackToDevice( *bundle, m_transferredItem->type() == MediaItem::PODCASTITEM );

        if( !item )
            break;

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
        m_transferList->itemCountChanged();
    }
    synchronizeDevice();
    unlockDevice();
    fileTransferFinished();

    m_parent->m_transferButton->setEnabled( m_transferList->childCount()>0 );
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
    m_parent->m_progress->show();
}

void
MediaDevice::fileTransferFinished()  //SLOT
{
    m_parent->updateStats();
    m_parent->m_progress->hide();
    m_parent->m_transferButton->setDisabled( true );
    m_wait = false;
}


void
MediaDevice::deleteFromDevice(MediaItem *item, bool onlyPlayed, bool recursing)
{
    MediaItem* fi = item;

    if ( !recursing )
    {
        QPtrList<MediaItem> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_parent->m_deviceList->getSelectedLeaves(item, &list, true /* only selected */, onlyPlayed);

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
                return;
            }

            if(!isTransferring())
            {
                setProgress( 0, numFiles );
            }

        }
        // don't return if numFiles==0: playlist items might be to delete

        lockDevice( true );
        if( !fi )
            fi = static_cast<MediaItem*>(m_parent->m_deviceList->firstChild());
    }

    while( fi )
    {
        MediaItem *next = static_cast<MediaItem*>(fi->nextSibling());

        if( fi->isSelected() )
        {
            deleteItemFromDevice(fi, onlyPlayed);
        }
        else
        {
            if( fi->childCount() )
                deleteFromDevice( static_cast<MediaItem*>(fi->firstChild()), onlyPlayed, true );
        }
        m_parent->updateStats();

        fi = next;
    }

    if(!recursing)
    {
        synchronizeDevice();
        unlockDevice();

        if(!isTransferring())
        {
            QTimer::singleShot( 1500, m_parent->m_progress, SLOT(hide()) );
        }
    }
    m_parent->updateStats();
}

void
MediaDevice::saveTransferList( const QString &path )
{
    QFile file( path );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument newdoc;
    QDomElement transferlist = newdoc.createElement( "playlist" );
    transferlist.setAttribute( "product", "amaroK" );
    transferlist.setAttribute( "version", APP_VERSION );
    newdoc.appendChild( transferlist );

    for( const MediaItem *item = static_cast<MediaItem *>( m_transferList->firstChild() );
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
MediaDevice::loadTransferList( const QString& filename )
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

        bool isPodcast = false;
        if(elem.hasAttribute( "podcast" ))
        {
            isPodcast = true;
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
        }

        QString playlist = elem.attribute( "playlist" );
        addURL( url, bundle, isPodcast, playlist );
    }

    //URLsAdded();
}


MediaDeviceTransferList::MediaDeviceTransferList(MediaDeviceView *parent)
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
}

void
MediaDeviceTransferList::dragEnterEvent( QDragEnterEvent *e )
{
    KListView::dragEnterEvent( e );

    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && e->source() != m_parent->m_deviceList
            && e->source() != m_parent->m_deviceList->viewport()
            && KURLDrag::canDecode( e ) );
}


void
MediaDeviceTransferList::dropEvent( QDropEvent *e )
{
    KListView::dropEvent( e );

    KURL::List list;
    if ( KURLDrag::decode( e, list ) )
    {
        KURL::List::Iterator it = list.begin();
        for ( ; it != list.end(); ++it )
        {
            MediaDevice::instance()->addURL( *it );
        }
    }

    MediaDevice::instance()->URLsAdded();
}

void
MediaDeviceTransferList::contentsDragEnterEvent( QDragEnterEvent *e )
{
    KListView::contentsDragEnterEvent( e );

    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && e->source() != m_parent->m_deviceList
            && e->source() != m_parent->m_deviceList->viewport()
            && KURLDrag::canDecode( e ) );
}


void
MediaDeviceTransferList::contentsDropEvent( QDropEvent *e )
{
    KListView::contentsDropEvent( e );

    KURL::List list;
    if ( KURLDrag::decode( e, list ) )
    {
        KURL::List::Iterator it = list.begin();
        for ( ; it != list.end(); ++it )
        {
            MediaDevice::instance()->addURL( *it );
        }
    }

    MediaDevice::instance()->URLsAdded();
}

void
MediaDeviceTransferList::contentsDragMoveEvent( QDragMoveEvent *e )
{
    KListView::contentsDragMoveEvent( e );

    //    const QPoint p = contentsToViewport( e->pos() );
    //    QListViewItem *item = itemAt( p );
    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && e->source() != m_parent->m_deviceList
            && e->source() != m_parent->m_deviceList->viewport()
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
MediaDeviceTransferList::findPath( QString path )
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
MediaDeviceTransferList::totalSize() const
{
    unsigned total = 0;
    for( QListViewItem *it = firstChild();
            it;
            it = it->nextSibling())
    {
        MediaItem *item = static_cast<MediaItem *>(it);

        if(!m_parent->m_device->isConnected() || !m_parent->m_device->trackExists(*item->bundle()) )
            total += (item->size()+1023)/1024;
    }

    return total;
}

void
MediaDeviceTransferList::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Key_Delete )
    {
        m_parent->m_device->removeSelected();
    }
}

void
MediaDeviceTransferList::itemCountChanged()
{
    if( childCount() == 0 )
        hide();
    else if( !isShown() )
        show();
}

#include "mediabrowser.moc"
