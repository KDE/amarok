// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information


#define DEBUG_PREFIX "MediaBrowser"

#include "amarokconfig.h"
#include "browserToolBar.h"
#include "clicklineedit.h"
#include "colorgenerator.h"
#include "debug.h"
#include "k3bexporter.h"
#include "metabundle.h"
#include "playlist.h"      //appendMedia()
#include "statusbar.h"
#include "mediabrowser.h"

#include <qdatetime.h>
#include <qgroupbox.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qregexp.h>
#include <qsimplerichtext.h>
#include <qtimer.h>
#include <qtooltip.h>       //QToolTip::add()
#include <qfileinfo.h>
#include <qdir.h>

#include <kapplication.h> //kapp
#include <kdirlister.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
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

#define escapeIPod(s)   QString(s).replace( "/", "%252f" )


MediaDevice *MediaDevice::s_instance = 0;

bool MediaBrowser::isAvailable() //static
{
    return true;
//    return !KStandardDirs::findExe( "gnupod_addsong.pl" ).isNull()
//           || !KStandardDirs::findExe( "gnupod_addsong" ).isNull();
}


MediaBrowser::MediaBrowser( const char *name )
        : QVBox( 0, name )
{
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

    setFocusProxy( m_view ); //default object to get focus
}

MediaBrowser::~MediaBrowser()
{
    delete m_view;
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
    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );

    addColumn( i18n( "Artist" ) );
    renderView( 0 );

    connect( this, SIGNAL( expanded( QListViewItem* ) ),
             this,   SLOT( renderView( QListViewItem* ) ) );

    connect( this, SIGNAL( collapsed( QListViewItem* ) ),
             this,   SLOT( slotCollapse( QListViewItem* ) ) );

    connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );
}


MediaDeviceList::~MediaDeviceList()
{}


void
MediaDeviceList::renderView( QListViewItem* parent )  //SLOT
{
    if ( parent == 0 )
        clear();

    KIconLoader iconLoader;
    QPixmap pixmap = iconLoader.loadIcon( "usbpendrive_unmount", KIcon::Toolbar, KIcon::SizeSmall );

    QStringList items;
    items = m_parent->m_device->items( parent );

    bool track = ( parent && parent->parent() );
    for ( uint i = 0; i < items.count(); track ? i+=2 : ++i )
    {
        MediaItem* item;
        if ( !parent )
            item = new MediaItem( this );
        else
            item = new MediaItem( parent );

        item->setExpandable( !track );
        item->setDragEnabled( true );
        item->setDropEnabled( true );
        item->setText( 0, items[ i ] );
        item->setPixmap( 0, pixmap );

        if ( track )
            item->setUrl( items[i+1] );
    }
}


void
MediaDeviceList::renderNode( QListViewItem* parent, const KURL& url )  //SLOT
{
//    renderNode( 0, KURL( "ipod:/Artists/" ) );
    KDirLister dl;
    dl.setAutoErrorHandlingEnabled( false, 0 );
    dl.openURL( url, false, true );

    while ( !dl.isFinished() )
        kapp->processEvents( 100 );

    KFileItem* fi;
    KFileItemList fil = dl.items();

    KIconLoader iconLoader;
    QPixmap pixmap = iconLoader.loadIcon( "usbpendrive_unmount", KIcon::Toolbar, KIcon::SizeSmall );
    for ( fi = fil.first(); fi; fi = fil.next() )
    {
        MediaItem* item;
        if ( !parent )
            item = new MediaItem( this );
        else
            item = new MediaItem( parent );

        item->setExpandable( fi->isDir() );
        item->setDragEnabled( true );
        item->setDropEnabled( true );
        item->setUrl( "ipod:" + fi->url().path() );
        item->setText( 0, fi->text() );
        item->setPixmap( 0, pixmap );
    }
}


void
MediaDeviceList::slotCollapse( QListViewItem* item )  //SLOT
{
    DEBUG_FUNC_INFO

    QListViewItem* child = item->firstChild();
    QListViewItem* childTmp;

    //delete all children
    while ( child )
    {
        childTmp = child;
        child = child->nextSibling();
        delete childTmp;
    }
}


void
MediaDeviceList::startDrag()
{
    KURL::List urls = nodeBuildDragList( 0 );
    debug() << urls.first().path() << endl;
    KURLDrag* d = new KURLDrag( urls, this );
    d->dragCopy();
}


KURL::List
MediaDeviceList::nodeBuildDragList( MediaItem* item )
{
    KURL::List items;
    MediaItem* fi;

    if ( !item )
        fi = (MediaItem*)firstChild();
    else
        fi = item;

    while ( fi )
    {
        if ( fi->isSelected() )
        {
            switch ( fi->depth() )
            {
                case 0:
                    items += m_parent->m_device->songsByArtist( fi->text( 0 ) );
                    break;
                case 1:
                    items += m_parent->m_device->songsByArtistAlbum( fi->parent()->text( 0 ), fi->text( 0 ) );
                    break;
                case 2:
                    items += fi->url().path();
                    break;
            }
        } else
        {
            if ( fi->childCount() )
                items += nodeBuildDragList( (MediaItem*)fi->firstChild() );
        }

        fi = (MediaItem*)fi->nextSibling();
    }

    return items;
}


void
MediaDeviceList::contentsDragEnterEvent( QDragEnterEvent *e )
{
    debug() << "Items dropping?" << endl;
    e->accept( e->source() != viewport() && KURLDrag::canDecode( e ) );
}


void
MediaDeviceList::contentsDropEvent( QDropEvent *e )
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


void
MediaDeviceList::contentsDragMoveEvent( QDragMoveEvent* /*e*/ )
{
//    const QPoint p = contentsToViewport( e->pos() );
//    QListViewItem *item = itemAt( p );
}


void
MediaDeviceList::viewportPaintEvent( QPaintEvent *e )
{
    KListView::viewportPaintEvent( e );

    // Superimpose bubble help:

    if ( !m_parent->m_connectButton->isOn() )
    {
        QPainter p( viewport() );

        QSimpleRichText t( i18n(
                "<div align=center>"
                  "<h3>MediaDevice Browser</h3>"
                  "Click the Connect button to access your mounted iPod. "
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
MediaDeviceList::rmbPressed( QListViewItem* item, const QPoint& point, int ) //SLOT
{
    if ( item )
    {
        KURL::List urls = nodeBuildDragList( 0 );
        KPopupMenu menu( this );

        enum Actions { APPEND, MAKE, QUEUE, BURN_ARTIST, BURN_ALBUM,
                       BURN_DATACD, BURN_AUDIOCD, DELETE };

        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue Track" ), QUEUE );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );
        menu.insertSeparator();

        switch ( item->depth() )
        {
            case 0:
                menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n( "Burn All Tracks by This Artist" ), BURN_ARTIST );
                menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() );
                break;

            case 1:
                menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n( "Burn This Album" ), BURN_ALBUM );
                menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() );
                break;

            case 2:
                menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n( "Burn to CD as Data" ), BURN_DATACD );
                menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
                menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n( "Burn to CD as Audio" ), BURN_AUDIOCD );
                menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
                break;
        }

        menu.insertSeparator();
        menu.insertItem( SmallIconSet( "editdelete" ), i18n( "Delete File" ), DELETE );

        switch( menu.exec( point ) )
        {
            case APPEND:
                Playlist::instance()->insertMedia( urls, Playlist::Append );
                break;
            case MAKE:
                Playlist::instance()->insertMedia( urls, Playlist::Replace );
                break;
            case QUEUE:
                Playlist::instance()->insertMedia( urls, Playlist::Queue );
                break;
            case BURN_ARTIST:
                K3bExporter::instance()->exportArtist( item->text(0) );
                break;
            case BURN_ALBUM:
                K3bExporter::instance()->exportAlbum( item->text(0) );
                break;
            case BURN_DATACD:
                K3bExporter::instance()->exportTracks( urls, K3bExporter::DataCD );
                break;
            case BURN_AUDIOCD:
                K3bExporter::instance()->exportTracks( urls, K3bExporter::AudioCD );
                break;
            case DELETE:
                m_parent->m_device->deleteFiles( urls );
                break;
        }
    }
}


MediaDeviceView::MediaDeviceView( MediaBrowser* parent )
    : QVBox( parent )
    , m_device ( new MediaDevice( this ) )
    , m_deviceList( new MediaDeviceList( this ) )
    , m_transferList( new KListView( this ) )
    , m_parent( parent )
{
    m_transferList->setFixedHeight( 200 );
    m_transferList->setSelectionMode( QListView::Extended );
    m_transferList->setItemsMovable( false );
    m_transferList->setShowSortIndicator( true );
    m_transferList->setFullWidth( true );
    m_transferList->setRootIsDecorated( false );
    m_transferList->setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    m_transferList->setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    m_transferList->setDropVisualizerWidth( 3 );
    m_transferList->setAcceptDrops( true );
    m_transferList->addColumn( i18n( "URL" ) );

    m_stats = new QLabel( i18n( "1 track in queue", "%n tracks in queue", m_transferList->childCount() ), this );
    m_progress = new KProgress( this );

    QHBox* hb = new QHBox( this );
    hb->setSpacing( 1 );
    m_connectButton  = new KPushButton( SmallIconSet( "usbpendrive_mount" ), i18n( "Connect"), hb );
    m_transferButton = new KPushButton( SmallIconSet( "rebuild" ), i18n( "Transfer" ), hb );
    m_configButton   = new KPushButton( KGuiItem( QString::null, "configure" ), hb );
    m_configButton->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred ); // too big!

    QToolTip::add( m_connectButton,  i18n( "Connect media device" ) );
    QToolTip::add( m_transferButton, i18n( "Transfer tracks to media device" ) );
    QToolTip::add( m_configButton,   i18n( "Configure mount commands" ) );

    m_connectButton->setToggleButton( true );
    m_connectButton->setOn( m_device->isConnected() ||  m_deviceList->childCount() != 0 );
    m_transferButton->setDisabled( true );

    m_progress->setFixedHeight( m_transferButton->sizeHint().height() );
    m_progress->hide();

    connect( m_connectButton,  SIGNAL( clicked() ), MediaDevice::instance(), SLOT( ipodConnection() ) );
    connect( m_transferButton, SIGNAL( clicked() ), MediaDevice::instance(), SLOT( transferFiles() ) );
    connect( m_configButton,   SIGNAL( clicked() ), MediaDevice::instance(), SLOT( config() ) );
    connect( m_transferList, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( slotShowContextMenu( QListViewItem*, const QPoint&, int ) ) );}

void
MediaDeviceView::slotShowContextMenu( QListViewItem* item, const QPoint& point, int )
{
    if ( item )
    {
        KPopupMenu menu( this );

        enum Actions { REMOVE_SELECTED, CLEAR_ALL };

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
}


MediaDeviceView::~MediaDeviceView()
{
    delete m_transferList;
    delete m_deviceList;
    delete m_device;
}


MediaDevice::MediaDevice( MediaDeviceView* parent )
    : m_parent( parent )
{
    s_instance = this;

    sysProc = new KShellProcess(); Q_CHECK_PTR(sysProc);

    m_ipod = new IPod::IPod();

    openIPod();

    m_mntcmd = AmarokConfig::mountCommand();
    m_umntcmd = AmarokConfig::umountCommand();
}

void
MediaDevice::openIPod()
{
    if ( !m_ipod->isOpen() )
        m_ipod->open( "/mnt/ipod" );

    if ( !m_ipod->isOpen() )
        m_ipod->open( "/media/iPod" ); // default path on SuSE 9.3

    if ( !m_ipod->isOpen() ) {
        // try to find a mounted ipod
        KMountPoint::List currentmountpoints = KMountPoint::currentMountPoints();
        KMountPoint::List::Iterator mountiter = currentmountpoints.begin();
        for(; mountiter != currentmountpoints.end(); ++mountiter) {
            QString mountpoint = (*mountiter)->mountPoint();
            QString device = (*mountiter)->mountedFrom();

            // only care about scsi devices (/dev/sd at the beginning or scsi somewhere in its name)
            if (device.find("/dev/sd") != 0 && device.find("scsi") < 0)
                continue;

            if (m_ipod->open(mountpoint))
                break;
        }
    }
}

MediaDevice::~MediaDevice()
{
    m_ipod->close();
    delete m_ipod;
}

void
MediaDevice::addURL( const KURL& url )
{
    MetaBundle mb( url );
    if ( !fileExists( mb ) && ( m_transferURLs.findIndex( url ) == -1 ) )
    {
        MediaItem* item = new MediaItem( m_parent->m_transferList );
        item->setExpandable( false );
        item->setDropEnabled( true );
        item->setUrl( url.path() );
        item->setText( 0, url.path() );

        m_transferURLs << url;
        m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_parent->m_transferList->childCount() ) );
        m_parent->m_transferButton->setEnabled( m_parent->m_device->isConnected() || m_parent->m_deviceList->childCount() != 0 );
    } else
        amaroK::StatusBar::instance()->longMessage( i18n( "Track already exists on iPod: " + url.path().local8Bit() ), KDE::StatusBar::Sorry );
}

void
MediaDevice::addURLs( const KURL::List urls )
{
        KURL::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
            addURL( *it );
}

QStringList
MediaDevice::items( QListViewItem* item )
{
    QStringList items;

    if ( !item )
        m_ipod->getArtists( items );
    else
    {
        if ( !item->parent() )  // second level
        {
            Artist* artist;
            artist = m_ipod->getArtistByName( item->text( 0 ) );

            if ( artist )
                for ( ArtistIterator it( *artist ); it.current(); ++it )
                    items << it.currentKey();
        }
        else
        {
            TrackList* album;
            album = m_ipod->getAlbum( item->parent()->text( 0 ), item->text( 0 ) );

            if ( album )
            {
                TrackList::Iterator it = album->getTrackIDs();
                while ( it.hasNext() )
                {
                    TrackMetadata* track = m_ipod->getTrackByID( it.next() );
                    items << track->getTitle();
                    items << m_ipod->getRealPath( track->getPath() );
                }
            }
        }
    }

    return items;
}


KURL::List
MediaDevice::songsByArtist( const QString& artist )
{
    KURL::List items;

    Artist* ar;
    ar = m_ipod->getArtistByName( artist );

    if ( ar )
        for ( ArtistIterator it( *ar ); it.current(); ++it )
        {
            TrackList* album;
            album = m_ipod->getAlbum( artist, it.currentKey() );

            if ( album )
            {
                TrackList::Iterator it = album->getTrackIDs();
                while ( it.hasNext() )
                {
                    TrackMetadata* track = m_ipod->getTrackByID( it.next() );
                    items << m_ipod->getRealPath( track->getPath() );
                }
            }
        }

    return items;
}


KURL::List
MediaDevice::songsByArtistAlbum( const QString& artist, const QString& album )
{
    KURL::List items;

    TrackList* al;
    al = m_ipod->getAlbum( artist, album );

    if ( al )
    {
        TrackList::Iterator it = al->getTrackIDs();
        while ( it.hasNext() )
        {
            TrackMetadata* track = m_ipod->getTrackByID( it.next() );
            items << m_ipod->getRealPath( track->getPath() );
        }
    }

    return items;
}

void
MediaDevice::clearItems()
{
    m_parent->m_transferList->clear();
    m_transferURLs.clear();
    m_parent->m_stats->setText( i18n( "0 tracks in queue" ) );
    m_parent->m_transferButton->setEnabled( false );
}

void
MediaDevice::removeSelected()
{
    QPtrList<QListViewItem>  selected = m_parent->m_transferList->selectedItems();

    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        m_transferURLs.remove( item->text( 0 ) );
        m_parent->m_transferList->takeItem( item );
        delete item;
    }
    m_parent->m_transferButton->setEnabled( m_parent->m_transferList->childCount() != 0 );
    m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_parent->m_transferList->childCount() ) );
}

void
MediaDevice::config()
{
    KDialogBase dialog( m_parent, 0, false );
    kapp->setTopWidget( &dialog );
    dialog.setCaption( kapp->makeStdCaption( i18n("Configure Media Device") ) );
    dialog.showButtonApply( false );
    QVBox *box = dialog.makeVBoxMainWidget();

    QLabel *mntLabel = new QLabel( box );
    mntLabel->setText( i18n( "&Mount command:" ) );
    QLineEdit *mntcmd = new QLineEdit( m_mntcmd, box );
    mntLabel->setBuddy( mntcmd );
    QToolTip::add( mntcmd, i18n( "Set the command to mount your device here, empty commands are not executed." ) );

    QLabel *umntLabel = new QLabel( box );
    umntLabel->setText( i18n( "&Unmount command:" ) );
    QLineEdit *umntcmd = new QLineEdit( m_umntcmd, box );
    umntLabel->setBuddy( umntcmd );
    QToolTip::add( umntcmd, i18n( "Set the command to unmount your device here, empty commands are not executed." ) );

    if ( dialog.exec() != QDialog::Rejected )
    {
        setMountCommand( mntcmd->text() );
        setUmountCommand( umntcmd->text() );
    }
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
MediaDevice::transferFiles()  //SLOT
{
    m_parent->m_transferButton->setEnabled( false );

    if ( m_ipod->ensureConsistency() )
    {
        m_parent->m_progress->setProgress( 0 );
        m_parent->m_progress->setTotalSteps( m_parent->m_transferList->childCount() );
        m_parent->m_progress->show();

        // ok, let's copy the stuff to the ipod
        m_ipod->lock( true );
        if ( m_ipod->ensureConsistency() )
        {
            // iterate through items
            KURL::List::Iterator it = m_transferURLs.begin();
            for ( ; it != m_transferURLs.end(); ++it )
            {
                debug() << "Transfering: " << (*it).path() << endl;
                MetaBundle bundle( *it );

                TrackMetadata track = m_ipod->createNewTrackMetadata();
                track.setPath( track.getPath() + ".mp3" );
                QString trackpath = m_ipod->getRealPath( track.getPath() );

                // check if path exists and make the path if needed
                QFileInfo finfo( trackpath );
                QDir dir = finfo.dir();
                if ( !dir.exists() )
                {
                    QDir parentdir = dir;
                    parentdir.cdUp();
                    if ( parentdir.exists() )
                        dir.mkdir( finfo.dirPath() );
                }

                m_wait = true;

                KIO::CopyJob *job = KIO::copy( *it, KURL( trackpath ), false );
                connect( job, SIGNAL( copyingDone( KIO::Job *, const KURL &, const KURL &, bool, bool ) ),
                        this,  SLOT( fileTransferred() ) );

                while ( m_wait )
                {
                    kapp->processEvents( 100 );
                }

                if( !track.readFromBundle( bundle ) )
                {
                    debug() << "Reading tags failed! File not added!" << endl;
                    QFile::remove( trackpath );
                }
                else
                    m_ipod->addTrack( track );
            }
        }
        else
            debug() << "iPod inconsistent!" << endl;

        m_ipod->unlock();
        syncIPod();
        fileTransferFinished();
    }
    else
        debug() << "iPod inconsistent!" << endl;

    m_parent->m_transferButton->setEnabled( false );
}


void
MediaDevice::deleteFiles( const KURL::List& urls )
{
    //NOTE we assume that currentItem is the main target
    int count  = urls.count();

    //Enable after string freeze.
    //m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", count ) );
    m_parent->m_progress->setProgress( 0 );
    m_parent->m_progress->setTotalSteps( count );
    m_parent->m_progress->show();

    int button = KMessageBox::warningContinueCancel( m_parent,
                                                     i18n( "<p>You have selected 1 file to be <b>irreversibly</b> deleted.",
                                                           "<p>You have selected %n files to be <b>irreversibly</b> deleted.",
                                                           count
                                                         ),
                                                     QString::null,
                                                     KGuiItem(i18n("&Delete"),"editdelete") );

    if ( button == KMessageBox::Continue )
    {

        KURL::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
        {
            m_parent->m_progress->setProgress( m_parent->m_progress->progress() + 1 );
            KIO::del( *it, false, false );
        }
        if ( m_ipod->ensureConsistency() )
        {
            m_ipod->lock( true );
            deleteFromIPod( 0 );
            m_ipod->unlock();
            syncIPod();
        }
        else
            debug() << "iPod inconsistent!" << endl;
    }
    QTimer::singleShot( 1500, m_parent->m_progress, SLOT(hide()) );
    //Enable after string freeze
    //m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_parent->m_transferList->childCount() ) );
}


void
MediaDevice::deleteFromIPod( MediaItem* item )
{
    MediaItem* fi;

    if ( !item )
        fi = (MediaItem*)m_parent->m_deviceList->firstChild();
    else
        fi = item;

    while ( fi )
    {
        if ( fi->isSelected() )
        {
            switch ( fi->depth() )
            {
                case 0:
                    Artist* artist;
                    artist = m_ipod->getArtistByName( fi->text( 0 ) );

                    if ( artist )
                        for ( ArtistIterator it( *artist ); it.current(); ++it )
                            m_ipod->deleteAlbum( fi->text( 0 ), it.currentKey() );

                    m_ipod->deleteArtist( fi->text( 0 ) );

                    break;
                case 1:
                    m_ipod->deleteAlbum( fi->parent()->text( 0 ), fi->text( 0 ) );

                    break;
                case 2:
                    TrackMetadata* track = 0;
                    TrackList* playlist;
                    playlist = m_ipod->getAlbum( fi->parent()->parent()->text( 0 ), fi->parent()->text( 0 ) );

                    TrackList::Iterator tit = playlist->getTrackIDs();
                    while( tit.hasNext() )
                    {
                        Q_UINT32 trackid = tit.next();
                        track = m_ipod->getTrackByID( trackid );
                        if ( track && ( track->getTitle() == fi->text( 0 ) ) )
                            break;

                        track = 0;
                    }

                    if ( track )
                        m_ipod->deleteTrack( track->getID() );

                    break;
            }
        } else
        {
            if ( fi->childCount() )
                deleteFromIPod( (MediaItem*)fi->firstChild() );
        }

        fi = (MediaItem*)fi->nextSibling();
    }
}

void
MediaDevice::ipodConnection() //SLOT
{
    if ( m_parent->m_connectButton->isOn() )
    {
        if ( !m_mntcmd.isEmpty() )
        {
            mount();
        }
        m_ipod = new IPod::IPod();
        openIPod();
        m_parent->m_deviceList->renderView( 0 );

        if( isConnected() || m_parent->m_deviceList->childCount() != 0 )
        {
            m_parent->m_connectButton->setOn( true );
            if ( m_parent->m_transferList->childCount() != 0 )
            {
                m_parent->m_transferButton->setEnabled( true );
                //Enable after string freeze
                //m_parent->m_stats->setText( i18n( "Checking device for duplicate files." ) );
                KURL::List urls;
                urls = m_transferURLs;
                clearItems();
                addURLs( urls );
           }
        }
        else
        {
            m_parent->m_connectButton->setOn( false );
            KMessageBox::error( m_parent->m_parent,
                i18n( "Could not find device, please mount it and try again." ),
                i18n( "Media Device Browser" ) );
        }
    }
    else
    {
        if ( m_parent->m_transferList->childCount() != 0 &&  m_ipod->isStillConnected() )
        {
            int button = KMessageBox::warningContinueCancel( m_parent->m_parent,
                                                            i18n( "There are tracks queued for transfer."
                                                            " Would you like to transfer them before disconnecting?"),
                                                            i18n( "Media Device Browser" ),
                                                            KGuiItem(i18n("&Transfer"),"rebuild") );

            if ( button == KMessageBox::Continue )
                transferFiles();
        }

        fileTransferFinished();
        m_ipod->close();
        QString text = i18n( "Your device is now in sync, please unmount it and disconnect now." );

        if ( !m_umntcmd.isEmpty() )
        {
            umount();
            text=i18n( "Your device is now in sync, you can disconnect now." );
        }

        m_ipod = new IPod::IPod();
        m_parent->m_deviceList->renderView( 0 );
        m_parent->m_connectButton->setOn( false );
        KMessageBox::information( m_parent->m_parent, text, i18n( "Media Device Browser" ) );
    }
}

bool
MediaDevice::fileExists( const MetaBundle& bundle )
{
    TrackList* album;

    album = m_ipod->getAlbum( bundle.artist(), bundle.album().isEmpty() ? i18n( "Unknown" ) : bundle.album() );
    if ( album )
    {
        TrackList::Iterator it = album->getTrackIDs();
        while ( it.hasNext() )
        {
            TrackMetadata* track = m_ipod->getTrackByID( it.next() );
            if ( track->getTitle() == bundle.title() )
                return true;
        }
    }

    return false;
}


void
MediaDevice::fileTransferred()  //SLOT
{
    m_wait = false;
    m_parent->m_progress->setProgress( m_parent->m_progress->progress() + 1 );
}


void
MediaDevice::fileTransferFinished()  //SLOT
{
    m_parent->m_transferList->clear();
    m_transferURLs.clear();

    m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_parent->m_transferList->childCount() ) );
    m_parent->m_progress->hide();
    m_parent->m_transferButton->setDisabled( true );
}


void
MediaDevice::syncIPod()  //SLOT
{
    debug() << "Syncing IPod!" << endl;

    m_ipod->ensureConsistency();
    m_ipod->writeItunesDB();
    if( !m_ipod->getItunesDBError().isEmpty() )
        debug() << "Sync failed: " + m_ipod->getItunesDBError() << endl;

    m_ipod->close();
    delete m_ipod;

    m_ipod = new IPod::IPod();
    openIPod();

    m_parent->m_deviceList->renderView( 0 );
}


#include "mediabrowser.moc"
