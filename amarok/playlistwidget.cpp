/* Copyright 2002-2004 Mark Kretschmann, Max Howell
 * Licensed as described in the COPYING file found in the root of this distribution
 * Maintainer: Max Howell <max.howell@methylblue.com>


 * NOTES
 *
 * The BrowserWin handles some PlaylistWidget events. Thanks!
 * This class has a QOBJECT but it's private so you can only connect via BrowserWin::BrowserWin
 * Mostly it's sensible to implement playlist functionality in this class
 * TODO Obtaining information about the playlist is currently hard, we need the playlist to be globally
 *      available and have some more useful public functions
 */


#include "amarokconfig.h"
#include "metabundle.h"
#include "playerapp.h" //restoreCurrentTrack(), removeSelectedItems() //FIXME remove!
#include "playlistbrowser.h"
#include "playlistitem.h"
#include "playlistloader.h"
#include "playlistwidget.h"
#include "threadweaver.h"
#include "enginecontroller.h"

#include <qclipboard.h> //copyToClipboard(), slotMouseButtonPressed()
#include <qcolor.h>
#include <qevent.h>
#include <qfile.h>
#include <qheader.h> //installEventFilter()
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpen.h>    //glowTimer()
#include <qpoint.h>
#include <qpushbutton.h>
#include <qrect.h>
#include <qstringlist.h>
#include <qtimer.h>

#include <kaction.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klineedit.h>  //setCurrentTrack()
#include <klistview.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <krandomsequence.h>
#include <kstandarddirs.h> //KGlobal::dirs()
#include <kstdaction.h>
#include <kurl.h>
#include <kurldrag.h>

#include <X11/Xlib.h>  // for XQueryPointer



PlaylistWidget::PlaylistWidget( QWidget *parent, KActionCollection *ac, const char *name )
    : KListView( parent, name )
    , m_browser( /*new PlaylistBrowser( "PlaylistBrowser" )*/ 0 )
    , m_GlowCount( 100 )
    , m_GlowAdd( 5 )
    , m_currentTrack( 0 )
    , m_cachedTrack( 0 )
    , m_marker( 0 )
    , m_weaver( new ThreadWeaver( this ) )
    , m_undoButton( KStdAction::undo( this, SLOT( undo() ), ac, "playlist_undo" ) )
    , m_redoButton( KStdAction::redo( this, SLOT( redo() ), ac, "playlist_redo" ) )
    , m_clearButton( 0 )
    , m_undoDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) )
    , m_undoCounter( 0 )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    // we want to receive engine updates
    EngineController::instance()->attach( this );

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
    setColumnAlignment( 10, Qt::AlignRight ); //bitrate


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
    connect( this, SIGNAL( aboutToClear() ), SLOT( saveUndoState() ) );


    //IMPORTANT CONNECTS!
    connect( this, SIGNAL( playRequest( const MetaBundle& ) ),
             EngineController::instance(),   SLOT( play( const MetaBundle& ) ) );

    //TODO in order to allow streams to update playlistitems I plan to allow a connection between a
    //metaBundle and the playlistItem is represents. Thus restoring the ability to modify stuff in the
    //playlist but without having problems with threading or the potential that items have been deleted
    //since the item started playback

    //TODO eventually the playlist should be globally accessible and have a nice public interface
    //that allows you to request a metaBundle by KURL etc. I don't think there is need to allow iteration
    //over elements

    //FIXME restore this functionality
    //connect( pApp, SIGNAL( metaData( const MetaBundle& ) ),
    //         this,   SLOT( handleStreamMeta( const MetaBundle& ) ) );


    connect( EngineController::instance(), SIGNAL( orderPrevious() ),
             this,   SLOT( handleOrderPrev() ) );
    connect( EngineController::instance(), SIGNAL( orderCurrent() ),
             this,   SLOT( handleOrderCurrent() ) );
    connect( EngineController::instance(), SIGNAL( orderNext() ),
             this,   SLOT( handleOrder() ) );

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

    //TODO move declarations to BrowserWin ctor? perhaps easier

    KAction *action;
    KStdAction::copy( this, SLOT( copyToClipboard() ), ac, "playlist_copy" );
    new KAction( i18n( "Shu&ffle" ), "rebuild", CTRL+Key_H, this, SLOT( shuffle() ), ac, "playlist_shuffle" );
    m_clearButton = new KAction( i18n( "&Clear" ), "view_remove", 0, this, SLOT( clear() ), ac, "playlist_clear" );
    action = new KAction( i18n( "&Show Playing" ), "today", CTRL+Key_Enter, this, SLOT( showCurrentTrack() ), ac, "playlist_show" );
    action->setToolTip( i18n( "Ensure the currently playing track is visible in the playlist" ) ); //FIXME doesn't show in toolbar!

    header()->installEventFilter( this );

    QTimer *timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( slotGlowTimer() ) );
    timer->start( 150 );

    restoreLayout( KGlobal::config(), "PlaylistColumnsLayout" );

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


PlaylistWidget::~PlaylistWidget()
{
   saveLayout( KGlobal::config(), "PlaylistColumnsLayout" );

   if( m_weaver->running() )
   {
       kdDebug() << "[weaver] Halting jobs..\n";
       m_weaver->halt();
       m_weaver->wait();
   }

   if( AmarokConfig::savePlaylist() ) saveXML( defaultPlaylistPath() );

   //speed up quit a little
   KListView::clear();
   blockSignals( true );
}



//PUBLIC INTERFACE ===================================================

QWidget *PlaylistWidget::browser() const { return m_browser; }
void PlaylistWidget::showCurrentTrack() { ensureItemVisible( currentTrack() ); } //SLOT


QString PlaylistWidget::defaultPlaylistPath() //static
{
    return KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "current.xml";
}


void PlaylistWidget::insertMedia( const KURL::List &list, bool directPlay )
{
    if( !list.isEmpty() )
    {
        //FIXME lastItem() scales badly!
        insertMediaInternal( list, lastItem(), directPlay );
    }
}


void PlaylistWidget::handleOrderPrev()    { handleOrder( Prev ); }    //SLOT
void PlaylistWidget::handleOrderCurrent() { handleOrder( Current ); } //SLOT


void PlaylistWidget::handleOrder( RequestType rt ) //SLOT
{
   PlaylistItem* item = (rt == Next) ? m_nextTracks.getFirst() : NULL;

   if( item == NULL )
   {
      item = currentTrack();

      if( item == NULL )
      {
         //no point advancing/receding track since there was no currentTrack!
         rt = Current;

         PlaylistItem *firstItem = firstChild();

         //if still NULL, then play first selected track
         for( item = firstItem; item; item = item->nextSibling() )
            if( item->isSelected() ) break;

         //if still NULL, then play first track
         if( item == NULL )
             item = firstItem;

         //if still null then playlist is empty
         //NOTE an initial ( childCount == 0 ) is possible, but this is safer
      }
   }
   else rt = Current; //play m_nextTrack

   switch( rt )
   {
   case Prev:
      //I've talked on a few channels, people hate it when media players restart the current track
      //first before going to the previous one (most players do this), so let's not do it!

      // choose right order in random-mode
      if( recentPtrs.count() > 1 )
      {
          item = (PlaylistItem *)recentPtrs.at( recentPtrs.count() - 2 );
          recentPtrs.remove( recentPtrs.at( recentPtrs.count() - 1 ) );
      }
      else
      {
          item = (PlaylistItem *)item->itemAbove();

          if( item == NULL && AmarokConfig::repeatPlaylist() )
             item = (PlaylistItem *)lastItem();
      }

      break;

   case Next:
      if ( AmarokConfig::repeatTrack() )
          break;
      else
          if ( AmarokConfig::randomMode() && childCount() > 3 ) //FIXME is childCount O(1)?
          {
              int x;
              do
              {
                  item = (PlaylistItem *)itemAtIndex( KApplication::random() % childCount() );
                  x = recentPtrs.find( item );
              }
              while( x >= 0 ); // try not to play the same tracks two often

              // add current item to the recently played list, and make sure this list doesn't get too large
              //FIXME: max. size of recent-buffer is set "manually" to 50 in the next lines.
              //       should be configurable or at least #define'd...
              recentPtrs.append( item );
              while ( ( recentPtrs.count() > (uint)( childCount() / 2 ) ) || ( recentPtrs.count() > 50 ) )
                  recentPtrs.remove( recentPtrs.at( 0 ) );
          }
          else
          {
              item = (PlaylistItem*)item->itemBelow();

              if( item == NULL && AmarokConfig::repeatPlaylist() )
                  item = firstChild();
          }
      break;

   case Current:
      if ( AmarokConfig::randomMode() )
          item = (PlaylistItem *)itemAtIndex( KApplication::random() % childCount() );

      break;
   }

   activate( item );
}


void PlaylistWidget::saveM3U( const QString &path ) const
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


void PlaylistWidget::saveXML( const QString &path ) const
{
    //TODO save nextTrack queue

    QFile file( path );

    if( !file.open( IO_WriteOnly ) ) return;

    QTextStream stream( &file );
    QString body  = "<%1>%2</%1>\n";
    QString open1 = "<item url=\"", open2 = "\">\n";
    QString close = "</item>\n";

    stream << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
    stream << "<playlist product=\"amaroK\" version=\"1\">\n";

    for( const PlaylistItem *item = firstChild(); item; item = item->nextSibling() )
    {
        stream << open1 << item->url().url() << open2;

        //TODO speed up, cache columnNames (if would help), etc.
        //NOTE we don't bother storing TrackName as this is based on the URL anyway

        for( int x = 1; x < columns(); ++x )
        {
            if( !item->exactText(x).isEmpty() )
                stream << body.arg( columnText(x), item->exactText(x).replace( '&', "&quot;" ).replace( '<', "&lt;" ) );
        }
        stream << close;
    }

    stream << "</playlist>\n";
    file.close();
}


void PlaylistWidget::shuffle() //SLOT
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
    for( uint i = 0; i < list.count(); ++i )
    {
        insertItem( list.at( i ) );
    }

    //now put nextTracks into playlist so they are first and from first to last
    for( PlaylistItem *item = m_nextTracks.last(); item; item = m_nextTracks.prev() )
    {
        insertItem( item );
    }

    m_nextTracks.clear();

    pApp->actionCollection()->action( "prev" )->setEnabled( isTrackBefore() );
    pApp->actionCollection()->action( "next" )->setEnabled( isTrackAfter() );
}


void PlaylistWidget::clear() //SLOT
{
    emit aboutToClear(); //will saveUndoState()

    setCurrentTrack( NULL );
    recentPtrs.clear();
    searchTokens.clear();
    searchPtrs.clear();
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
}


bool PlaylistWidget::isTrackAfter() const
{
    return AmarokConfig::repeatPlaylist() ||
           !m_nextTracks.isEmpty() ||
           m_currentTrack && m_currentTrack->itemBelow();
}

bool PlaylistWidget::isTrackBefore() const
{
    return AmarokConfig::repeatPlaylist() ||
           currentTrack() && currentTrack()->itemAbove();
}



void PlaylistWidget::removeSelectedItems() //SLOT
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
}

/*
void PlaylistWidget::summary( QPopupMenu &popup ) const
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



// PRIVATE METHODS ===============================================

void PlaylistWidget::insertMediaInternal( const KURL::List &list, QListViewItem *after, bool directPlay )
{
   //we don't check list.isEmpty(), this is a private function so we shouldn't have to
    PlaylistLoader *loader = new PlaylistLoader( list, this, after );

    if( loader )
    {
        setSorting( NO_SORT ); //disable sorting and saveState()

        QApplication::postEvent( this, new QCustomEvent( PlaylistLoader::Started ) ); //see customEvent for explanation

        loader->setOptions( AmarokConfig::directoriesRecursively(),
                            directPlay,
                            AmarokConfig::browserSortingSpec() );
        loader->start();
    }
    else kdDebug() << "[playlist] Unable to create loader-thread!\n";
}


void PlaylistWidget::removeItem( PlaylistItem *item )
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
        if( m_nextTracks.isEmpty() )
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

    //keep search system synchronised
    int x = searchPtrs.findRef( item );
    if( x >= 0 )
    {
        searchTokens.remove( searchTokens.at( x ) );
        searchPtrs.remove(/* x */); //TODO check this is safe!
    }

    //keep recent buffer synchronised
    for( x = -2; x != -1; )
    {
        x = recentPtrs.findRef( item ); //returns -1 if not found
        recentPtrs.remove( /*x*/ ); //removes current item, findRef sets current item
    }
}


void PlaylistWidget::refreshNextTracks( int from )
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


void PlaylistWidget::activate( QListViewItem *item ) //SLOT
{
    //lets ask the engine to play something
    if( PlaylistItem* const playItem = (PlaylistItem*)item )
    {
        kdDebug() << "[playlist] Requesting playback for: " << item->text( 0 ) << endl;

        int index = m_nextTracks.findRef( playItem );
        if( index != -1 )
        {
            //if we're playing a queued item, take it off the queue and refresh
            m_nextTracks.remove(); //will remove list's current member, which is set by findRef()
            refreshNextTracks( index );
        }

        //cache the track so when the engine tells us a KURL is playing we don't have to search
        m_cachedTrack = playItem;
        EngineController::instance()->play( playItem->metaBundle() );
    }
    else // NULL, stop the player
        EngineController::instance()->stop(); //FIXME this may cause premature stopping with crossfading..
}


void PlaylistWidget::setCurrentTrack( const KURL &u ) //SLOT
{
    //the engine confirms a new track is playing, lets try and highlight it

    if( m_currentTrack && m_currentTrack->url() == u ) return;
    if( m_cachedTrack == NULL || (m_cachedTrack && m_cachedTrack->url() != u) )
    {
        //FIXME most likely best to start at currentTrack() and be clever
        for( m_cachedTrack = firstChild();
             m_cachedTrack && m_cachedTrack->url() != u;
             m_cachedTrack = m_cachedTrack->nextSibling() );
    }

    setCurrentTrack( m_cachedTrack );
}


void PlaylistWidget::setCurrentTrack( PlaylistItem *item )
{
    //item has been verified to be the currently playing track

    PlaylistItem *prev = currentTrack();

    //the following 2 statements may seem strange, they are important however:
    //1. if nothing is current and then playback starts, the user needs to be shown the currentTrack
    //2. if we are setting to NULL (eg reached end of playlist) we need to unselect the item as well as unglow it
    //   as otherwise we will play that track next time the user presses play (rather than say the first track)
    //   because that is a feature of amaroK //FIXME this is sillyness

    if( m_currentTrack == NULL ) ensureItemVisible( item ); //handles NULL gracefully
    else if( item == NULL ) m_currentTrack->setSelected( false );
    else item->setSelected( false ); //looks bad paint selected and paint red

    //FIXME this sucks
    if( m_currentTrack == NULL && item ) item->setSelected( false ); //looks bad paint selected and paint red

    if( item && AmarokConfig::playlistFollowActive() && m_currentTrack &&
        selectedItems().count() < 2 &&  // do not scroll if more than one item is selected //FIXME O(n)
        renameLineEdit()->isVisible() == false ) // do not scroll if user is doing tag editing
    {
        // if old item in view and the new one isn't do scrolling
        int currentY = itemPos( m_currentTrack );
        if( currentY + m_currentTrack->height() <= contentsY() + visibleHeight()
            && currentY >= contentsY() )
        {
            // Scroll towards the middle but no more than two lines extra
            int scrollAdd = viewport()->height() / 2 - item->height();
            if( scrollAdd > 2 * item->height() ) scrollAdd = 2 * item->height();

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
    m_cachedTrack = 0; //invalidate cached pointers

    repaintItem( prev );
    repaintItem( item );

    pApp->actionCollection()->action( "prev" )->setEnabled( isTrackBefore() );
    pApp->actionCollection()->action( "next" )->setEnabled( isTrackAfter() );
}


PlaylistItem *PlaylistWidget::restoreCurrentTrack()
{
    bool loaded = EngineController::instance()->engine() ? EngineController::instance()->engine()->loaded() : false;
   if( !loaded ) return 0;

   KURL url( EngineController::instance()->playingURL() );

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


void PlaylistWidget::setSorting( int col, bool b )
{
    saveUndoState();

    KListView::setSorting( col, b );
}


void PlaylistWidget::setColumnWidth( int col, int width )
{
    KListView::setColumnWidth( col, width );

    //FIXME this is because Qt doesn't by default disable resizing width 0 columns. GRRR!
    //NOTE  default column sizes are stored in default amarokrc so that restoreLayout() in ctor will
    //      call this function. This is necessary because addColumn() doesn't call setColumnWidth() GRRR!
    header()->setResizeEnabled( width != 0, col );
}


void PlaylistWidget::saveUndoState() //SLOT
{
   if( saveState( m_undoList ) )
   {
      m_redoList.clear();

      m_undoButton->setEnabled( true );
      m_redoButton->setEnabled( false );
   }
}


bool PlaylistWidget::saveState( QStringList &list )
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

      kdDebug() << "Saved state: " << fileName << endl;

      return true;
   }

   return false;
}


void PlaylistWidget::undo() { switchState( m_undoList, m_redoList ); } //SLOT
void PlaylistWidget::redo() { switchState( m_redoList, m_undoList ); } //SLOT


void PlaylistWidget::switchState( QStringList &loadFromMe, QStringList &saveToMe )
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


void PlaylistWidget::copyToClipboard( const QListViewItem *item ) const //SLOT
{
    if( item == NULL ) item = currentTrack();

    if( item != NULL )
    {
        #define item static_cast<const PlaylistItem*>(item)
        QApplication::clipboard()->setText( item->trackName(), QClipboard::Clipboard );
        QApplication::clipboard()->setText( item->trackName(), QClipboard::Selection );
        #undef item
    }
}


void PlaylistWidget::slotMouseButtonPressed( int button, QListViewItem *after, const QPoint &p, int col ) //SLOT
{
    switch( button )
    {
    case Qt::MidButton:
        {
            //FIXME shouldn't the X11 paste get to Qt via some kind of drop?
            //TODO handle multiple urls?
            QString path = QApplication::clipboard()->text( QClipboard::Selection );
            kdDebug() << "[playlist] X11 Paste: " << path << endl;

            insertMediaInternal( KURL::fromPathOrURL( path ), after );
        }
        break;

    case Qt::RightButton:
        showContextMenu( after, p, col );
        break;

    default:
        break;
    }
}


void PlaylistWidget::showContextMenu( QListViewItem *item, const QPoint &p, int col ) //SLOT
{
    #define PLAY       0
    #define PLAY_NEXT  1
    #define VIEW       2
    #define EDIT       3
    #define FILL_DOWN  4
    #define COPY       5
    #define REMOVE     6

    #define item static_cast<PlaylistItem*>(item)

    if( item == NULL ) return; //technically we should show "Remove" but this is far neater

    const bool canRename = isRenameable( col );
    const bool isCurrent = (item == m_currentTrack);
    const bool isPlaying = EngineController::instance()->engine() ? EngineController::instance()->engine()->loaded() : false;
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
    popup.insertItem( SmallIcon( "edit" ), i18n( "&Edit Tag: '%1'" ).arg( columnText( col ) ), EDIT );
    if( canRename )
    {
        QListViewItem *below = item->itemBelow();
        if( below && below->isSelected() )
        {
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
        //TODO for track, increment obviously
        {
            QString newTag = static_cast<PlaylistItem*>(item)->exactText( col );
            QListViewItemIterator it( item );

            for( ++it; it.current(); ++it )
            {
                if( it.current()->isSelected() )
                {
                    m_weaver->append( new TagWriter( this, (PlaylistItem*)*it, newTag, col ), true );
                }
                else break;
            }
        }
        break;

    case COPY:
        copyToClipboard( item );
        break;
    }

    #undef item

    #undef PLAY
    #undef PLAY_NEXT
    #undef PROPERTIES
    #undef VIEW
    #undef FILL_DOWN
    #undef COPY
    #undef REMOVE
}


void PlaylistWidget::showTrackInfo( PlaylistItem *pItem ) const //SLOT
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
    }
    else
    {
        //FIXME this is wrong, see above if statement
        str += body.arg( i18n( "Stream" ), pItem->url().prettyURL() );
        str += body.arg( i18n( "Title" ),  pItem->trackName() );
    }

    str.append( "</table></body></html>" );

    QMessageBox box( i18n( "Meta Information" ), str, QMessageBox::Information,
                     QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton,
                     0, 0, true, Qt::WStyle_DialogBorder );
    box.setTextFormat( Qt::RichText );
    box.exec();
}

void PlaylistWidget::engineNewMetaData( const MetaBundle &bundle, bool trackChanged )
{
    if( trackChanged )
    {
        KURL u = bundle.url();
        //the engine confirms a new track is playing, lets try and highlight it
        if( m_currentTrack && m_currentTrack->url() == u ) return;
        if( m_cachedTrack == NULL || (m_cachedTrack && m_cachedTrack->url() != u) )
        {
            //FIXME most likely best to start at currentTrack() and be clever
            for( m_cachedTrack = firstChild();
                 m_cachedTrack && m_cachedTrack->url() != u;
                 m_cachedTrack = m_cachedTrack->nextSibling() );
        }

        setCurrentTrack( m_cachedTrack );
    }
}

void PlaylistWidget::slotGlowTimer() //SLOT
{
    if ( PlaylistItem *item = currentTrack() )
    {
        if ( m_GlowCount > 120 )
            m_GlowAdd = -m_GlowAdd;

        if ( m_GlowCount < 90 )
            m_GlowAdd = -m_GlowAdd;

        m_GlowCount += m_GlowAdd;

        //draw glowing rectangle around current track, to indicate activity
        QRect rect = itemRect( item ); //FIXME slow function!

        if ( rect.isValid() ) {
            QPainter p( viewport() );
            p.setPen( colorGroup().brightText().light( m_GlowCount ) );

            rect.setTop   ( rect.top()      );
            rect.setBottom( rect.bottom()   );
            rect.setWidth ( contentsWidth() );    //neccessary to draw on the complete width

            p.drawRect( rect );
        }
    }
}


void PlaylistWidget::slotTextChanged( const QString &str ) //SLOT
{
    QListViewItem *pVisibleItem = NULL;
    unsigned int x = 0;

    QStringList tokens = QStringList::split( " ", str.lower() );

    for ( QStringList::Iterator it = searchTokens.begin(); it != searchTokens.end(); ++it )
    {
        pVisibleItem = searchPtrs.at( x );

        pVisibleItem->setVisible( true );
        for ( uint y = 0; y < tokens.count(); ++y )
        {
            if ( !(*it).lower().contains( tokens[y] ) )
                pVisibleItem->setVisible( false );
        }

        x++;
    }

    //to me it seems sensible to do this, BUT if it seems annoying to you, remove it
    showCurrentTrack();

    clearSelection(); //why do this?
    triggerUpdate();
}


void PlaylistWidget::slotEraseMarker() //SLOT
{
    if( m_marker )
    {
        QRect spot = drawDropVisualizer( 0, 0, m_marker );
        m_marker = 0;
        viewport()->repaint( spot, false );
    }
}


void PlaylistWidget::writeTag( QListViewItem *lvi, const QString &tag, int col ) //SLOT
{
    m_weaver->append( new TagWriter( this, (PlaylistItem *)lvi, tag, col ), true );

    QListViewItem *below = lvi->itemBelow();
    //FIXME will result in nesting of this function?
    if( below && below->isSelected() ) { rename( below, col ); }
}


void PlaylistWidget::readAudioProperties( PlaylistItem *pi )
{
    m_weaver->append( new AudioPropertiesReader( this, pi ) );
}



// PRIVATE EVENTS =======================================================

void PlaylistWidget::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() == viewport() || KURLDrag::canDecode( e ) );
}


void PlaylistWidget::contentsDragMoveEvent( QDragMoveEvent* e )
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


void PlaylistWidget::contentsDragLeaveEvent( QDragLeaveEvent* )
{
    slotEraseMarker();
}


void PlaylistWidget::contentsDropEvent( QDropEvent *e )
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
        KURL::List urlList;
        if( KURLDrag::decode( e, urlList ) )
        {
            insertMediaInternal( urlList, after );
        }
        else e->ignore();
    }
}


void PlaylistWidget::viewportPaintEvent( QPaintEvent *e )
{
    KListView::viewportPaintEvent( e );

    if( m_marker )
    {
        QPainter painter( viewport() );
        painter.fillRect( drawDropVisualizer( 0, 0, m_marker ), QBrush( Qt::red, QBrush::Dense4Pattern ) );
    }
}


bool PlaylistWidget::eventFilter( QObject *o, QEvent *e )
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


void PlaylistWidget::customEvent( QCustomEvent *e )
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

            if( AmarokConfig::showMetaInfo() )
                m_weaver->append( new TagReader( this, item ) );
            else
            {
                searchTokens.append( item->trackName() );
                searchPtrs.append( item );
            }
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
        m_undoButton->setEnabled( true );
        m_redoButton->setEnabled( true );
        QApplication::restoreOverrideCursor();
        restoreCurrentTrack(); //just in case the track that is playing is not set current
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
        e->addSearchTokens( searchTokens, searchPtrs );
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

#include "playlistwidget.moc"
