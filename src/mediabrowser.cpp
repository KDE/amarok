// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information


#include "config.h"        //for AMAZON_SUPPORT

#include "amarokconfig.h"
#include "clicklineedit.h"
#include "colorgenerator.h"
#include "mediabrowser.h"
#include "metabundle.h"
#include "playlist.h"      //appendMedia()
#include "statusbar.h"

#include <qdatetime.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qtooltip.h>       //QToolTip::add()

#include <kapplication.h> //kapp
#include <kdebug.h>
#include <kdirlister.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kprogress.h>
#include <krun.h>
#include <kstandarddirs.h> //locate file
#include <ktabbar.h>
#include <ktempfile.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h> //ctor
#include <kurl.h>
#include <kurldrag.h>       //dragObject()

#define escapeIPod(s)   QString(s).replace( "/", "%252f" )


MediaDevice *MediaDevice::s_instance = 0;

bool MediaBrowser::isAvailable() //static
{
    return !KStandardDirs::findExe( "gnupod_addsong.pl" ).isNull();
}


MediaBrowser::MediaBrowser( const char *name )
   : QVBox( 0, name )
{
    m_device = new MediaDevice( this );

    setSpacing( 4 );
    setMargin( 5 );

    KToolBar* toolbar = new KToolBar( this );
    toolbar->setMovingEnabled(false);
    toolbar->setFlat(true);
    toolbar->setIconSize( 16 );
    toolbar->setEnableContextMenu( false );

    { //<Search LineEdit>
        KToolBarButton *button;
        KToolBar* searchToolBar = new KToolBar( this );
        searchToolBar->setMovingEnabled(false);
        searchToolBar->setFlat(true);
        searchToolBar->setIconSize( 16 );
        searchToolBar->setEnableContextMenu( false );

        button       = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_searchEdit = new ClickLineEdit( searchToolBar, i18n( "Filter here..." ), "filter_edit" );
        searchToolBar->setStretchableWidget( m_searchEdit );

        m_searchEdit->setFrame( QFrame::Sunken );
        connect( button, SIGNAL( clicked() ), m_searchEdit, SLOT( clear() ) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to filter collection" ) );
    } //</Search LineEdit>

    m_view = new MediaDeviceView( this );

    setFocusProxy( m_view ); //default object to get focus
    setMinimumWidth( toolbar->sizeHint().width() + 2 ); //set a reasonable minWidth
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
    renderView();
    
    connect( this, SIGNAL( expanded( QListViewItem* ) ),
             this,   SLOT( slotExpand( QListViewItem* ) ) );

    connect( this, SIGNAL( collapsed( QListViewItem* ) ),
             this,   SLOT( slotCollapse( QListViewItem* ) ) );
}


MediaDeviceList::~MediaDeviceList()
{}


void
MediaDeviceList::renderView()
{
    clear();
    renderNode( 0, KURL( "ipod:/Artists/" ) );
}


void
MediaDeviceList::renderNode( QListViewItem* parent, const KURL& url )  //SLOT
{
    KDirLister dl;
    dl.setAutoErrorHandlingEnabled( false, 0 );
    dl.openURL( url, false, true );

    while ( !dl.isFinished() )
        kapp->processEvents( 100 );

    KFileItem* fi;
    KFileItemList fil = dl.items();

    for ( fi = fil.first(); fi; fi = fil.next() )
    {
        KIconLoader iconLoader;
        QPixmap pixmap = iconLoader.loadIcon( "usbpendrive_unmount", KIcon::Toolbar, KIcon::SizeSmall );

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
MediaDeviceList::slotExpand( QListViewItem* item )  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !item ) return;

    QString url( "ipod:/Artists/" );
    if ( item->parent() ) url += escapeIPod( item->parent()->text( 0 ) ) + "/";
    url += escapeIPod( item->text( 0 ) );

    kdDebug() << "[MediaBrowser] " << url << endl;
    renderNode( item, KURL( url ) );
}


void
MediaDeviceList::slotCollapse( QListViewItem* item )  //SLOT
{
    kdDebug() << k_funcinfo << endl;

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
    nodeBuildDragList( 0 );
    KURLDrag* d = new KURLDrag( m_dragList, this );
    d->dragCopy();
}


void
MediaDeviceList::nodeBuildDragList( MediaItem* item )
{
    MediaItem* fi;

    m_dragList.clear();

    if ( !item )
        fi = (MediaItem*)firstChild();
    else
        fi = item;

    while ( fi )
    {
        if ( fi->isSelected() )
        {
            kdDebug() << fi->url().path() << endl;
            m_dragList << fi->url().path();
        } else
        {
            if ( fi->childCount() )
                nodeBuildDragList( (MediaItem*)fi->firstChild() );
        }

        fi = (MediaItem*)fi->nextSibling();
    }
}


void
MediaDeviceList::contentsDragEnterEvent( QDragEnterEvent *e )
{
    kdDebug() << "[MediaBrowser] Items dropping?" << endl;
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
MediaDeviceList::contentsDragMoveEvent( QDragMoveEvent* e )
{
    const QPoint p = contentsToViewport( e->pos() );
    QListViewItem *item = itemAt( p );
}   


MediaDeviceView::MediaDeviceView( MediaBrowser* parent )
    : QVBox( parent )
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
    
    QHBox* hb( this );
    hb->setSpacing( 4 );
    m_stats = new QLabel( i18n( "1 track in queue", "%n tracks in queue", m_transferList->childCount() ), hb );
    m_progress = new KProgress( hb );
    QPushButton* transferButton = new QPushButton( SmallIconSet( "rebuild" ), i18n( "Transfer" ), hb );

    m_progress->setFixedHeight( transferButton->sizeHint().height() );
    m_progress->hide();

    connect( transferButton, SIGNAL( clicked() ), MediaDevice::instance(), SLOT( transferFiles() ) );
}


MediaDeviceView::~MediaDeviceView()
{}


MediaDevice::MediaDevice( MediaBrowser* parent )
    : m_parent( parent )
{
    s_instance = this;
}


void
MediaDevice::addURL( const KURL& url )
{
    MetaBundle mb( url, false );
    if ( !fileExists( mb ) && ( m_transferURLs.findIndex( url ) == -1 ) )
    {
        MediaItem* item = new MediaItem( m_parent->m_view->m_transferList );
        item->setExpandable( false );
        item->setDropEnabled( true );
        item->setUrl( url.path() );
        item->setText( 0, url.path() );
    
        m_transferURLs << url;
        m_parent->m_view->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_parent->m_view->m_transferList->childCount() ) );
    } else
        amaroK::StatusBar::instance()->message( i18n( "Track already exists on iPod: " + url.path().local8Bit() ) );
}


void
MediaDevice::transferFiles()  //SLOT
{
    m_parent->m_view->m_progress->setTotalSteps( m_parent->m_view->m_transferList->childCount() );
    m_parent->m_view->m_progress->show();

    KIO::CopyJob *job = KIO::copy( m_transferURLs, KURL( "ipod:/Artists/" ), true );
    connect( job, SIGNAL( copyingDone( KIO::Job *, const KURL &, const KURL &, bool, bool ) ),
             this,  SLOT( fileTransferred( KIO::Job *, const KURL &, const KURL &, bool, bool ) ) );

    connect( job, SIGNAL( result( KIO::Job * ) ),
             this,  SLOT( fileTransferFinished( KIO::Job * ) ) );
}


bool
MediaDevice::fileExists( const MetaBundle& bundle )  
{
    // trackno - songtitle.extension
    QString filename = QString( "%1 - %2.mp3" )
                          .arg( bundle.track().toInt() > 0 ? bundle.track() : "1" )
                          .arg( bundle.title() );

    QString path = escapeIPod( bundle.artist() ) + "/" + escapeIPod( bundle.album() ) + "/";

    return KIO::NetAccess::exists( KURL( "ipod:/Artists/" + path + filename ) );
}


void
MediaDevice::fileTransferred( KIO::Job *job, const KURL &from, const KURL &to, bool dir, bool renamed )  //SLOT
{
    m_parent->m_view->m_progress->setProgress( m_parent->m_view->m_progress->value() + 1 );
    m_parent->m_view->m_deviceList->renderView();
}


void
MediaDevice::fileTransferFinished( KIO::Job *job )  //SLOT
{
    m_parent->m_view->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_parent->m_view->m_transferList->childCount() ) );
    m_parent->m_view->m_transferList->clear();
    m_transferURLs.clear();
    
    // sync ipod, now
    KIO::get( "ipod:/Utilities/Synchronize?really=OK", false, false );
    m_parent->m_view->m_progress->hide();
}


#include "mediabrowser.moc"
