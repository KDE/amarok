/* Copyright 2002-2004 Mark Kretschmann, Max Howell
 * Licensed as described in the COPYING file found in the root of this distribution
 * Maintainer: Max Howell <max.howell@methylblue.com>

 * NOTES
 *
 * The PlaylistWindow handles some Playlist events. Thanks!
 * This class has a QOBJECT but it's private so you can only connect via PlaylistWindow::PlaylistWindow
 * Mostly it's sensible to implement playlist functionality in this class
 * TODO Obtaining information about the playlist is currently hard, we need the playlist to be globally
 *      available and have some more useful public functions
 */

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"    //startEditTag()
#include "enginecontroller.h"
#include "metabundle.h"
#include "osd.h"
#include "playlist.h"
#include "playlistitem.h"
#include "playlistloader.h"
#include "statusbar.h"       //for status messages
#include "tagdialog.h"
#include "threadweaver.h"

#include <qclipboard.h>      //copyToClipboard(), slotMouseButtonPressed()
#include <qcolor.h>
#include <qevent.h>
#include <qfile.h>           //undo system
#include <qheader.h>         //eventFilter()
#include <qmap.h>            //dragObject()
#include <qpainter.h>
#include <qpen.h>            //slotGlowTimer()
#include <qtimer.h>
#include <qvaluevector.h>    //playNextTrack()
#include <kaction.h>
#include <kapplication.h>
#include <kcursor.h>         //setOverrideCursor()
#include <kdebug.h>
#include <kiconloader.h>     //slotShowContextMenu()
#include <kio/job.h>         //deleteSelectedFiles()
#include <klineedit.h>       //setCurrentTrack()
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krandomsequence.h> //random Mode
#include <kstandarddirs.h>   //KGlobal::dirs()
#include <kstdaction.h>
#include <kstringhandler.h>  //::showContextMenu()
#include <kglobalsettings.h> //startEditTag()
#include <kurldrag.h>
#include <X11/Xlib.h>        //ControlMask in contentsDragMoveEvent()
#include <qsortedlist.h>


class MyIterator : public QListViewItemIterator
{
public:
    MyIterator( QListViewItem *item, int flags = MyIterator::Visible )
        : QListViewItemIterator( item, flags )
    {}

    MyIterator( QListView *view, int flags = MyIterator::Visible )
        : QListViewItemIterator( view, flags )
    {}

    PlaylistItem *operator*() { return (PlaylistItem*)QListViewItemIterator::operator*(); }
};

typedef MyIterator MyIt;

namespace Glow
{
    namespace Text
    {
        static float dr, dg, db;
        static int    r, g, b;
    }
    namespace Base
    {
        static float dr, dg, db;
        static int    r, g, b;
    }

    static const uint STEPS = 13;
    static uint counter;
    static QTimer timer;

    inline void startTimer()
    {
        counter = 0;
        timer.start( 40 );
    }

    inline void reset()
    {
        counter = 0;
    }
}


Playlist *Playlist::s_instance = 0;


Playlist::Playlist( QWidget *parent, KActionCollection *ac, const char *name )
    : KListView( parent, name )
    , m_currentTrack( 0 )
    , m_marker( 0 )
    , m_weaver( new ThreadWeaver( this ) )
    , m_firstColumn( 0 )
    , m_undoDir( KGlobal::dirs()->saveLocation( "data", "amarok/undo/", true ) )
    , m_undoCounter( 0 )
    , m_editText( 0 )
    , m_ac( ac ) //REMOVE
{
    s_instance = this;

    EngineController* const ec = EngineController::instance();
    ec->attach( this );
    connect( ec, SIGNAL(orderPrevious()), SLOT(playPrevTrack()) );
    connect( ec, SIGNAL(orderNext()),     SLOT(playNextTrack()) );
    connect( ec, SIGNAL(orderCurrent()),  SLOT(playCurrentTrack()) );


    setShowSortIndicator( true );
    setDropVisualizer( false );   //we handle the drawing for ourselves
    setDropVisualizerWidth( 3 );
    setItemsRenameable( true );
    KListView::setSorting( NO_SORT ); //use base so we don't saveUndoState() too
    setAcceptDrops( true );
    setSelectionMode( QListView::Extended );
    setAllColumnsShowFocus( true );
    //setItemMargin( 3 ); adds margin to ALL sides, not just left and right. DAMN!
    //setDefaultRenameAction( QListView::Reject ); //FIXME Qt says this is the default anyway!

    //NOTE order is critical because we can't set indexes or ids
    addColumn( i18n( "Track Name" ),   0 );
    addColumn( i18n( "Title"      ), 200 ); //displays trackname if no title tag
    addColumn( i18n( "Artist"     ), 100 );
    addColumn( i18n( "Album"      ), 100 );
    addColumn( i18n( "Year"       ),   0 ); //0 means hidden
    addColumn( i18n( "Comment"    ),   0 );
    addColumn( i18n( "Genre"      ),   0 );
    addColumn( i18n( "Track"      ),   0 );
    addColumn( i18n( "Directory"  ),   0 );
    addColumn( i18n( "Length"     ),  80 );
    addColumn( i18n( "Bitrate"    ),   0 );

    setRenameable( 0, false ); //TODO allow renaming of the filename
    setRenameable( 1 );
    setRenameable( 2 );
    setRenameable( 3 );
    setRenameable( 4 );
    setRenameable( 5 );
    setRenameable( 6 );
    setRenameable( 7 );
    setColumnAlignment(  7, Qt::AlignCenter ); //track
    setColumnAlignment(  9, Qt::AlignRight ); //length
    setColumnAlignment( 10, Qt::AlignCenter ); //bitrate


    connect( this,     SIGNAL( doubleClicked( QListViewItem* ) ),
             this,       SLOT( activate( QListViewItem* ) ) );
    connect( this,     SIGNAL( returnPressed( QListViewItem* ) ),
             this,       SLOT( activate( QListViewItem* ) ) );
    connect( this,     SIGNAL( mouseButtonPressed( int, QListViewItem*, const QPoint&, int ) ),
             this,       SLOT( slotMouseButtonPressed( int, QListViewItem*, const QPoint&, int ) ) );
    connect( this,     SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
             this,       SLOT( writeTag( QListViewItem*, const QString&, int ) ) );
    connect( this,     SIGNAL( aboutToClear() ),
             this,       SLOT( saveUndoState() ) );

    connect( &Glow::timer, SIGNAL(timeout()), SLOT(slotGlowTimer()) );


    KStdAction::copy( this, SLOT( copyToClipboard() ), ac, "playlist_copy" );
    KStdAction::selectAll( this, SLOT( selectAll() ), ac, "playlist_select_all" );
    m_clearButton = KStdAction::clear( this, SLOT( clear() ), ac, "playlist_clear" );
    m_undoButton  = KStdAction::undo( this, SLOT( undo() ), ac, "playlist_undo" );
    m_redoButton  = KStdAction::redo( this, SLOT( redo() ), ac, "playlist_redo" );
    new KAction( i18n( "S&huffle" ), "rebuild", CTRL+Key_H, this, SLOT( shuffle() ), ac, "playlist_shuffle" );
    new KAction( i18n( "&Goto Current" ), "today", CTRL+Key_Enter, this, SLOT( showCurrentTrack() ), ac, "playlist_show" );


    //ensure we update action enabled states when repeat Playlist is toggled
    connect( ac->action( "repeat_playlist" ), SIGNAL(toggled( bool )), SLOT(updateNextPrev()) );


    m_clearButton->setIcon( "view_remove" );
    m_undoButton->setEnabled( false );
    m_redoButton->setEnabled( false );


    engineStateChanged( EngineController::engine()->state() ); //initialise state of UI
    paletteChange( palette() ); //sets up glowColors
    restoreLayout( KGlobal::config(), "PlaylistColumnsLayout" );
    columnOrderChanged();


    header()->installEventFilter( this );
}

Playlist::~Playlist()
{
    saveLayout( KGlobal::config(), "PlaylistColumnsLayout" );

    if( m_weaver->running() )
    {
        kdDebug() << "[weaver] Halting jobs..\n";
        m_weaver->halt();
        m_weaver->wait();
    }

    delete m_weaver;

    if( AmarokConfig::savePlaylist() ) saveXML( defaultPlaylistPath() );

    EngineController::instance()->detach( this );

    //clean undo directory
    QStringList list = m_undoDir.entryList();
    for( QStringList::ConstIterator it = list.constBegin(), end = list.constEnd(); it != end; ++it )
        m_undoDir.remove( *it );

    //speed up quit a little
    KListView::clear();   //our implementation is slow
    blockSignals( true ); //might help
}



////////////////////////////////////////////////////////////////////////////////
/// Media Handling
////////////////////////////////////////////////////////////////////////////////

void
Playlist::appendMedia( KURL::List list, bool directPlay, bool preventDoubles )
{
    //NOTE passing by value is quick for QValueLists, although it is slow if we change the list
    //but we only do that ocassionally

    if( preventDoubles )
    {
        KURL::List::Iterator jt;

        for( MyIt it( this, 0 ); *it; ++it )
        {
            jt = list.find( (*it)->url() );

            if( jt != list.end() )
            {
                if( directPlay && jt == list.begin() )
                {
                    directPlay = false;
                    activate( *it );
                }

                list.remove( jt );
            }
        }
    }

    insertMediaInternal( list, lastItem(), directPlay );
}

void
Playlist::queueMedia( const KURL::List &list, bool play )
{
    insertMediaInternal( list, currentTrack(), play );
}

void
Playlist::insertMediaInternal( const KURL::List &list, PlaylistItem *after, bool directPlay )
{
    //TODO directPlay handling

    if( list.count() == 1 )
    {
        //if safe just add it

        const KURL &url = list.front();

        if( PlaylistLoader::isPlaylist( url ) )
        {
            if( !url.isLocalFile() )
            {
                PlaylistLoader::downloadPlaylist( url, this, after );
                return;
            }
            //else use the normal loader route
        }
        else if( !QFileInfo( url.path() ).isDir() && EngineController::canDecode( url ) )
        {
            setSorting( NO_SORT );
            PlaylistItem *item = new PlaylistItem( url, this, after );
            item->setText( MetaBundle( url ) );
            if ( directPlay )
                activate( item );
            return;
        }
        //else go via the loader as that will present an error dialog
    }

    if( !list.isEmpty() )
    {
        setSorting( NO_SORT );
        (new PlaylistLoader( list, this, after, directPlay ))->start();
    }

    return;
}

QString
Playlist::defaultPlaylistPath() //static
{
    return KGlobal::dirs()->saveLocation( "data", "amarok/" ) + "current.xml";
}

void
Playlist::restoreSession()
{
    const QStringList list = m_undoDir.entryList( "*.xml", QDir::Files | QDir::Readable, QDir::Time );

    QString
    path  = KGlobal::dirs()->saveLocation( "data", "amarok/" );
    path += /*list.count() ? list.last() :*/ "current.xml"; //FIXME

    kdDebug() << "Session Playlist: " << path << endl;

    KURL url;
    url.setPath( path );
    (new PlaylistLoader( url, this, 0 ))->start();
}



////////////////////////////////////////////////////////////////////////////////
/// Current Track Handling
////////////////////////////////////////////////////////////////////////////////

void
Playlist::playNextTrack()
{
    PlaylistItem *item = currentTrack();

    if( !AmarokConfig::repeatTrack() )
    {
        if( !m_nextTracks.isEmpty() )
        {
            item = m_nextTracks.first();
            m_nextTracks.remove();
        }
        else if( AmarokConfig::randomMode() )
        {
            QValueVector<PlaylistItem*> tracks;

            //make a list of everything we can play
            for( MyIt it( this ); *it; ++it )
                if ( !m_prevTracks.containsRef( *it ) )
                    tracks.push_back( *it );

            if( tracks.isEmpty() )
            {
                //we have played everything

                if ( m_prevTracks.count() <= 40 ) {
                    m_prevTracks.clear();
                    m_prevTracks.append( m_currentTrack );
                }
                else {
                    m_prevTracks.first(); //set's current item to first item

                    //keep 40 tracks in the previous list so item time user pushes play
                    //we don't risk playing anything too recent
                    while( m_prevTracks.count() > 40 )
                        m_prevTracks.remove(); //removes current item
                }

                if( AmarokConfig::repeatPlaylist() )
                {
                    playNextTrack();
                    return;
                }
                //else we stop via activate( 0 ) below
            }
            else item = tracks.at( KApplication::random() % tracks.count() ); //is O(1)
        }
        else if( item )
        {
            item = *(MyIt&)++MyIt( item );
        }
        else item = *MyIt( this ); //ie. first visible item

        if ( !item && AmarokConfig::repeatPlaylist() )
            item = *MyIt( this ); //ie. first visible item
    }

    if ( EngineController::engine()->loaded() )
        activate( item );
    else
        setCurrentTrack( item );
}

void
Playlist::playPrevTrack()
{
    PlaylistItem *item = m_currentTrack;

    if ( !AmarokConfig::randomMode() || m_prevTracks.count() <= 1 )
        item = *(MyIt&)--MyIt( item );

    else {
        // if enough songs in buffer, jump to the previous one
        m_prevTracks.last();
        m_prevTracks.remove(); //remove the track playing now
        item = m_prevTracks.last();
    }

    if ( !item && AmarokConfig::repeatPlaylist() )
        item = *MyIt( lastItem() ); //TODO check this works!

    if ( EngineController::engine()->loaded() )
        activate( item );
    else
        setCurrentTrack( item );
}

void
Playlist::playCurrentTrack()
{
    activate( currentTrack() ? currentTrack() : *MyIt( this ) );
}

void
Playlist::activate( QListViewItem *item )
{
    //All internal requests for playback should come via
    //this function please!

    if( item )
    {
        #define item static_cast<PlaylistItem*>(item)

        m_prevTracks.append( item );

        //if we are playing something from the next tracks
        //list, remove it from the list
        if ( m_nextTracks.removeRef( item ) )
            refreshNextTracks();

        //looks bad painting selected and glowing
        //only do when user explicitely activates an item though
        item->setSelected( false );

        setCurrentTrack( item );

        EngineController::instance()->play( item );
        #undef item
    }
    else
    {
        //we have reached the end of the playlist

        setCurrentTrack( 0 );
        EngineController::instance()->stop();
    }
}

void
Playlist::setCurrentTrack( PlaylistItem *item )
{
    const bool canScroll = !renameLineEdit()->isVisible() && selectedItems().count() < 2; //FIXME O(n)

    //if nothing is current and then playback starts, we must show the currentTrack
    if( !m_currentTrack && canScroll ) ensureItemVisible( item ); //handles 0 gracefully

    if( AmarokConfig::playlistFollowActive() && m_currentTrack && item && canScroll )
    {
        // if old item in view and the new one isn't do scrolling
        int currentY = itemPos( m_currentTrack );

        if( currentY + m_currentTrack->height() <= contentsY() + visibleHeight()
            && currentY >= contentsY() )
        {
            const uint scrollMax = 2 * item->height();
            // Scroll towards the middle but no more than two lines extra
            uint scrollAdd = viewport()->height() / 2 - item->height();
            if( scrollAdd > scrollMax ) scrollAdd = scrollMax;

            int itemY = itemPos( item );
            int itemH = item->height();

            if( itemY + itemH > contentsY() + visibleHeight() ) // scroll down
            {
                setContentsPos( contentsX(), itemY - visibleHeight() + itemH + scrollAdd );
            }
            else if( itemY < contentsY() ) // scroll up
            {
                setContentsPos( contentsX(), itemY - scrollAdd );
            }
        }
    }

    PlaylistItem *prev = m_currentTrack;
    m_currentTrack = item;

    if ( prev ) {
        //reset to normal height
        prev->invalidateHeight();
        prev->setup();
        //remove pixmap in first column
        prev->setPixmap( m_firstColumn, QPixmap() );
    }

    //aesthetics and usability
    if ( item && currentItem() == item )
        setCurrentItem( item->itemBelow() );

    updateNextPrev();

    Glow::reset();
    slotGlowTimer();
}

PlaylistItem*
Playlist::restoreCurrentTrack()
{
    const KURL &url = EngineController::instance()->playingURL();

    if( !(m_currentTrack && m_currentTrack->url() == url) )
    {
        PlaylistItem* item;

        for( item = firstChild();
             item && item->url() != url;
             item = item->nextSibling() )
        {}

        setCurrentTrack( item ); //set even if NULL

        if( item )
           //display "Play" icon
           item->setPixmap( m_firstColumn, SmallIcon( "artsbuilderexecute" ) );
    }

    return m_currentTrack;
}

bool
Playlist::isTrackAfter() const
{
    //order is carefully crafted, remember count() is O(n)
    //TODO randomMode will end if everything is in prevTracks

    return !currentTrack() && !isEmpty() ||
            currentTrack() && currentTrack()->itemBelow() ||
            childCount() > 1 && ( AmarokConfig::randomMode() || AmarokConfig::repeatPlaylist() );
}

bool
Playlist::isTrackBefore() const
{
    //order is carefully crafted, remember count() is O(n)

    return !isEmpty() &&
           (
               currentTrack() && (currentTrack()->itemAbove() || AmarokConfig::repeatPlaylist() && childCount() > 1)
               ||
               AmarokConfig::randomMode() && childCount() > 1
           );
}

void
Playlist::updateNextPrev()
{
    m_ac->action( "prev" )->setEnabled( isTrackBefore() );
    m_ac->action( "next" )->setEnabled( isTrackAfter() );
}



////////////////////////////////////////////////////////////////////////////////
/// EngineObserver Reimplementation
////////////////////////////////////////////////////////////////////////////////

void
Playlist::engineNewMetaData( const MetaBundle &bundle, bool trackChanged )
{
    if ( m_currentTrack && !trackChanged )

        //if the track hasn't changed then this is a meta-data update
        m_currentTrack->setText( bundle );

    else
        //ensure the currentTrack is set correctly and highlight it
        restoreCurrentTrack();
}

void
Playlist::engineStateChanged( Engine::State state )
{
    switch( state )
    {
    case Engine::Playing:
        m_ac->action( "pause" )->setEnabled( true );
        m_ac->action( "stop" )->setEnabled( true );
        m_ac->action( "playlist_show" )->setEnabled( true );

        Glow::startTimer();

        if ( m_currentTrack )
            //display "Play" icon
            m_currentTrack->setPixmap( m_firstColumn, SmallIcon( "artsbuilderexecute" ) );

        break;

    case Engine::Empty:
        //TODO do this with setState() in PlaylistWindow?
        m_ac->action( "pause" )->setEnabled( false );
        m_ac->action( "stop" )->setEnabled( false );
        m_ac->action( "playlist_show" )->setEnabled( false );

        //leave the glow state at full colour
        Glow::reset();

        if ( m_currentTrack )
        {
            //remove pixmap in all columns
            QPixmap null;
            for( int i = 0; i < header()->count(); i++ )
                m_currentTrack->setPixmap( i, null );

            //reset glow state
            slotGlowTimer();
        }

        //FALL THROUGH

    default:
        Glow::timer.stop();
    }
}



////////////////////////////////////////////////////////////////////////////////
/// KListView Reimplementation
////////////////////////////////////////////////////////////////////////////////

void
Playlist::clear() //SLOT
{
    emit aboutToClear(); //will saveUndoState()

    setCurrentTrack( 0 );
    m_prevTracks.clear();
    m_nextTracks.clear();

    //TODO make it possible to tell when it is safe to not delay deletion
    //TODO you'll have to do the same as below for removeSelected() too.
    m_weaver->cancel(); //cancel all jobs in this weaver, no new events will be sent

    //now we have to ensure we don't delete the items before any events the weaver sent
    //have been processed, so we stick them in a QPtrList and delete it later
    QPtrList<QListViewItem> *list = new QPtrList<QListViewItem>;
    list->setAutoDelete( true );
    while( QListViewItem *item = firstChild() )
    {
        takeItem( item );
        list->append( item );
    }
    QApplication::postEvent( this, new QCustomEvent( QCustomEvent::Type(4000), list ) );

    emit itemCountChanged( childCount() );
}

void
Playlist::setSorting( int col, bool b )
{
    saveUndoState();

    KListView::setSorting( col, b );
}

void
Playlist::setColumnWidth( int col, int width )
{
    KListView::setColumnWidth( col, width );

    //FIXME this is because Qt doesn't by default disable resizing width 0 columns. GRRR!
    //NOTE  default column sizes are stored in default amarokrc so that restoreLayout() in ctor will
    //      call this function. This is necessary because addColumn() doesn't call setColumnWidth() GRRR!
    header()->setResizeEnabled( width != 0, col );
}

void
Playlist::columnOrderChanged() //SLOT
{
    const uint prevColumn = m_firstColumn;

    //determine first visible column
    for ( m_firstColumn = 0; m_firstColumn < header()->count(); m_firstColumn++ )
        if ( header()->sectionSize( header()->mapToSection( m_firstColumn ) ) )
            break;

    //convert to logical column
    m_firstColumn = header()->mapToSection( m_firstColumn );

    //force redraw of currentTrack
    if( m_currentTrack )
    {
        m_currentTrack->setPixmap( prevColumn, QPixmap() );
        m_currentTrack->setPixmap( m_firstColumn, SmallIcon( "artsbuilderexecute" ) );
    }
}

void
Playlist::paletteChange( const QPalette &p )
{
    using namespace Glow;

    QColor fg;
    QColor bg;

    {
        using namespace Base;

        const uint steps = STEPS+5+5; //so we don't fade all the way to base, and all the way up to highlight either

        fg = colorGroup().highlight();
        bg = colorGroup().base();

        dr = double(bg.red() - fg.red()) / steps;
        dg = double(bg.green() - fg.green()) / steps;
        db = double(bg.blue() - fg.blue()) / steps;

        r = fg.red() + int(dr*5.0); //we add 5 steps so the default colour is slightly different to highlight
        g = fg.green() + int(dg*5.0);
        b = fg.blue() + int(db*5.0);
    }

    {
        using namespace Text;

        const uint steps = STEPS+5; //so we don't fade all the way to base

        fg = colorGroup().highlightedText();
        bg = colorGroup().text();

        dr = double(bg.red() - fg.red()) / steps;
        dg = double(bg.green() - fg.green()) / steps;
        db = double(bg.blue() - fg.blue()) / steps;

        r = fg.red();
        g = fg.green();
        b = fg.blue();
    }

    Glow::counter = 63; //ensure color is set

    KListView::paletteChange( p );
}

void
Playlist::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() == viewport() || KURLDrag::canDecode( e ) );
}

void
Playlist::contentsDragMoveEvent( QDragMoveEvent* e )
{
    if( !e->isAccepted() ) return;

    //TODO decide, use this or what was here before? still have to include the Xlib header..
    const bool ctrlPressed= KApplication::keyboardModifiers() & ControlMask;

    //Get the closest item _before_ the cursor
    const QPoint p = contentsToViewport( e->pos() );
    QListViewItem *item = itemAt( p );
    if( !item || ctrlPressed ) item = lastItem();
    else if( p.y() - itemRect( item ).top() < (item->height()/2) ) item = item->itemAbove();

    if( item != m_marker )
    {
        //NOTE the if block prevents flicker
        //NOTE it is correct to set m_marker in the middle

        slotEraseMarker();
        m_marker = item;
        viewportPaintEvent( 0 );
    }
}

void
Playlist::contentsDragLeaveEvent( QDragLeaveEvent* )
{
    slotEraseMarker();
}

void
Playlist::contentsDropEvent( QDropEvent *e )
{
    //NOTE parent is always 0 currently, but we support it in case we start using trees
    QListViewItem *parent = 0;
    QListViewItem *after  = m_marker;

    if( !after ) findDrop( e->pos(), parent, after ); //shouldn't happen, but you never know!

    slotEraseMarker();

    if( e->source() == viewport() )
    {
        setSorting( NO_SORT ); //disableSorting and saveState()
        movableDropEvent( parent, after );

        updateNextPrev();

    } else {

        KURL::List list;
        if( KURLDrag::decode( e, list ) )
        {
            insertMediaInternal( list, (PlaylistItem*)after );
        }
        else e->ignore();
    }
}

QDragObject*
Playlist::dragObject()
{
    //TODO use of the map is pointless
    //just use another list in same order as kurl::list

    KURL::List list;
    QMap<QString,QString> map;

    for( MyIterator it( this, MyIterator::Selected ); *it; ++it )
    {
        const PlaylistItem *item = (PlaylistItem*)*it;
        const KURL &url = item->url();
        list += url;
        const QString key = url.isLocalFile() ? url.path() : url.url();
        map[ key ] = QString("%1;%2").arg( item->title() ).arg( item->seconds() );
    }

    //it returns a KURLDrag with a QMap containing the title and the length of the track
    //this is used by the playlistbrowser to insert tracks in playlists without re-reading tags
    return new KURLDrag( list, map, viewport() );
}

void
Playlist::viewportPaintEvent( QPaintEvent *e )
{
    if( e ) KListView::viewportPaintEvent( e ); //we call with 0 in contentsDropEvent()

    if( m_marker )
    {
        QPainter painter( viewport() );
        painter.fillRect( drawDropVisualizer( 0, 0, m_marker ), QBrush( colorGroup().highlight(), QBrush::Dense4Pattern ) );
    }
}

bool
Playlist::eventFilter( QObject *o, QEvent *e )
{
    #define me static_cast<QMouseEvent*>(e)

    if( o == header() && e->type() == QEvent::MouseButtonPress && me->button() == Qt::RightButton )
    {
        KPopupMenu popup;
        popup.setCheckable( true );
        popup.insertItem( i18n("Hide this Column"), 1 ); //TODO
        popup.insertTitle( i18n( "Other Playlist Columns" ) );

        for( int i = 0; i < columns(); ++i ) //columns() references a property
        {
            popup.insertItem( columnText( i ), i, i + 1 );
            popup.setItemChecked( i, columnWidth( i ) != 0 );
        }

        int col = popup.exec( static_cast<QMouseEvent *>(e)->globalPos() );

        if( col != -1 )
        {
            //TODO can result in massively wide column appearing!
            if( columnWidth( col ) == 0 )
            {
                adjustColumn( col );
                header()->setResizeEnabled( true, col );
            }
            else hideColumn( col );
        }

        //determine first visible column again, since it has changed
        columnOrderChanged();
        //eat event
        return TRUE;
    }

    // not in slotMouseButtonPressed because we need to disable normal usage.
    if( o == viewport() && e->type() == QEvent::MouseButtonPress && me->state() == Qt::ControlButton && me->button() == RightButton )
    {
        PlaylistItem *item = (PlaylistItem*)itemAt( me->pos() );

        if( item )
        {
            if( m_nextTracks.removeRef( item ) )
            {
                //m_nextTracks.current() is now item
                refreshNextTracks(); //will repaint from current()
            }
            else m_nextTracks.append( item );

            item->repaint(); //we need to repaint item in both cases

            updateNextPrev();
        }

        return TRUE; //yum!
    }

    //allow the header to process this
    return KListView::eventFilter( o, e );

    #undef me
}

void
Playlist::customEvent( QCustomEvent *e )
{
    //the threads send their results here for completion that is GUI-safe
    switch( e->type() )
    {
    case PlaylistLoader::Started:
        m_clearButton->setEnabled( false );
        m_undoButton->setEnabled( false );
        m_redoButton->setEnabled( false );
        break;

    case PlaylistLoader::Done:
        m_clearButton->setEnabled( true );
        m_undoButton->setEnabled( !m_undoList.isEmpty() );
        m_redoButton->setEnabled( !m_redoList.isEmpty() );

        //just in case the track that is playing is not set current
        restoreCurrentTrack();
        //necessary usually
        emit itemCountChanged( childCount() );
        //force redraw of currentTrack marker, play icon, etc.
        //setCurrentTrack( currentTrack() );

        {
            KURL::List &list = static_cast<PlaylistLoader::DoneEvent*>(e)->badURLs();

            if( !list.isEmpty() )
            {
                amaroK::StatusBar::instance()->message( i18n("Some URLs could not be loaded."), 4000 );
//                 KMessageBox::error( this, i18n("Some URLs could not be loaded.") );//TODO details dialog

                for( KURL::List::ConstIterator it = list.begin(); it != list.end(); ++it )
                    kdDebug() << *it << endl;
            }
        }
        break;

    case PlaylistLoader::Play:
        activate( (PlaylistItem*)e->data() );
        break;

    case PlaylistLoader::Tags:
        #define e static_cast<PlaylistLoader::TagsEvent*>(e)
        e->item->setText( e->bundle );
        #undef e
        break;

    case ThreadWeaver::Started:

        QApplication::setOverrideCursor( KCursor::workingCursor() );
        break;

    case ThreadWeaver::Done:

        QApplication::restoreOverrideCursor();
        break;

    case ThreadWeaver::Job::GenericJob:
        #define e static_cast<ThreadWeaver::Job*>(e)
        e->completeJob();
        #undef e
        break;

    case 4000: //dustbinEvent

        //this is a list of all the listItems from a clear operation
        #define list static_cast<QPtrList<QListViewItem>*>(e->data())
        delete list;
        #undef list
        break;

    default:
        ;
    }
}



////////////////////////////////////////////////////////////////////////////////
/// Misc Public Methods
////////////////////////////////////////////////////////////////////////////////

void
Playlist::saveM3U( const QString &path ) const
{
    QFile file( path );

    if( file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        stream << "#EXTM3U\n";

        for( const PlaylistItem *item = firstChild(); item; item = item->nextSibling() )
        {
            const KURL url = item->url();

            stream << "#EXTINF:";
            stream << item->seconds();
            stream << ',';
            stream << item->title();
            stream << '\n';
            stream << (url.protocol() == "file" ? url.path() : url.url());
            stream << "\n";
        }
        file.close();
    }
}

void
Playlist::saveXML( const QString &path ) const
{
    //TODO save nextTrack queue
    QFile file( path );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument newdoc;
    QDomElement playlist = newdoc.createElement( "playlist" );
    playlist.setAttribute( "product", "amaroK" );
    playlist.setAttribute( "version", APP_VERSION );
    newdoc.appendChild( playlist );

    for( const PlaylistItem *item = firstChild(); item; item = item->nextSibling() )
    {
        QDomElement i = newdoc.createElement("item");
        i.setAttribute("url", item->url().prettyURL());

        for( int x = 1; x < columns(); ++x )
        {
            if( !item->exactText(x).isEmpty() )
            {
                QDomElement attr = newdoc.createElement( item->columnName(x) );
                QDomText t = newdoc.createTextNode( item->exactText(x) );
                attr.appendChild( t );
                i.appendChild( attr );
            }
        }

        playlist.appendChild( i );
    }

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << newdoc.toString();
    file.close();
}

void
Playlist::shuffle() //SLOT
{
    QPtrList<QListViewItem> list;
    KRandomSequence seq( (long)KApplication::random() );

    setSorting( NO_SORT );

    //first take nextTracks
    for( PlaylistItem *item = m_nextTracks.first(); item; item = m_nextTracks.next() )
    {
        takeItem( item );
    }

    //remove rest
    while( QListViewItem *first = firstChild() )
    {
        list.append( first );
        takeItem( first );
    }

    //shuffle the rest
    seq.randomize( &list );

    //reinsert rest
    for( QListViewItem *item = list.first(); item; item = list.next() )
    {
        insertItem( item );
    }

    //now put nextTracks into playlist so they are first and from first to last
    for( PlaylistItem *item = m_nextTracks.last(); item; item = m_nextTracks.prev() )
    {
        insertItem( item );
    }

    m_nextTracks.clear();

    updateNextPrev();
}

void
Playlist::removeSelectedItems() //SLOT
{
    setSelected( currentItem(), true );     //remove currentItem, no matter if selected or not

    //assemble a list of what needs removing
    //calling removeItem() iteratively is more efficient if they are in _reverse_ order, hence the prepend()
    QPtrList<QListViewItem> list;
    for( MyIterator it( this, MyIterator::Selected );
         it.current();
         list.prepend( it.current() ), ++it );

    if( list.isEmpty() ) return;
    saveUndoState();

    //remove the items, unless the weaver is running, in which case it is safest to just hide them
    for( QListViewItem *item = list.first(); item; item = list.next() )
    {
        removeItem( (PlaylistItem*)item );

        //if tagreader is running don't let tags be read for this item and delete later
        if( m_weaver->running() )
        {
            //FIXME this will cause childCount() to report wrongly
            //FIXME make a customEvent to deleteLater(), can't use QObject::deleteLater() as we don't inherit QObject!
            item->setVisible( false ); //will be removed next time playlist is cleared
            //FIXME m_weaver->remove( item );
        }
        else delete item;
    }

    updateNextPrev();

    //NOTE no need to emit childCountChanged(), removeItem() does that for us
}

void
Playlist::deleteSelectedFiles() //SLOT
{
    //NOTE we assume that currentItem is the main target
    int count  = selectedItems().count();
    int button = KMessageBox::warningContinueCancel( this,
                    i18n( "<p>You have selected %1 to be <b>irreversibly</b> deleted." ).
                        arg( count > 1 ?
                            i18n( "1 file", "<u>%n files</u>", count ) : //we must use this form of i18n()
                            static_cast<PlaylistItem*>(currentItem())->url().prettyURL().prepend("<i>'").append("'</i>") ),
                    QString::null,
                    i18n("&Delete") );

    if ( button == KMessageBox::Continue )
    {
        setSelected( currentItem(), true );     //remove currentItem, no matter if selected or not

        KURL::List urls;

        //assemble a list of what needs removing
        for( MyIterator it( this, MyIterator::Visible | MyIterator::Selected );
             it.current();
             urls << static_cast<PlaylistItem*>( *it )->url(), ++it );

        if ( urls.isEmpty() ) return;

        // TODO We need to check which files have been deleted successfully
        KIO::DeleteJob* job = KIO::del( urls );
        connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( removeSelectedItems() ) );
    }
}

void
Playlist::removeDuplicates() //SLOT
{
    QSortedList<PlaylistItem> list;
    for( QListViewItemIterator it( this ); it.current(); ++it )
        list.prepend( (PlaylistItem*)it.current() );

    list.sort();

    QPtrListIterator<PlaylistItem> it( list );
    PlaylistItem *item;
    while( (item = it.current()) ) {
        const KURL &compare = item->url();
        ++it;
        if ( *it && compare == it.current()->url() ) {
            removeItem( item );
            delete item;
        }
    }
}

void
Playlist::copyToClipboard( const QListViewItem *item ) const //SLOT
{
    if( !item ) item = currentTrack();

    if( item )
    {
        const PlaylistItem* playlistItem = static_cast<const PlaylistItem*>( item );

        QString text = MetaBundle( playlistItem ).prettyTitle();
        // For streams add the streamtitle too
        //TODO make prettyTitle do this
        if ( playlistItem->url().protocol() == "http" ) text.prepend( playlistItem->title() + " :: " );

        // Copy both to clipboard and X11-selection
        QApplication::clipboard()->setText( text, QClipboard::Clipboard );
        QApplication::clipboard()->setText( text, QClipboard::Selection );

        amaroK::OSD::instance()->showOSD( i18n( "Copied: %1" ).arg( text ) );
    }
}

void Playlist::undo() { switchState( m_undoList, m_redoList ); } //SLOT
void Playlist::redo() { switchState( m_redoList, m_undoList ); } //SLOT

void
Playlist::showContextMenu( QListViewItem *item, const QPoint &p, int col ) //SLOT
{
    #define item static_cast<PlaylistItem*>(item)

    enum Id { PLAY, PLAY_NEXT, VIEW, EDIT, FILL_DOWN, COPY, REMOVE };

    if( item == NULL ) return; //technically we should show "Remove" but this is far neater

    const bool canRename   = isRenameable( col );
    const bool isCurrent   = (item == m_currentTrack);
    const bool isPlaying   = EngineController::engine()->state() == Engine::Playing;
    const int  queueIndex  = m_nextTracks.findRef( item );
    const bool isQueued    = queueIndex != -1;
    const uint itemCount   = selectedItems().count();
    const bool trackColumn = col == PlaylistItem::Track;
    const QString tagName  = columnText( col );
    const QString tag      = item->exactText( col );
    //Markey, sorry for the lengths of these lines! -mxcl

    KPopupMenu popup( this );

    popup.insertTitle( KStringHandler::rsqueeze( item->metaBundle().prettyTitle(), 50 ) );
    popup.insertItem( SmallIcon( "player_play" ), isCurrent && isPlaying ? i18n( "&Restart" ) : i18n( "&Play" ), 0, 0, Key_Enter, PLAY );

    if( !isQueued ) //not in nextTracks queue
    {
        QString nextText = i18n( "Play as &Next" );

        const uint nextIndex = m_nextTracks.count() + 1;
        if ( nextIndex > 1 )
            nextText += QString( " (%1)" ).arg( nextIndex );

        popup.insertItem( SmallIcon( "2rightarrow" ), nextText, PLAY_NEXT );
    }
    else popup.insertItem( SmallIcon( "2leftarrow" ), i18n( "&Dequeue (%1)" ).arg( queueIndex+1 ), PLAY_NEXT );

    popup.insertSeparator();
    popup.insertItem( SmallIcon( "edit" ), i18n( "&Edit Tag Inline: '%1'" ).arg( tagName ), 0, 0, Key_F2, EDIT );
    popup.insertItem( trackColumn ? i18n("&Iteratively Assign Track Numbers") : i18n("Write '%1' For Selected Tracks").arg( tag ), FILL_DOWN );
    popup.insertItem( SmallIcon( "editcopy" ), i18n( "&Copy Meta-string" ), 0, 0, CTRL+Key_C, COPY );
    popup.insertSeparator();
    popup.insertItem( SmallIcon( "edittrash" ), i18n( "&Remove From Playlist" ), this, SLOT(removeSelectedItems()), Key_Delete );
    popup.insertItem( SmallIcon( "editdelete" ), i18n("&Delete File", "&Delete Selected Files", itemCount ), this, SLOT(deleteSelectedFiles()), SHIFT+Key_Delete );
    popup.insertSeparator();
    popup.insertItem( SmallIcon( "info" ), i18n( "&View/Edit Meta Information..." ), VIEW ); //TODO rename properties


    popup.setItemEnabled( EDIT, canRename ); //only enable for columns that have editable tags
    popup.setItemEnabled( FILL_DOWN, canRename && itemCount > 1 );


    switch( popup.exec( p ) )
    {
    case PLAY:
        activate( item );
        break;

    case PLAY_NEXT:
        item->setSelected( false ); //for prettiness

        if( isQueued )
        {
            //remove the item, this is better way than remove( item )
            m_nextTracks.remove( queueIndex ); //sets current() to next item
            refreshNextTracks(); //from current()
        }
        else m_nextTracks.append( item );

        //NOTE "item" is repainted due to the setSelected() call

        updateNextPrev();
        break;

    case VIEW:
    {
        showTagDialog( item );
        break;
    }

    case EDIT:
        startEditTag( item, col );
        break;

    case FILL_DOWN:
        //Spreadsheet like fill-down
        {
            QString newTag = item->exactText( col );
            MyIterator it( this, MyIterator::Visible | MyIterator::Selected );

            //special handling for track column
            uint trackNo = (*it)->exactText( PlaylistItem::Track ).toInt(); //returns 0 if it is not a number

            //we should start at the next row if we are doing track number
            //and the first row has a number set
            if ( trackColumn && trackNo > 0 )
                ++it;

            for( ; *it; ++it )
            {
                if ( trackColumn )
                    //special handling for track column
                    newTag = QString::number( ++trackNo );

                else if ( *it == item )
                    //skip the one we are copying
                    continue;

                //FIXME fix this hack!
                if ( (*it)->exactText( col ) != i18n("Writing tag...") )
                    m_weaver->append( new TagWriter( this, *it, newTag, col ), true );
            }
        }
        break;

    case COPY:
        copyToClipboard( item );
        break;
    }

    #undef item
}



////////////////////////////////////////////////////////////////////////////////
/// Misc Private Methods
////////////////////////////////////////////////////////////////////////////////

int
Playlist::mapToLogicalColumn( int physical )
{
    int logical;

    //skip hidden columns
    do logical = header()->mapToSection( physical++ );
    while ( !header()->sectionSize( logical ) );

    return logical;
}

void
Playlist::removeItem( PlaylistItem *item )
{
    //this function ensures we don't have dangling pointers to items that are about to be removed
    //for some reason using QListView::takeItem() and QListViewItem::takeItem() was ineffective
    //NOTE we don't delete item for you! You must call delete item yourself :)

    //TODO there must be a way to do this without requiring notification from the item dtor!
    //NOTE orginally this was in ~PlaylistItem(), but that caused crashes due to clear() *shrug*
    //NOTE items already removed by takeItem() will crash if you call nextSibling() on them
    //     taken items return 0 from listView()
    //FIXME if you remove a series of items including the currentTrack and all the nextTracks
    //      then no new nextTrack will be selected and the playlist will resume from the begging
    //      next time

    if( m_currentTrack == item )
    {
        setCurrentTrack( 0 );

        //ensure the playlist doesn't start at the beginning after the track that's playing ends
        //we don't need to do that in random mode, it's getting randomly selected anyways
        if( m_nextTracks.isEmpty() && !AmarokConfig::randomMode() )
        {
            PlaylistItem* const next = *MyIt( item );
            m_nextTracks.append( next );
            repaintItem( next );
        }
    }

    //keep m_nextTracks queue synchronised
    if( m_nextTracks.removeRef( item ) ) refreshNextTracks();

    //keep recent buffer synchronised
    m_prevTracks.removeRef( item ); //removes all pointers to item

    emit itemCountChanged( childCount() );
}

void
Playlist::refreshNextTracks( int from )
{
    // This function scans the m_nextTracks list starting from the 'from'
    // position and from there on updates the progressive numbering on related
    // items and repaints them. In short it performs an update subsequent to
    // a renumbering/order changing at some point of the m_nextTracks list.

    //start on the 'from'-th item of the list

    for( PlaylistItem* item = (from == -1) ? m_nextTracks.current() : m_nextTracks.at( from );
         item;
         item = m_nextTracks.next() )
    {
        repaintItem( item );
    }
}

void
Playlist::saveUndoState() //SLOT
{
   if( saveState( m_undoList ) )
   {
      m_redoList.clear();

      m_undoButton->setEnabled( true );
      m_redoButton->setEnabled( false );
   }
}

bool
Playlist::saveState( QStringList &list )
{
    //used by undo system, save state of playlist to undo/redo list

   //do not change this! It's required by the undo/redo system to work!
   //if you must change this, fix undo/redo first. Ask me what needs fixing <mxcl>
   if( !isEmpty() )
   {
      QString fileName;
      m_undoCounter %= AmarokConfig::undoLevels();
      fileName.setNum( m_undoCounter++ );
      fileName.prepend( m_undoDir.absPath() + "/" );
      fileName.append( ".xml" );

      if ( list.count() >= (uint)AmarokConfig::undoLevels() )
      {
         m_undoDir.remove( list.first() );
         list.pop_front();
      }

      saveXML( fileName );
      list.append( fileName );

      return true;
   }

   return false;
}

void
Playlist::switchState( QStringList &loadFromMe, QStringList &saveToMe )
{
    //switch to a previously saved state, remember current state
    KURL url; url.setPath( loadFromMe.last() );
    loadFromMe.pop_back();

    //save current state
    saveState( saveToMe );

    //blockSignals so that we don't cause a saveUndoState()
    //FIXME, but this will stop the search lineEdit from being cleared..
    blockSignals( true );
      clear();
    blockSignals( false );

    appendMedia( url ); //because the listview is empty, undoState won't be forced

    m_undoButton->setEnabled( !m_undoList.isEmpty() );
    m_redoButton->setEnabled( !m_redoList.isEmpty() );
}

void
Playlist::slotMouseButtonPressed( int button, QListViewItem *after, const QPoint &p, int col ) //SLOT
{
    switch( button )
    {
    case Qt::MidButton:
    {
        const QString path = QApplication::clipboard()->text( QClipboard::Selection );

        kdDebug() << "[playlist] X11 Paste: " << path << endl;

        const KURL url = KURL::fromPathOrURL( path );

        if ( EngineController::engine()->canDecode( url ) )
            insertMediaInternal( url, (PlaylistItem*)(after ? after : lastItem()) );

        break;
    }

    case Qt::RightButton:
        showContextMenu( after, p, col );
        break;

    default:
        ;
    }
}

void
Playlist::slotGlowTimer() //SLOT
{
    if( !currentTrack() ) return;

    using namespace Glow;

    if( counter <= STEPS*2 )
    {
        // 0 -> STEPS -> 0
        const double d = (counter > STEPS) ? 2*STEPS-counter : counter;

        {
            using namespace Base;
            PlaylistItem::glowBase = QColor( r + int(d*dr), g + int(d*dg), b + int(d*db) );
        }

        {
            using namespace Text;
            PlaylistItem::glowText = QColor( r + int(d*dr), g + int(d*dg), b + int(d*db) );
        }

        repaintItem( currentTrack() );
    }

    ++counter &= 63; //built in bounds checking with &=
}

void
Playlist::slotTextChanged( const QString &query ) //SLOT
{
    //TODO allow a slight delay before searching
    //TODO if we provided the lineEdit m_lastSearch would be unecessary

    const QString loweredQuery = query.lower();
    const QStringList terms = QStringList::split( ' ', loweredQuery );
    PlaylistItem *item = 0;
    MyIterator it( this, loweredQuery.startsWith( m_lastSearch ) ? MyIterator::Visible : 0 );

    while( (item = (PlaylistItem*)it.current()) )
    {
        bool listed = true;

        //if query is empty skip the loops and show all items
        if( !query.isEmpty() )
        {
            for( uint x = 0; listed && x < terms.count(); ++x ) //v.count() is constant time
            {
                bool b = false;

                //search in Trackname, Artist, Songtitle, Album
                for( uint y = 0; !b && y < 4; ++y )
                    b = item->exactText( y ).lower().contains( terms[x] );

                // exit loop, when one of the search tokens doesn't match
                if( !b ) listed = false;
            }
        }

        item->setVisible( listed );
        ++it;
    }

    m_lastSearch = loweredQuery;

    //to me it seems sensible to do this, BUT if it seems annoying to you, remove it
    showCurrentTrack();
    clearSelection(); //we do this because QListView selects inbetween visible items, this is a non ideal solution
    triggerUpdate();
}

void
Playlist::slotEraseMarker() //SLOT
{
    if( m_marker )
    {
        const QRect spot = drawDropVisualizer( 0, 0, m_marker );
        m_marker = 0;
        viewport()->repaint( spot, false );
    }
}

void
Playlist::startEditTag( QListViewItem *item, int column )
{
    KLineEdit *edit = renameLineEdit();

    switch( column )
    {
        case PlaylistItem::Artist:
            edit->completionObject()->setItems( CollectionDB().artistList() );
            break;

        case PlaylistItem::Album:
            edit->completionObject()->setItems( CollectionDB().albumList() );
            break;

        case PlaylistItem::Genre:
            edit->completionObject()->setItems( MetaBundle::genreList() );
            break;

        default:
            edit->completionObject()->clear();
            break;
    }

    edit->completionObject()->setCompletionMode( KGlobalSettings::CompletionPopupAuto );

    m_editText = static_cast<PlaylistItem *>(item)->exactText( column );

    rename( item, column );
}

void
Playlist::writeTag( QListViewItem *lvi, const QString &tag, int col ) //SLOT
{
    if( m_editText != tag && !(m_editText.isEmpty() && tag.isEmpty()) )    //write the new tag only if it's changed
        m_weaver->append( new TagWriter( this, (PlaylistItem *)lvi, tag, col ), true );

    QListViewItem *below = lvi->itemBelow();
    //FIXME will result in nesting of this function?
    if( below && below->isSelected() ) { rename( below, col ); }
}

void Playlist::showTagDialog( PlaylistItem* item )
{
    TagDialog* dialog = new TagDialog( item->metaBundle(), item, instance() );
    dialog->show();
}

#include "playlist.moc"
