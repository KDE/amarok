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
#include "gpodmediadevice.h"
#include "amarok.h"

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


MediaDevice *MediaDevice::s_instance = 0;

bool MediaBrowser::isAvailable() //static
{
    // perhaps the user should configure if he wants to use a media device?
#ifdef HAVE_LIBGPOD
    return true;
#else
    return false;
#endif
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
    expandItem( 0 );

    connect( this, SIGNAL( expanded( QListViewItem* ) ),
             this,   SLOT( expandItem( QListViewItem* ) ) );

    connect( this, SIGNAL( collapsed( QListViewItem* ) ),
             this,   SLOT( collapseItem( QListViewItem* ) ) );

    connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );
}


MediaDeviceList::~MediaDeviceList()
{}


void
MediaDeviceList::expandItem( QListViewItem* parent )  //SLOT
{
    if ( parent == 0 )
        clear();

    if ( m_parent->m_device == NULL )
        return;

    KIconLoader iconLoader;
    QPixmap pixmap = iconLoader.loadIcon( "usbpendrive_unmount", KIcon::Toolbar, KIcon::SizeSmall );

    QStringList items;
    items = m_parent->m_device->items( parent );

    bool track = ( parent && parent->parent() );
    for ( uint i = 0; i < items.count(); track ? i+=3 : ++i )
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
        {
            item->setUrl( items[i+1] );
            item->m_track = items[i+2].toInt();
        }
    }
}


void
MediaDeviceList::collapseItem( QListViewItem* item )  //SLOT
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
MediaDeviceList::nodeBuildDragList( MediaItem* item, bool onlySelected )
{
    KURL::List items;
    MediaItem* fi;

    if ( !item )
    {
        fi = (MediaItem*)firstChild();
    }
    else
        fi = item;

    while ( fi )
    {
        if ( fi->isSelected() || !onlySelected )
        {
            debug() << "drag depth: " << fi->depth() << endl;
            switch ( fi->depth() )
            {
                case 0:
                case 1:
                    {
                        bool collapse = false;
                        if(fi->childCount() == 0)
                        {
                            expandItem( fi );
                            collapse = true;
                        }
                        if(fi->childCount())
                            items += nodeBuildDragList( (MediaItem*)fi->firstChild(), false );
                        if(collapse)
                            collapseItem(fi);
                    }
                    break;
                case 2:
                    items += fi->url().path();
                    break;
            }
        }
        else
        {
            if ( fi->childCount() )
                items += nodeBuildDragList( (MediaItem*)fi->firstChild(), true );
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
MediaDeviceList::contentsDragMoveEvent( QDragMoveEvent *e )
{
//    const QPoint p = contentsToViewport( e->pos() );
//    QListViewItem *item = itemAt( p );
    e->accept( e->source() != this
            && e->source() != viewport()
            && e->source() != m_parent
            && e->source() != m_parent->m_device->m_transferList
            && e->source() != m_parent->m_device->m_transferList->viewport()
            && KURLDrag::canDecode( e ) );
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
    , m_stats( NULL )
    , m_device( NULL )
    , m_deviceList( new MediaDeviceList( this ) )
    , m_parent( parent )
{
    m_device = new GpodMediaDevice( this );
    m_progress = new KProgress( this );

    QHBox* hb = new QHBox( this );
    hb->setSpacing( 1 );
    m_connectButton  = new KPushButton( SmallIconSet( "usbpendrive_mount" ), i18n( "Connect"), hb );
    m_transferButton = new KPushButton( SmallIconSet( "rebuild" ), i18n( "Transfer" ), hb );
    m_configButton   = new KPushButton( KGuiItem( QString::null, "configure" ), hb );
    m_configButton->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred ); // too big!

    m_stats = new QLabel( i18n( "1 track in queue", "%n tracks in queue", m_device->m_transferList->childCount() ), this );

    QToolTip::add( m_connectButton,  i18n( "Connect media device" ) );
    QToolTip::add( m_transferButton, i18n( "Transfer tracks to media device" ) );
    QToolTip::add( m_configButton,   i18n( "Configure mount commands" ) );

    m_connectButton->setToggleButton( true );
    m_connectButton->setOn( m_device->isConnected() ||  m_deviceList->childCount() != 0 );
    m_transferButton->setDisabled( true );

    m_progress->setFixedHeight( m_transferButton->sizeHint().height() );
    m_progress->hide();

    connect( m_connectButton,  SIGNAL( clicked() ), MediaDevice::instance(), SLOT( connectDevice() ) );
    connect( m_transferButton, SIGNAL( clicked() ), MediaDevice::instance(), SLOT( transferFiles() ) );
    connect( m_configButton,   SIGNAL( clicked() ), MediaDevice::instance(), SLOT( config() ) );
    connect( m_device->m_transferList, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( slotShowContextMenu( QListViewItem*, const QPoint&, int ) ) );

    m_device->loadTransferList( amaroK::saveLocation() + "transferlist.xml" );

    m_device->openDevice();
}

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
    m_device->saveTransferList( amaroK::saveLocation() + "transferlist.xml" );
    m_device->closeDevice();

    delete m_deviceList;
    delete m_device;
}


MediaDevice::MediaDevice( MediaDeviceView* parent )
    : m_parent( parent )
    , m_transferList( new MediaDeviceTransferList( parent ) )
{
    s_instance = this;

    sysProc = new KShellProcess(); Q_CHECK_PTR(sysProc);

    m_mntpnt = AmarokConfig::mountPoint();
    m_mntcmd = AmarokConfig::mountCommand();
    m_umntcmd = AmarokConfig::umountCommand();
}

MediaDevice::~MediaDevice()
{
    delete m_transferList;
}

void
MediaDevice::addURL( const KURL& url, MetaBundle *bundle, bool isPodcast )
{
    if(!bundle)
        bundle = new MetaBundle( url );
    if ( !trackExists( *bundle ) && ( m_transferList->findPath( url.path() ) == NULL ) )
    {
        MediaItem* item = new MediaItem( m_transferList, m_transferList->lastItem() );
        item->setExpandable( false );
        item->setDropEnabled( true );
        item->setUrl( url.path() );
        item->m_bundle = bundle;
        item->m_podcast = isPodcast;

        QString text = item->bundle()->prettyTitle();
        if(item->m_podcast)
        {
            text += " (" + i18n("Podcast") + ")";
        }
        item->setText( 0, text);

        m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_transferList->childCount() ) );
        m_parent->m_transferButton->setEnabled( m_parent->m_device->isConnected() || m_parent->m_deviceList->childCount() != 0 );
        m_parent->m_progress->setTotalSteps( m_parent->m_progress->totalSteps() + 1 );
    } else
        amaroK::StatusBar::instance()->longMessage( i18n( "Track already exists on media device: " + url.path().local8Bit() ), KDE::StatusBar::Sorry );
}

void
MediaDevice::addURLs( const KURL::List urls, MetaBundle *bundle )
{
        KURL::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
            addURL( *it, bundle, bundle!=NULL );
}


void
MediaDevice::clearItems()
{
    m_transferList->clear();
    if(m_parent && m_parent->m_stats)
        m_parent->m_stats->setText( i18n( "0 tracks in queue" ) );
    if(m_parent && m_parent->m_transferButton)
        m_parent->m_transferButton->setEnabled( false );
}

void
MediaDevice::removeSelected()
{
    QPtrList<QListViewItem>  selected = m_transferList->selectedItems();

    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        m_transferList->takeItem( item );
        delete item;
    }
    m_parent->m_transferButton->setEnabled( m_transferList->childCount() != 0 );
    m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_transferList->childCount() ) );
}

void
MediaDevice::config()
{
    KDialogBase dialog( m_parent, 0, false );
    kapp->setTopWidget( &dialog );
    dialog.setCaption( kapp->makeStdCaption( i18n("Configure Media Device") ) );
    dialog.showButtonApply( false );
    QVBox *box = dialog.makeVBoxMainWidget();

    QLabel *mntpntLabel = new QLabel( box );
    mntpntLabel->setText( i18n( "&Mount point:" ) );
    QLineEdit *mntpnt = new QLineEdit( m_mntpnt, box );
    mntpntLabel->setBuddy( mntpnt );
    QToolTip::add( mntpnt, i18n( "Set the mount point of your device here, when empty autodetection is tried." ) );

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
        setMountPoint( mntpnt->text() );
        setMountCommand( mntcmd->text() );
        setUmountCommand( umntcmd->text() );
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
MediaDevice::connectDevice()
{
    if ( m_parent->m_connectButton->isOn() )
    {
        if ( !m_mntcmd.isEmpty() )
        {
            mount();
        }

        openDevice();
        updateView();

        if( isConnected() || m_parent->m_deviceList->childCount() != 0 )
        {
            m_parent->m_connectButton->setOn( true );
            if ( m_transferList->childCount() != 0 )
            {
                m_parent->m_transferButton->setEnabled( true );
                m_parent->m_stats->setText( i18n( "Checking device for duplicate files." ) );
                KURL::List urls;
                for( MediaItem *cur = dynamic_cast<MediaItem *>(m_transferList->firstChild());
                        cur != NULL;
                        cur = dynamic_cast<MediaItem *>(cur->nextSibling()) )
                {
                    urls.append( cur->url() );
                }
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
        if ( m_transferList->childCount() != 0 &&  isConnected() )
        {
            KGuiItem transfer = KGuiItem(i18n("&Transfer"),"rebuild");
            KGuiItem disconnect = KGuiItem(i18n("Disconnect immediately"),"rebuild");
            int button = KMessageBox::warningYesNo( m_parent->m_parent,
                    i18n( "There are tracks queued for transfer."
                        " Would you like to transfer them before disconnecting?"),
                    i18n( "Media Device Browser" ),
                    transfer, disconnect);

            if ( button == KMessageBox::Yes )
            {
                transferFiles();
                fileTransferFinished();
            }
        }

        m_parent->m_transferButton->setEnabled( false );

        closeDevice();
        QString text = i18n( "Your device is now in sync, please unmount it and disconnect now." );

        if ( !m_umntcmd.isEmpty() )
        {
            umount();
            text=i18n( "Your device is now in sync, you can disconnect now." );
        }

        updateView();
        m_parent->m_connectButton->setOn( false );
        KMessageBox::information( m_parent->m_parent, text, i18n( "Media Device Browser" ) );
    }
}


void
MediaDevice::fileTransferred()  //SLOT
{
    m_wait = false;
    m_parent->m_progress->setProgress( m_parent->m_progress->progress() + 1 );
    // the track just transferred has not yet been removed from the queue
    m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_transferList->childCount()-1 ) );
}

void
MediaDevice::transferFiles()
{
    m_parent->m_transferButton->setEnabled( false );

    m_parent->m_progress->setProgress( 0 );
    m_parent->m_progress->setTotalSteps( m_transferList->childCount() );
    m_parent->m_progress->show();

    // ok, let's copy the stuff to the device
    lockDevice( true );

    // iterate through items
    for( MediaItem *cur =  dynamic_cast<MediaItem *>(m_transferList->firstChild());
            cur != NULL;
            cur =  dynamic_cast<MediaItem *>(m_transferList->firstChild()) )
    {
        debug() << "Transfering: " << cur->url().path() << endl;

        MetaBundle *bundle = cur->bundle();
        if(!bundle)
        {
            bundle = new MetaBundle( cur->url() );
        }

        QString trackpath = createPathname(*bundle);

        // check if path exists and make it if needed
        QFileInfo finfo( trackpath );
        QDir dir = finfo.dir();
        while ( !dir.exists() )
        {
            QString path = dir.absPath();
            QDir parentdir;
            QDir create;
            do
            {
                create.setPath(path);
                path = path.section("/", 0, path.contains('/')-1);
                parentdir.setPath(path);
            }
            while( !path.isEmpty() && !(path==m_mntpnt) && !parentdir.exists() );
            debug() << "trying to create \"" << path << "\"" << endl;
            if(!create.mkdir( create.absPath() ))
            {
                break;
            }
        }

        if ( !dir.exists() )
        {
            KMessageBox::error( m_parent->m_parent,
                    i18n("Could not create directory for file") + trackpath,
                    i18n( "Media Device Browser" ) );
            delete bundle;
            break;
        }

        m_wait = true;

        KIO::CopyJob *job = KIO::copy( cur->url(), KURL( trackpath ), false );
        connect( job, SIGNAL( copyingDone( KIO::Job *, const KURL &, const KURL &, bool, bool ) ),
                this,  SLOT( fileTransferred() ) );

        while ( m_wait )
        {
            usleep(10000);
            kapp->processEvents( 100 );
        }


        KURL url;
        url.setPath(trackpath);
        MetaBundle bundle2(url);
        if(!bundle2.isValidMedia())
        {
            // probably s.th. went wrong
            debug() << "Reading tags failed! File not added!" << endl;
            QFile::remove( trackpath );
        }
        else
        {
            addTrackToDevice(trackpath, *bundle, cur->m_podcast);

            m_transferList->takeItem( cur );
            delete cur;
            cur = NULL;
        }
        delete bundle;
    }
    unlockDevice();
    synchronizeDevice();
    fileTransferFinished();

    m_parent->m_transferButton->setEnabled( m_transferList->childCount()>0 );
}


void
MediaDevice::fileTransferFinished()  //SLOT
{
    m_transferList->clear();

    m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_transferList->childCount() ) );
    m_parent->m_progress->hide();
    m_parent->m_transferButton->setDisabled( true );
}

void
MediaDevice::deleteFiles( const KURL::List& urls )
{
    //NOTE we assume that currentItem is the main target
    int count  = urls.count();

    m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", count ) );
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

        lockDevice( true );
        KURL::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
        {
            m_parent->m_progress->setProgress( m_parent->m_progress->progress() + 1 );
            debug() << "deleting " << (*it).prettyURL() << endl;
            //KIO::del( *it, false, false );
            KIO::file_delete( *it, false );
        }
        deleteFromDevice( 0 );
        unlockDevice();
        synchronizeDevice();
    }
    QTimer::singleShot( 1500, m_parent->m_progress, SLOT(hide()) );
    m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_transferList->childCount() ) );
}

void
MediaDevice::deleteFromDevice(MediaItem *item, bool onlySelected)
{
    MediaItem* fi;

    if ( !item )
    {
        fi = (MediaItem*)m_parent->m_deviceList->firstChild();
    }
    else
        fi = item;

    while ( fi )
    {
        if ( fi->isSelected() || !onlySelected )
        {
            debug() << "depth=" << fi->depth() << endl;
            switch ( fi->depth() )
            {
            case 0:
            case 1:
                {
                    bool collapse = false;
                    if(fi->childCount() == 0)
                    {
                        m_parent->m_deviceList->expandItem( fi );
                        collapse = true;
                    }
                    if ( fi->childCount() )
                        deleteFromDevice( (MediaItem*)fi->firstChild(), false);
                    if(collapse)
                        m_parent->m_deviceList->collapseItem(fi);
                }
                break;
            case 2:
                deleteTrackFromDevice(fi->parent()->parent()->text( 0 ), fi->parent()->text( 0 ), fi->text( 0 ));
                break;
            }
        }
        else
        {
            if ( fi->childCount() )
                deleteFromDevice( (MediaItem*)fi->firstChild(), true );
        }

        fi = (MediaItem*)fi->nextSibling();
    }
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

        if(item->bundle())
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

        if(item->m_podcast)
        {
            i.setAttribute( "podcast", "1" );
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

        addURL( url, bundle, isPodcast );
    }
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
    addColumn( i18n( "Track" ) );
}

void
MediaDeviceTransferList::dragEnterEvent( QDragEnterEvent *e )
{
    debug() << "Items dropping to list?" << endl;
    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && e->source() != m_parent->m_deviceList
            && e->source() != m_parent->m_deviceList->viewport()
            && KURLDrag::canDecode( e ) );
}


void
MediaDeviceTransferList::dropEvent( QDropEvent *e )
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
MediaDeviceTransferList::contentsDragEnterEvent( QDragEnterEvent *e )
{
    debug() << "Items dropping to list?" << endl;
    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && e->source() != m_parent->m_deviceList
            && e->source() != m_parent->m_deviceList->viewport()
            && KURLDrag::canDecode( e ) );
}


void
MediaDeviceTransferList::contentsDropEvent( QDropEvent *e )
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
MediaDeviceTransferList::contentsDragMoveEvent( QDragMoveEvent *e )
{
//    const QPoint p = contentsToViewport( e->pos() );
//    QListViewItem *item = itemAt( p );
    e->accept( e->source() != viewport()
            && e->source() != m_parent
            && e->source() != m_parent->m_deviceList
            && e->source() != m_parent->m_deviceList->viewport()
            && KURLDrag::canDecode( e ) );
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

    return NULL;
}



#include "mediabrowser.moc"
