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


#include "amarokconfig.h"
#include "metabundle.h"
#ifdef PLAYLIST_BROWSER
    #include "playlistbrowser.h"
#endif
#include "playlistitem.h"
#include "playlistloader.h"
#include "playlist.h"
#include "threadweaver.h"
#include "enginecontroller.h"

#include <qclipboard.h> //copyToClipboard(), slotMouseButtonPressed()
#include <qcolor.h>
#include <qevent.h>
#include <qfile.h>
#include <qheader.h> //eventFilter()
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpen.h>    //slotGlowTimer()
#include <qpoint.h>
#include <qrect.h>
#include <qtimer.h>

#include <kapplication.h> //use of kapp
#include <kaction.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kiconloader.h> //slotShowContextMenu()
#include <klineedit.h>   //setCurrentTrack()
#include <klocale.h>
#include <kpopupmenu.h>
#include <krandomsequence.h> //random Mode
#include <kstandarddirs.h>   //KGlobal::dirs()
#include <kstdaction.h>
#include <kurldrag.h>

#include <X11/Xlib.h>  // for XQueryPointer



Playlist::Playlist( QWidget *parent, KActionCollection *ac, const char *name )
    : KListView( parent, name )
#ifdef PLAYLIST_BROWSER
    , m_browser( new PlaylistBrowser( "PlaylistBrowser" ) )
#endif
    , m_currentTrack( 0 )
    , m_cachedTrack( 0 )
    , m_marker( 0 )
    , m_glowTimer( new QTimer( this ) )
    , m_weaver( new ThreadWeaver( this ) )
    , m_undoButton( KStdAction::undo( this, SLOT( undo() ), ac, "playlist_undo" ) )
    , m_redoButton( KStdAction::redo( this, SLOT( redo() ), ac, "playlist_redo" ) )
    , m_clearButton( 0 )
    , m_undoDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) )
    , m_undoCounter( 0 )
    , m_ac( ac ) //we use this so we don't have to include playerapp.h
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    EngineController* const ec = EngineController::instance();

    // we want to receive engine updates
    ec->attach( this );


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
    addColumn( i18n( "Trackname" ),   0 );
    addColumn( i18n( "Title"     ), 200 ); //displays trackname if no title tag
    addColumn( i18n( "Artist"    ), 100 );
    addColumn( i18n( "Album"     ), 100 );
    addColumn( i18n( "Year"      ),   0 ); //0 means hidden
    addColumn( i18n( "Comment"   ),   0 );
    addColumn( i18n( "Genre"     ),   0 );
    addColumn( i18n( "Track"     ),   0 );
    addColumn( i18n( "Directory" ),   0 );
    addColumn( i18n( "Length"    ),  80 );
    addColumn( i18n( "Bitrate"   ),   0 );

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


    connect( this, SIGNAL( contentsMoving( int, int ) ),
             this,   SLOT( slotEraseMarker() ) );
    connect( this, SIGNAL( doubleClicked( QListViewItem* ) ),
             this,   SLOT( activate( QListViewItem* ) ) );
    connect( this, SIGNAL( returnPressed( QListViewItem* ) ),
             this,   SLOT( activate( QListViewItem* ) ) );
    connect( this, SIGNAL( mouseButtonPressed( int, QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( slotMouseButtonPressed( int, QListViewItem*, const QPoint&, int ) ) );
    connect( this, SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
             this,   SLOT( writeTag( QListViewItem*, const QString&, int ) ) );
    connect( this, SIGNAL( aboutToClear() ),
             this,   SLOT( saveUndoState() ) );

    //FIXME causes problems with saving playlists
    //connect( header(), SIGNAL(sizeChange( int, int, int )), SLOT(slotHeaderResized( int, int, int )) );

    connect( m_glowTimer, SIGNAL( timeout() ), this, SLOT( slotGlowTimer() ) );


    //TODO deprecate these, use a better system
    connect( ec, SIGNAL( orderPrevious() ), SLOT( handleOrderPrev() ) );
    connect( ec, SIGNAL( orderCurrent() ), SLOT( handleOrderCurrent() ) );
    connect( ec, SIGNAL( orderNext() ), SLOT( handleOrder() ) );


    //<init undo/redo>
        //create undo buffer directory
        if( !m_undoDir.exists( "undo", false ) )
             m_undoDir.mkdir( "undo", false );
        m_undoDir.cd( "undo" );

        //clean directory
        QStringList dirList = m_undoDir.entryList();
        for( QStringList::Iterator it = dirList.begin(); it != dirList.end(); ++it )
            m_undoDir.remove( *it );

        m_undoButton->setEnabled( false );
        m_redoButton->setEnabled( false );
    //</init undo/redo>


    KStdAction::copy( this, SLOT( copyToClipboard() ), ac, "playlist_copy" );
    new KAction( i18n( "Shu&ffle" ), "rebuild", CTRL+Key_H, this, SLOT( shuffle() ), ac, "playlist_shuffle" );
    new KAction( i18n( "&Goto Current" ), "today", CTRL+Key_Enter, this, SLOT( showCurrentTrack() ), ac, "playlist_show" );
    m_clearButton = new KAction( i18n( "&Clear" ), "view_remove", 0, this, SLOT( clear() ), ac, "playlist_clear" );

    ac->action( "playlist_show" )->setEnabled( false ); //FIXME instead get the engineController to set state on registration!


    header()->installEventFilter( this );

    restoreLayout( KGlobal::config(), "PlaylistColumnsLayout" );

    paletteChange( palette() ); //sets up glowColors

    /*
        TODO the following actually works!
             so we should be able to take a qlistviewitem from the view and hand that
             to loader threads, then they can actually make playlistitems in the thread
             rather than post just data here for manufacture
        NOTE is it worth it?

    QListViewItem *fred = new PlaylistItem( this, 0, KURL(), "moo", 9 );
    takeItem( fred );
    new PlaylistItem( this, fred, KURL(), "moo2", 9 );

    insertItem( fred );
    */

    kdDebug() << "END " << k_funcinfo << endl;
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

   //speed up quit a little
   KListView::clear();
   blockSignals( true );
}



//PUBLIC INTERFACE ===================================================

namespace Glow
{
    namespace Text
    {
        static double dr, dg, db;
        static int    r, g, b;
    }
    namespace Base
    {
        static double dr, dg, db;
        static int    r, g, b;
    }

    static const uint STEPS = 13;
    static uint counter;
}


//QWidget *Playlist::browser() const { return m_browser; }
void Playlist::showCurrentTrack() { ensureItemVisible( currentTrack() ); } //SLOT


QString Playlist::defaultPlaylistPath() //static
{
    return KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "current.xml";
}


void Playlist::insertMedia( KURL::List list, bool directPlay, bool preventDoubles )
{
    if ( preventDoubles )
        for ( QListViewItemIterator it( this ); it.current(); ++it )
        {
            PlaylistItem *tItem = (PlaylistItem *)it.current();
            if ( list.contains( tItem->url() ) )
            {
                list.remove( tItem->url() );
                
                if ( directPlay )
                    activate ( tItem );
            }
        }

    if( !list.isEmpty() )
    {
        //FIXME lastItem() scales badly!
        insertMediaInternal( list, lastItem(), directPlay );
    }
}


void Playlist::handleOrderPrev()    { handleOrder( Prev ); }    //SLOT
void Playlist::handleOrderCurrent() { handleOrder( Current ); } //SLOT


void Playlist::handleOrder( RequestType request ) //SLOT
{
    //NOTE PLEASE only modify this function with EXTREME care!
    //     most modifications ever have caused regressions you WILL not expect!

    if( isEmpty() )
    {
        activate( NULL );
        return;
    }

    PlaylistItem *item = currentTrack();
    if( !item ) request = Next;

    switch( request )
    {
    case Prev:

        //FIXME since 1.172 changes prevTracks is never empty so repeatPlaylist
        //      backwards never occurs

        //FIXME pre 1.172 we didn't need to store the currentTrack in the prevTracks list,
        //      which made the code simpler

        if ( m_prevTracks.count() < 2 )
        {
            item = (PlaylistItem *)item->itemAbove();
            if( !item && AmarokConfig::repeatPlaylist() ) item = lastItem();
        }
        else
        {
            // if enough songs in buffer, jump to the previous one
            {
                item = m_prevTracks.at( 1 );
                m_prevTracks.remove( m_prevTracks.at( 0 ) );
            }
        }

        activate( item, false ); //don't append this to the prevTrack stack, that _would_ be daft!
        return;

    case Current:

        if( item ) break; //then restart this track = amaroK behavior for pushing play button

        //else FALL THROUGH

    case Next:

        //FIXME make me pretty!

        if( !(AmarokConfig::repeatTrack() && item ) )
        {
            if( !m_nextTracks.isEmpty() )
            {
                item = m_nextTracks.first();
                m_nextTracks.remove();
            }
            else if( AmarokConfig::randomMode() )
            {
                // FIXME: anyone knows a more decent way to count visible items?
                uint visCount = 0;
                for ( QListViewItemIterator it( this, QListViewItemIterator::Visible ); it.current(); ++it )
                    ++visCount;

                uint x = 0;
                uint rnd = KApplication::random() % visCount;
                kdDebug() << rnd << endl;
                for ( QListViewItemIterator it( this, QListViewItemIterator::Visible ); it.current() && x <= rnd; ++it )
                {
                    if ( rnd == x )
                    {
                        item = (PlaylistItem*)it.current();

                        // check if this file was played lately
                        PlaylistItem *tItem = item;
                        while ( m_prevTracks.contains( item ) )
                        {
                            // song was already played, let's cycle through the list to find an unplayed one
                            item = (PlaylistItem*)item->itemBelow();

                            if ( !item )  // end of list, jump to the beginning
                                item = firstChild();

                            // did we cycle around completely? if so, every song was already played once.
                            // fallback to random output, then
                            if ( item == tItem )
                                // we only pick another song, if playlist repeating
                                // is enabled, otherwise we're done
                                if ( AmarokConfig::repeatPlaylist() )
                                {
                                    // be sure that no song gets played twice in a row
                                    if ( item == currentTrack() )
                                        item = (PlaylistItem*)item->itemBelow();
                                    if ( !item )
                                      item = firstChild();

                                    break;
                                }
                                else
                                {
                                    item = 0;
                                    break;
                                }
                        }
                    }
                    x++;
                }
            }
            else if( item )
            {
                item = (PlaylistItem*)item->itemBelow();

                if( !item && AmarokConfig::repeatPlaylist() ) item = firstChild();
            }
            else item = firstChild();
        }
    }

    activate( item );
}


void Playlist::saveM3U( const QString &path ) const
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


void Playlist::saveXML( const QString &path ) const
{
    //TODO save nextTrack queue
    QFile file( path );
    QDomDocument newdoc;

    if( !file.open( IO_WriteOnly ) ) return;
    QTextStream stream( &file );
    stream.setEncoding(QTextStream::UnicodeUTF8);

    stream << "<?xml version=\"1.0\" encoding=\"utf8\"?>\n";

    QDomElement playlist = newdoc.createElement("playlist");
    playlist.setAttribute("product", "amaroK");
    playlist.setAttribute("version", "1");
    newdoc.appendChild(playlist);

    for( const PlaylistItem *item = firstChild(); item; item = item->nextSibling() )
    {
        QDomElement i = newdoc.createElement("item");
        i.setAttribute("url", item->url().url());

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

        playlist.appendChild(i);
    }

    stream << newdoc.toString();
    file.close();
}


void Playlist::shuffle() //SLOT
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

    m_ac->action( "prev" )->setEnabled( isTrackBefore() );
    m_ac->action( "next" )->setEnabled( isTrackAfter() );
}


void Playlist::clear() //SLOT
{
    emit aboutToClear(); //will saveUndoState()

    setCurrentTrack( NULL );
    m_prevTracks.clear();
    m_nextTracks.clear();

    //TODO make it possible to tell when it is safe to not delay deletion
    //TODO you'll have to do the same as below for removeSelected() too.
    m_weaver->cancel(); //cancel all jobs in this weaver, no new events will be sent

    //now we have to ensure we don't delete the items before any events the weaver sent
    //have been processed, so we stick them in a QPtrList and delete it later
    QPtrList<QListViewItem> *list = new QPtrList<QListViewItem>;
    list->setAutoDelete( true );
    while( QListViewItem *item = firstChild() ) //FIXME check firstChild() is an efficient function here
    {
        takeItem( item );
        list->append( item );
    }
    QApplication::postEvent( this, new QCustomEvent( QCustomEvent::Type(4000), list ) );
    emit itemCountChanged( childCount() );
}


bool Playlist::isTrackAfter() const
{
    return !isEmpty() && (
           !m_nextTracks.isEmpty() ||
           m_currentTrack && (
           AmarokConfig::repeatPlaylist() || m_currentTrack->itemBelow() ) );
}

bool Playlist::isTrackBefore() const
{
    return !isEmpty() && (
           !m_prevTracks.isEmpty() ||
           currentTrack() && (
           AmarokConfig::repeatPlaylist() || currentTrack()->itemAbove() ) );
}


void Playlist::removeSelectedItems() //SLOT
{
    setSelected( currentItem(), true );     //remove currentItem, no matter if selected or not

    //assemble a list of what needs removing
    //calling removeItem() iteratively is more efficient if they are in _reverse_ order
    QPtrList<QListViewItem> list;
    for( QListViewItemIterator it( this, QListViewItemIterator::Selected );
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
            //FIXME make a customEvent to deleteLater(), can't use QObject::deleteLater() as we don't inherit QObject!
            item->setVisible( false ); //will be removed next time playlist is cleared
            //FIXME m_weaver->remove( item );
        }
        else delete item;
    }
    emit itemCountChanged( childCount() );
}

/*
void Playlist::summary( QPopupMenu &popup ) const
{
    QStringList summary( currentTrack()->text( 0 ) );

    //TODO easier to return a popupmenu with slots connected :)

    //FIXME use const Iterators if poss
    for( QListViewItemIterator it( currentTrack() ); it.current(); --it )
        summary.prepend( (*it)->text( 0 ) );
    for( QListViewItemIterator it( currentTrack() ); it.current(); ++it )
        summary.append( (*it)->text( 0 ) );
}
*/

int Playlist::mapToLogicalColumn( int physical )
{
    int logical;

    //skip hidden columns
    do logical = header()->mapToSection( physical++ );
    while ( !header()->sectionSize( logical ) );

    return logical;
}


// PRIVATE METHODS ===============================================

void Playlist::insertMediaInternal( const KURL::List &list, PlaylistItem *after, bool directPlay )
{
    //we don't check list.isEmpty(), this is a private function so we shouldn't have to
    
    PlaylistLoader *loader = new PlaylistLoader( list, this, after );

    if( loader )
    {
        setSorting( NO_SORT ); //disable sorting and saveState()

        QApplication::postEvent( this, new QCustomEvent( PlaylistLoader::Started ) ); //see customEvent for explanation

        loader->setOptions( AmarokConfig::directoriesRecursively(),
                            directPlay,
                            0 ); //no longer used
        loader->start();
    }
    else kdDebug() << "[playlist] Unable to create loader-thread!\n";
}


void Playlist::slotHeaderResized( int /*section*/, int, int /*newSize*/ )
{
#if 0
    //TODO the header has some padding either side of the text, so the fontMetrics width
    //     is too small

    if( section == PlaylistItem::Track )
    {
        const QString text  = columnText( PlaylistItem::Track );
        const int     width = fontMetrics().width( i18n("Track") );

        if( text != "#" && newSize < width )
        {
            setColumnText( PlaylistItem::Track, "#" );

        } else if( text == "#" && newSize >= width ) {

            setColumnText( PlaylistItem::Track, i18n("Track") );
        }
    }
#endif
}


void Playlist::removeItem( PlaylistItem *item )
{
    //this function ensures we don't have dangling pointers to items that are about to be removed
    //for some reason using QListView::takeItem() and QListViewItem::takeItem() was ineffective
    //FIXME there must be a way to do this without requiring notification from the item dtor!
    //NOTE  orginally this was in ~PlaylistItem(), but that caused crashes due to clear() *shrug*

    //NOTE items already removed by takeItem() will crash if you call nextSibling() on them
    //taken items return 0 from listView()

    //FIXME if you remove a series of items including the currentTrack and all the nextTracks
    //      then no new nextTrack will be selected and the playlist will resume from the begging
    //      next time

    if( item == m_currentTrack )
    {
        setCurrentTrack( 0 );

        //ensure the playlist doesn't start at the beginning after the track that's playing ends
        //we don't need to do that in random mode, it's getting randomly selected anyways
        if( m_nextTracks.isEmpty() && !AmarokConfig::randomMode() )
        {
            PlaylistItem *nextItem = item->nextSibling();
            m_nextTracks.append( nextItem );
            repaintItem( nextItem );
        }
    }

    if( item == m_cachedTrack ) m_cachedTrack = 0;

    //keep m_nextTracks queue synchronised
    if( m_nextTracks.findRef( item ) != -1 ) //sets List Current Item
    {
        m_nextTracks.remove(); //remove list's listCurrentItem, set listCurrentItem to next item
        refreshNextTracks();   //repaint from current
    }

    //keep recent buffer synchronised
    m_prevTracks.remove( item ); //removes all items

    emit itemCountChanged( childCount() );
}


void Playlist::refreshNextTracks( int from )
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


void Playlist::activate( QListViewItem *lvi, bool rememberTrack ) //SLOT
{
    //ATTENTION!
    //_All_ requests for playing items should come through here, thanks!

    PlaylistItem* const item = (PlaylistItem*)lvi;

    if( rememberTrack && item )
    {
        m_prevTracks.insert( 0, item ); //is push_back, see QPtrStack docs

        //keep prevList within reasonable limits, remove most distant members
        while( m_prevTracks.count() > 40 )
            m_prevTracks.remove( m_prevTracks.at( m_prevTracks.count() - 1 ) );
    }


    if( item )
    {
        //if we are playing something from the next tracks list, remove it from the list
        //do it here rather than in setCurrentTrack(), because if playback fails we don't
        //want to try playing it repeatedly!

        int index = m_nextTracks.findRef( item ); //set's m_nextTracks.current()
        if( index != -1 )
        {
            m_nextTracks.remove(); //will remove m_nextTracks.current()
            refreshNextTracks( index );
        }

        //when the engine calls newMetaDataNotify we are expecting it
        m_cachedTrack = item;

        //tell the engine to play the new track
        EngineController::instance()->play( item->metaBundle() );

    } else {

        //FIXME this may cause premature stopping with crossfading..
        EngineController::instance()->stop();
        setCurrentTrack( 0 );
    }
}


void Playlist::engineNewMetaData( const MetaBundle &bundle, bool trackChanged )
{
    if( m_currentTrack && !trackChanged )
    {
        kdDebug() << "newMetaData, track is same: " << !trackChanged << endl;

        //if the track hasn't changed then we should update the meta data for the item
        m_currentTrack->setText( bundle );

    }
    if( !m_cachedTrack || m_cachedTrack->url() != bundle.url() )
    {
        //FIXME most likely best to start at currentTrack() and be clever
        for( m_cachedTrack = firstChild();
             m_cachedTrack && m_cachedTrack->url() != bundle.url();
             m_cachedTrack = m_cachedTrack->nextSibling() );
    }
    if ( trackChanged )
        setCurrentTrack( m_cachedTrack );
}


void Playlist::engineStateChanged( EngineBase::EngineState state )
{
    //TODO define states in the ui.rc file and override setEnabled() for prev and next so they auto check
    //     isTrackBefore/After (you could make them engineObservers but that's more overhead)

    switch( state )
    {
    case EngineBase::Playing:
        //TODO make it fade in and out of the backgroundColor()
        m_glowTimer->start( 40 );
        m_ac->action( "pause" )->setEnabled( true );
        m_ac->action( "stop" )->setEnabled( true );
        m_ac->action( "prev" )->setEnabled( isTrackBefore() ); //FIXME you also do this in setCurrenTrack
        m_ac->action( "next" )->setEnabled( isTrackAfter() );  //FIXME you also do this in setCurrenTrack
        m_ac->action( "playlist_show" )->setEnabled( true );
        break;

    case EngineBase::Empty:
        //TODO do this with setState() in PlaylistWindow?
        m_ac->action( "pause" )->setEnabled( false );
        m_ac->action( "stop" )->setEnabled( false );
        m_ac->action( "prev" )->setEnabled( false );
        m_ac->action( "next" )->setEnabled( false );
        m_ac->action( "playlist_show" )->setEnabled( false );

        //don't leave currentTrack in undefined glow state
        Glow::counter = 63;
        if ( currentTrack() )
        {
            currentTrack()->m_playing = false;
            currentTrack()->invalidateHeight();
        }
        slotGlowTimer();

        //FALL THROUGH

    case EngineBase::Paused:
        m_glowTimer->stop();
        repaintItem( currentTrack() );
        break;

    default:
        break;
    }
}


void Playlist::setCurrentTrack( PlaylistItem *item )
{
    //item has been verified to be the currently playing track, let's paint it red!

    PlaylistItem *prev = currentTrack();
    const bool canScroll = !renameLineEdit()->isVisible() && selectedItems().count() < 2; //FIXME O(n)

    //if nothing is current and then playback starts, we must show the currentTrack
    if( !m_currentTrack && canScroll ) ensureItemVisible( item ); //handles 0 gracefully
    if( item ) item->setSelected( false ); //looks bad painting selected and glowing


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

    m_currentTrack = item;
    m_cachedTrack  = 0; //invalidate cached pointer
    
    if( item ) {
        item->m_playing = true;
        item->setHeight( fontMetrics().height() * 2 );
    }    
    if( prev && item != prev ) {
        prev->m_playing = false;
        prev->invalidateHeight();
    }

    repaintItem( prev );
    repaintItem( item );

    m_ac->action( "prev" )->setEnabled( isTrackBefore() );
    m_ac->action( "next" )->setEnabled( isTrackAfter() );
}


PlaylistItem *Playlist::restoreCurrentTrack()
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
    }

    return m_currentTrack;
}


void Playlist::setSorting( int col, bool b )
{
    saveUndoState();

    KListView::setSorting( col, b );
}


void Playlist::setColumnWidth( int col, int width )
{
    KListView::setColumnWidth( col, width );

    //FIXME this is because Qt doesn't by default disable resizing width 0 columns. GRRR!
    //NOTE  default column sizes are stored in default amarokrc so that restoreLayout() in ctor will
    //      call this function. This is necessary because addColumn() doesn't call setColumnWidth() GRRR!
    header()->setResizeEnabled( width != 0, col );
}


void Playlist::saveUndoState() //SLOT
{
   if( saveState( m_undoList ) )
   {
      m_redoList.clear();

      m_undoButton->setEnabled( true );
      m_redoButton->setEnabled( false );
   }
}


bool Playlist::saveState( QStringList &list )
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

      //kdDebug() << "Saved state: " << fileName << endl;

      return true;
   }

   return false;
}


void Playlist::undo() { switchState( m_undoList, m_redoList ); } //SLOT
void Playlist::redo() { switchState( m_redoList, m_undoList ); } //SLOT

void Playlist::switchState( QStringList &loadFromMe, QStringList &saveToMe )
{
    //switch to a previously saved state, remember current state
    KURL url; url.setPath( loadFromMe.last() );
    KURL::List playlist( url );
    loadFromMe.pop_back();

    //save current state
    saveState( saveToMe );

    //blockSignals so that we don't cause a saveUndoState()
    //FIXME, but this will stop the search lineEdit from being cleared..
    blockSignals( true );
        clear();
    blockSignals( false );

    insertMedia( playlist ); //because the listview is empty, undoState won't be forced

    m_undoButton->setEnabled( !m_undoList.isEmpty() );
    m_redoButton->setEnabled( !m_redoList.isEmpty() );
}


void Playlist::copyToClipboard( const QListViewItem *item ) const //SLOT
{
    if( !item ) item = currentTrack();

    if( item )
    {
        #define item static_cast<const PlaylistItem*>(item)
        QApplication::clipboard()->setText( item->trackName(), QClipboard::Clipboard );
        QApplication::clipboard()->setText( item->trackName(), QClipboard::Selection );
        #undef item
    }
}


void Playlist::slotMouseButtonPressed( int button, QListViewItem *after, const QPoint &p, int col ) //SLOT
{
    switch( button )
    {
    case Qt::MidButton:
    {
        //FIXME shouldn't the X11 paste get to Qt via some kind of drop?
        //TODO handle multiple urls?
        QString path = QApplication::clipboard()->text( QClipboard::Selection );
        kdDebug() << "[playlist] X11 Paste: " << path << endl;

        insertMediaInternal( KURL::fromPathOrURL( path ), (PlaylistItem*)after );
        break;
    }

    case Qt::RightButton:
        showContextMenu( after, p, col );
        break;

    default:
        break;
    }
}


void Playlist::showContextMenu( QListViewItem *item, const QPoint &p, int col ) //SLOT
{
    #define item static_cast<PlaylistItem*>(item)

    enum Id { PLAY, PLAY_NEXT, VIEW, EDIT, FILL_DOWN, COPY, REMOVE };

    if( item == NULL ) return; //technically we should show "Remove" but this is far neater

    const bool canRename = isRenameable( col );
    const bool isCurrent = (item == m_currentTrack);
    const bool isPlaying = EngineController::engine()->state() == EngineBase::Playing;
    const int queueIndex = m_nextTracks.findRef( item );
    const bool isQueued  = queueIndex != -1;

    //Markey, sorry for the lengths of these lines! -mxcl

    QPopupMenu popup( this );
    popup.insertItem( SmallIcon( "player_play" ), isCurrent && isPlaying ? i18n( "&Play (Restart)" ) : i18n( "&Play" ), 0, 0, Key_Enter, PLAY );

    if( !isQueued ) //not in nextTracks queue
    {
        QString nextText = i18n( "Play as &Next" );

        uint nextIndex = m_nextTracks.count() + 1;
        if( nextIndex > 1 ) nextText += QString( " (%1)" ).arg( nextIndex );

        popup.insertItem( SmallIcon( "2downarrow" ), nextText, PLAY_NEXT );
    }
    else popup.insertItem( i18n( "&Dequeue (%1)" ).arg( queueIndex+1 ), PLAY_NEXT );

    popup.insertItem( SmallIcon( "info" ), i18n( "&View Meta Information..." ), VIEW ); //TODO rename properties
    popup.insertItem( SmallIcon( "edit" ), i18n( "&Edit Tag: '%1'" ).arg( columnText( col ) ), 0, 0, Key_F2, EDIT );
    if( canRename )
    {
        const QListViewItem *below = item->itemBelow();
        if( below && below->isSelected() && item->exactText( col ) != i18n( "Writing tag..." ) )
        {
            //we disable Fill-Down when the current cell is having it's tag written
            //but it's a hack that needs to be improved

            popup.insertItem( i18n( "Spreadsheet-style fill down", "&Fill-down" ), FILL_DOWN );
        }
    }
    popup.insertItem( SmallIcon( "editcopy" ), i18n( "&Copy Trackname" ), 0, 0, CTRL+Key_C, COPY ); //FIXME use KAction
    popup.insertSeparator();
    popup.insertItem( SmallIcon( "edittrash" ), i18n( "&Remove Selected" ), this, SLOT( removeSelectedItems() ), Key_Delete );

    //only enable for columns that have editable tags
    popup.setItemEnabled( EDIT, canRename );

    switch( popup.exec( p ) )
    {
    case PLAY:
        activate( item );
        break;

    case PLAY_NEXT:
        item->setSelected( false ); //for prettiness

        if( isQueued )
        {
            //if is Queued, remove the item
            m_nextTracks.at( queueIndex ); //set current item, at() is quickest way
            m_nextTracks.remove(); //NOTE!! gets repainted due to the deselect above
        }
        else m_nextTracks.append( item ); //else append it on the end of the list

        refreshNextTracks(); //will start at current item in QPtrList

        break;

    case VIEW:
        showTrackInfo( static_cast<PlaylistItem *>(item) );
        break;

    case EDIT:
        rename( item, col );
        break;

    case FILL_DOWN:
        //Spreadsheet like fill-down
        {
            QString newTag = item->exactText( col );
            QListViewItemIterator it( item, QListViewItemIterator::Visible | QListViewItemIterator::Selected );

            // special handling for track column
            bool isNumber;
            int trackNo = 0;
            if ( col == 7 )
            {
                trackNo = newTag.toInt( &isNumber );
                // first row has no valid number, let's start with 1
                if ( !isNumber )
                {
                    m_weaver->append( new TagWriter( this, item, "1", col ), true );
                    trackNo = 1;
                }
            }

            while( ++it, *it )
            {
                // special handling for track column
                if ( col == 7 )
                    newTag = QString::number( ++trackNo );
                m_weaver->append( new TagWriter( this, (PlaylistItem*)*it, newTag, col ), true );
            }
        }
        break;

    case COPY:
        copyToClipboard( item );
        break;
    }

    #undef item
}


void Playlist::showTrackInfo( PlaylistItem *pItem ) const //SLOT
{
    QString str  = "<html><body><table width=\"100%\" border=\"1\">";
    QString body = "<tr><td>%1</td><td>%2</td></tr>";

    if( AmarokConfig::showMetaInfo() )
    {
         MetaBundle mb = pItem->metaBundle();

         str += body.arg( i18n( "Title" ),  mb.title() );
         str += body.arg( i18n( "Artist" ), mb.artist() );
         str += body.arg( i18n( "Album" ),  mb.album() );
         str += body.arg( i18n( "Genre" ),  mb.genre() );
         str += body.arg( i18n( "Year" ),   mb.year() );
         str += body.arg( i18n( "Comment" ),mb.comment() );
         str += body.arg( i18n( "Length" ), mb.prettyLength() );
         str += body.arg( i18n( "Bitrate" ),mb.prettyBitrate() );
         str += body.arg( i18n( "Samplerate" ), mb.prettySampleRate() );
         str += body.arg( i18n( "Location" ), mb.url().path() );
    }
    else
    {
        //FIXME this is wrong, see above if statement
        str += body.arg( i18n( "Stream" ), pItem->url().prettyURL() );
        str += body.arg( i18n( "Title" ),  pItem->trackName() );
    }

    str.append( "</table></body></html>" );

    QMessageBox box( kapp->makeStdCaption( i18n("Meta Information") ), str, QMessageBox::Information,
                     QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton,
                     0, 0, true, Qt::WStyle_DialogBorder );
    box.setTextFormat( Qt::RichText );
    box.exec();
}


void Playlist::paletteChange( const QPalette &p )
{
    using namespace Glow;

    QColor fg;
    QColor bg;

    {
        using namespace Base;

        const uint steps = STEPS+7; //so we don't fade all the way to base

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

        const uint steps = STEPS+2; //so we don't fade all the way to base

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

void Playlist::slotGlowTimer() //SLOT
{
    if( !currentTrack() ) return;

    using namespace Glow;

    if( counter > 63-(STEPS*2) )
    {
        const double d = (counter > 63-STEPS) ? STEPS-(counter-(63-STEPS)) : counter-(63-STEPS*2);

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


void Playlist::slotTextChanged( const QString &query ) //SLOT
{
    //TODO allow a slight delay before searching
    //TODO if we provided the lineEdit m_lastSearch would be unecessary

    const QString loweredQuery = query.lower();
    const QStringList v = QStringList::split( ' ', loweredQuery );
    QListViewItem *item = 0;
    QListViewItemIterator it( this, loweredQuery.startsWith( m_lastSearch ) ? QListViewItemIterator::Visible : 0 );
    bool b;

    while( (item = it.current()) )
    {
        b = query.isEmpty(); //if query is empty skip the loops and show all items

        for( uint x = 0; !b && x < v.count(); ++x ) //v.count() is constant time
            for( uint y = 0; !b && y < 4; ++y ) // search in Trackname, Artist, Songtitle, Album
                b = static_cast<PlaylistItem*>(item)->exactText( y ).lower().contains( v[x] );

        item->setVisible( b );
        ++it;
    }

    m_lastSearch = loweredQuery;

    //to me it seems sensible to do this, BUT if it seems annoying to you, remove it
    showCurrentTrack();
    clearSelection(); //we do this because QListView selects inbetween visible items, this is a non ideal solution
    triggerUpdate();
}


void Playlist::slotEraseMarker() //SLOT
{
    if( m_marker )
    {
        QRect spot = drawDropVisualizer( 0, 0, m_marker );
        m_marker = 0;
        viewport()->repaint( spot, false );
    }
}


void Playlist::writeTag( QListViewItem *lvi, const QString &tag, int col ) //SLOT
{
    //TODO we need a way to tell if the text after the rename was the same
    //     at this point the text is already changed grrr..

    m_weaver->append( new TagWriter( this, (PlaylistItem *)lvi, tag, col ), true );

    QListViewItem *below = lvi->itemBelow();
    //FIXME will result in nesting of this function?
    if( below && below->isSelected() ) { rename( below, col ); }
}


void Playlist::readAudioProperties( PlaylistItem *pi )
{
    if( AmarokConfig::showMetaInfo() ) m_weaver->append( new AudioPropertiesReader( this, pi ) );
}



// PRIVATE EVENTS =======================================================

void Playlist::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() == viewport() || KURLDrag::canDecode( e ) );
}


void Playlist::contentsDragMoveEvent( QDragMoveEvent* e )
{
    slotEraseMarker();

    Window root;
    Window child;
    int root_x, root_y, win_x, win_y;
    uint keybstate;
    XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
                   &root_x, &root_y, &win_x, &win_y, &keybstate );
    bool shiftPressed = keybstate & ShiftMask;

    //NOTE this code straight from KListView::findDrop()
    //TODO use findDrop()!
    //Get the closest item before us ('atpos' or the one above, if any)
    QPoint p = contentsToViewport( e->pos() );
    m_marker = itemAt( p );
    if( NULL == m_marker || shiftPressed )
        m_marker = lastItem();
    else if( p.y() - itemRect( m_marker ).topLeft().y() < (m_marker->height()/2) )
        m_marker = m_marker->itemAbove();

    //TODO don't dupe code, see viewportPaintEvent()
    QPainter painter( viewport() );
    painter.fillRect( drawDropVisualizer( 0, 0, m_marker ), QBrush( Qt::red, QBrush::Dense4Pattern ) );
}


void Playlist::contentsDragLeaveEvent( QDragLeaveEvent* )
{
    slotEraseMarker();
}


void Playlist::contentsDropEvent( QDropEvent *e )
{
    //NOTE parent is always 0 currently, but we support it in case we start using trees
    QListViewItem *parent = 0;
    QListViewItem *after  = m_marker;
    if( after == 0 )
    {
        findDrop( e->pos(), parent, after );
    }
    slotEraseMarker();

    if( e->source() == viewport() )
    {
        setSorting( NO_SORT ); //disableSorting and saveState()
        movableDropEvent( parent, after );

        //FIXME if the current Item was moved we need to update whether next/prev are enabled/disabled
    }
    else
    {
        KURL::List list;
        if( KURLDrag::decode( e, list ) )
        {
            insertMediaInternal( list, (PlaylistItem*)after );
        }
        else e->ignore();
    }
}


QDragObject* Playlist::dragObject()
{
    KURL::List list;

    for( QListViewItemIterator it( this, QListViewItemIterator::Selected ); *it; ++it )
        list += static_cast<PlaylistItem*>(*it)->url();

    return new KURLDrag( list, viewport() );
}


void Playlist::viewportPaintEvent( QPaintEvent *e )
{
    KListView::viewportPaintEvent( e );

    if( m_marker )
    {
        QPainter painter( viewport() );
        painter.fillRect( drawDropVisualizer( 0, 0, m_marker ), QBrush( Qt::red, QBrush::Dense4Pattern ) );
    }
}


bool Playlist::eventFilter( QObject *o, QEvent *e )
{
    #define me static_cast<QMouseEvent*>(e)

    //we only filter the header currently, but the base class has eventFilters in place too

    if( o == header() && e->type() == QEvent::MouseButtonPress && me->button() == Qt::RightButton )
    {
        //currently the only use for this filter is to get mouse clicks on the header()
        KPopupMenu popup;
        popup.setCheckable( true );
        popup.insertTitle( i18n( "Available Columns" ) );

        for( int i = 0; i < columns(); ++i ) //columns() references a property
        {
            popup.insertItem( columnText( i ), i, i + 1 );
            popup.setItemChecked( i, columnWidth( i ) != 0 );
        }

        int col = popup.exec( static_cast<QMouseEvent *>(e)->globalPos() );

        if( col != -1 )
        {
            //TODO can result in massively wide column appearing!
            if( columnWidth( col ) == 0 ) adjustColumn( col );
            else hideColumn( col );
        }

        return TRUE; // eat event

    }
    // not in slotMouseButtonPressed because we need to disable normal usage.
    else if( o == viewport() && e->type() == QEvent::MouseButtonPress &&
             me->button() == RightButton && me->state() == Qt::ControlButton )
    {
        const QPoint p = me->pos();
        PlaylistItem *item = static_cast<PlaylistItem*>(itemAt(p) );
        if( item )
        {
            item->setSelected( false ); //for prettiness

            if( m_nextTracks.findRef( item ) != -1 ) //will set list's current item
            {
                //if is Queued, remove the item
                m_nextTracks.remove();
                item->repaint();
            }
            else m_nextTracks.append( item );

            refreshNextTracks();
        }

        return TRUE; //yum!
    }

    //allow the header to process this
    return KListView::eventFilter( o, e );

    #undef me
}


void Playlist::customEvent( QCustomEvent *e )
{
    //the threads send their results here for completion that is GUI-safe

    switch( e->type() )
    {
    case PlaylistLoader::Started:

        //FIXME This is done here rather than startLoader()
        //because Qt DnD sets the overrideCursor and then when it
        //restores the cursor it removes the waitCursor we set!
        //Qt4 may fix this (?) (if we're lucky)
        //FIXME report to Trolltech?

        //FIXME this doesn't work 100% yet as you can spawn multiple loaders..
        m_clearButton->setEnabled( false );
        m_undoButton->setEnabled( false );
        m_redoButton->setEnabled( false );
        QApplication::setOverrideCursor( KCursor::workingCursor() );
        break;

    case PlaylistLoader::MakeItem:

        #define e static_cast<PlaylistLoader::MakeItemEvent*>(e)
        if( PlaylistItem *item = e->makePlaylistItem( this ) )
        {
            if( e->playMe() ) activate( item );

            if( AmarokConfig::showMetaInfo() ) m_weaver->append( new TagReader( this, item ) );

            emit itemCountChanged( childCount() );
        }
        #undef e
        break;

    case PlaylistLoader::PlaylistFound:

        #define e static_cast<PlaylistLoader::PlaylistFoundEvent*>(e)
        //m_weaver->append( new PLStats( m_browser, e->url(), e->contents() ) );
        #undef e
        break;

    case PlaylistLoader::Done:

        //FIXME this doesn't work 100% yet as you can spawn multiple loaders..
        m_clearButton->setEnabled( true );
        m_undoButton->setEnabled( !m_undoList.isEmpty() );
        m_redoButton->setEnabled( !m_redoList.isEmpty() );

        QApplication::restoreOverrideCursor();
        restoreCurrentTrack(); //just in case the track that is playing is not set current
        emit itemCountChanged( childCount() ); // final touch (also helps with default pls)
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

    case ThreadWeaver::Job::TagReader:

        #define e static_cast<TagReader*>(e)
        e->bindTags();
        #undef e
        break;

    case 4000: //dustbinEvent

        //this is a list of all the listItems from a clear operation
        #define list static_cast<QPtrList<QListViewItem>*>(e->data())
        kdDebug() << "Deleting " << list->count() << " PlaylistItems\n";
        delete list;
        #undef list
        break;

    default:
        break;
    }
}

#include "playlist.moc"
