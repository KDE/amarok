/* Copyright 2002-2004 Mark Kretschmann, Max Howell
 * Licensed as described in the COPYING file found in the root of this distribution

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

#include <qclipboard.h> //copyToClipboard(), slotMouseButtonPressed()
#include <qcolor.h>
#include <qevent.h>
#include <qfile.h>
#include <qheader.h> //installEventFilter()
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpen.h>    //glowTimer()
#include <qpoint.h>
#include <qrect.h>
#include <qstringlist.h>
#include <qtimer.h>

#include <kcursor.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klineedit.h>  //setCurrentTrack()
#include <klistview.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <krandomsequence.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kurldrag.h>



PlaylistWidget::PlaylistWidget( QWidget *parent, /*KActionCollection *ac,*/ const char *name )
    : KListView( parent, name )
    , m_browser( /*new PlaylistBrowser( "PlaylistBrowser" )*/ 0 )
    , m_GlowTimer( new QTimer( this ) )
    , m_GlowCount( 100 )
    , m_GlowAdd( 5 )
    , m_currentTrack( 0 )
    , m_cachedTrack( 0 )
    , m_nextTrack( 0 )
    , m_marker( 0 )
    , m_weaver( new ThreadWeaver( this ) )
    , m_undoButton( new QPushButton( i18n( "&Undo" ), 0 ) )
    , m_redoButton( new QPushButton( i18n( "&Redo" ), 0 ) )
    , m_undoDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) )
    , m_undoCounter( 0 )
    , m_directPlay( false )
{
    kdDebug() << "PlaylistWidget::PlaylistWidget()\n";

    setShowSortIndicator( true );
    setDropVisualizer( false );   //we handle the drawing for ourselves
    setDropVisualizerWidth( 3 );
    setItemsRenameable( true );
    KListView::setSorting( NO_SORT ); //use base so we don't saveUndoState() too
    setAcceptDrops( true );
    setSelectionMode( QListView::Extended );
    setAllColumnsShowFocus( true );
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
    setColumnAlignment( 7, Qt::AlignRight );
    setColumnAlignment( 9, Qt::AlignRight );

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
    connect( m_undoButton, SIGNAL( clicked() ), SLOT( undo() ) );
    connect( m_redoButton, SIGNAL( clicked() ), SLOT( redo() ) );


    //IMPORTANT CONNECTS!
    connect( this, SIGNAL( playRequest( const MetaBundle& ) ),
             pApp,   SLOT( play( const MetaBundle& ) ) );

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


    connect( pApp, SIGNAL( orderPreviousTrack() ),
             this,   SLOT( handleOrderPrev() ) );
    connect( pApp, SIGNAL( orderCurrentTrack() ),
             this,   SLOT( handleOrderCurrent() ) );
    connect( pApp, SIGNAL( orderNextTrack() ),
             this,   SLOT( handleOrder() ) );

    connect( pApp, SIGNAL( currentTrack( const KURL& ) ),
             this,   SLOT( setCurrentTrack( const KURL& ) ) );


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
        m_undoButton->setFlat( true );
        m_redoButton->setFlat( true );
        m_undoButton->setFocusPolicy( QWidget::NoFocus );
        m_redoButton->setFocusPolicy( QWidget::NoFocus );
    //</init undo/redo>


    //install header eventFilter
    header()->installEventFilter( this );

    m_GlowColor.setRgb( 0xff, 0x40, 0x40 ); //FIXME move into the planned derived QColorGroup

    //TODO use timerEvent as is neater
    connect( m_GlowTimer, SIGNAL( timeout() ), this, SLOT( slotGlowTimer() ) );
    m_GlowTimer->start( 70 );

    //read playlist columns layout
    restoreLayout( KGlobal::config(), "PlaylistColumnsLayout" );
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
}



//PUBLIC INTERFACE ===================================================

QWidget *PlaylistWidget::browser() const { return m_browser; }
void PlaylistWidget::showCurrentTrack() { ensureItemVisible( currentTrack() ); } //SLOT


void PlaylistWidget::insertMedia( const KURL::List &list, bool directPlay )
{
    m_directPlay = directPlay;

    if( !list.isEmpty() )
    {
        //FIXME lastItem() scales badly!
        insertMediaInternal( list, lastItem() );
    }
}


void PlaylistWidget::handleOrderPrev()    { handleOrder( Prev ); }    //SLOT
void PlaylistWidget::handleOrderCurrent() { handleOrder( Current ); } //SLOT


void PlaylistWidget::handleOrder( RequestType rt ) //SLOT
{
   PlaylistItem* item = m_nextTrack;

   if( item == NULL )
   {
      item = currentTrack();

      if( item == NULL )
      {
         //no point advancing/receding track since there was no currentTrack!
         rt = Current;

         PlaylistItem *firstItem = (PlaylistItem*)firstChild();

         //if still NULL, then play first selected track
         for( item = firstItem; item; item = (PlaylistItem*)item->nextSibling() )
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
      if( AmarokConfig::repeatTrack() )
          break;
      else
          if( AmarokConfig::randomMode() && childCount() > 3 ) //FIXME is childCount O(1)?
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
                  item = (PlaylistItem*)firstChild();
          }
      break;

   case Current:
      break;
   }

   activate( item );
   m_nextTrack = 0;
}


void PlaylistWidget::saveM3u( const QString &fileName ) const
{
    QFile file( fileName );

    if( file.open( IO_WriteOnly ) )
    {
        KURL url;
        QTextStream stream( &file );
        stream << "#EXTM3U\n";

        for( const QListViewItem *item = this->firstChild(); item; item = item->nextSibling() )
        {
            url = static_cast<const PlaylistItem *>(item)->url();

            if ( url.protocol() == "file" )
            {
                stream << "#EXTINF:";
                {
                    QString length = item->text( 9 );

                    if( length == "?" ) length = QString();
                    else if( length == "-" ) length += '1';
                    else if( !length.isEmpty() )
                    {
                        //TODO if you ever decide to store length as an int in the playlistItem, scrap this!
                        int m = length.section( ':', 0, 0 ).toInt();
                        int s = length.section( ':', 1, 1 ).toInt();

                        length.setNum( m * 60 + s );
                    }

                    stream << length;
                }
                stream << ',';
                stream << item->text( 1 );
                stream << '\n';
                stream << url.path();
            }
            else
            {
                stream << QString( "#EXTINF:-1,%1\n" ).arg( item->text( 1 ) );
                stream << url.url();
            }

            stream << "\n";
        }
        file.close();
    }
}


void PlaylistWidget::shuffle() //SLOT
{
    //TODO offer this out as an action in a custom kactioncollection?

    setSorting( NO_SORT );

    QPtrList<QListViewItem> list;

    while( QListViewItem *first = firstChild() )
    {
        list.append( first );
        takeItem( first );
    }

    // initalize with seed
    KRandomSequence seq( static_cast<long>( KApplication::random() ) );
    seq.randomize( &list );

    for( uint i = 0; i < list.count(); ++i )
    {
        insertItem( list.at( i ) );
    }
}


void PlaylistWidget::clear() //SLOT
{
    emit aboutToClear(); //will saveUndoState()

    setCurrentTrack( NULL );
    recentPtrs.clear();
    searchTokens.clear();
    searchPtrs.clear();

    //TODO make it possible to tell when it is safe to not delay deletion
    //TODO you'll have to do the same as below for removeSelected() too.
    m_weaver->cancel(); //cancel all jobs in this weaver, no new events will be sent

    //now we have to ensure we don't delete the items before any events the weaver sent
    //have been processed, so we stick them in a QPtrList and delete it later
    QPtrList<QListViewItem> *list = new QPtrList<QListViewItem>;
    while( QListViewItem *item = firstChild() ) //FIXME check firstChild() is an efficient function here
    {
        takeItem( item );
        list->append( item );
    }
    QApplication::postEvent( this, new QCustomEvent( QCustomEvent::Type(4000), list ) );
}


bool PlaylistWidget::isAnotherTrack() const
{
    if( m_nextTrack != NULL ) return TRUE;
    if( m_currentTrack && m_currentTrack->itemBelow() ) return TRUE;

    return FALSE;
}


void PlaylistWidget::removeSelectedItems() //SLOT
{
    //two loops because:
    //1)the code is neater
    //2)If we remove m_currentTrack we select the next track because when m_currentTrack == NULL
    //  we play the first selected item. In order to be sure what we select won't be removed by
    //  this function, we use two loops

    //FIXME set m_nextTrack instead of selection
    //FIXME if you delete the last track when set current the playlist repeats on track end

    QPtrList<PlaylistItem> list;

    setSelected( currentItem(), true );     //remove currentItem, no matter if selected or not

    for( QListViewItemIterator it( this, QListViewItemIterator::Selected ); it.current(); ++it )
        if( it.current() != m_currentTrack )
            list.append( (PlaylistItem *)it.current() );

    //currenTrack must be last to ensure the item after it won't be removed
    //we select the item after currentTrack so it's played when currentTrack finishes
    if ( m_currentTrack != NULL && m_currentTrack->isSelected() ) list.append( m_currentTrack );
    if ( !list.isEmpty() ) saveUndoState();

    for ( PlaylistItem *item = list.first(); item; item = list.next() )
    {
        if ( m_currentTrack == item )
        {
            m_currentTrack = NULL;
            //now we select the next item if available so playback will continue from there next iteration
            if( pApp->isPlaying() )
                if( QListViewItem *tmp = item->nextSibling() )
                    tmp->setSelected( true );
        }
        else if( m_nextTrack == item ) m_nextTrack = 0;
        else if( m_cachedTrack == item ) m_cachedTrack = 0;

        //keep search system and recent buffer synchronised
        int x = searchPtrs.find( item );
        if ( x >= 0 )
        {
            searchTokens.remove( searchTokens.at( x ) );
            searchPtrs.remove( searchPtrs.at( x ) );
        }

        do
        {
            x = recentPtrs.find( item );
            if ( x >= 0 )
                recentPtrs.remove( recentPtrs.at( x ) );
        } while ( x >= 0 );

        //if tagreader is running don't let tags be read for this item and delete later
        if ( m_weaver->running() )
        {
            //FIXME make a customEvent to deleteLater(), can't use QObject::deleteLater() as we don't inherit QObject!
            item->setVisible( false ); //will be removed next time playlist is cleared
            //FIXME m_weaver->remove( item );
        }
        else
            delete item;
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

void PlaylistWidget::insertMediaInternal( const KURL::List &list, QListViewItem *after )
{
   //we don't check list.isEmpty(), this is a private function so we shouldn't have to
    PlaylistLoader *loader = new PlaylistLoader( list, this, after );

    if( loader )
    {
        setSorting( NO_SORT ); //disable sorting and saveState()

        QApplication::postEvent( this, new QCustomEvent( PlaylistLoader::Started ) ); //see customEvent for explanation

        loader->setOptions( AmarokConfig::directoriesRecursively(),
                            AmarokConfig::followSymlinks(),
                            AmarokConfig::browserSortingSpec() );
        loader->start();
    }
    else kdDebug() << "[playlist] Unable to create loader-thread!\n";
}


void PlaylistWidget::activate( QListViewItem *item ) //SLOT
{
    //lets ask the engine to play something
    if( PlaylistItem* const playItem = static_cast<PlaylistItem*>( item ) )
    {
        kdDebug() << "[playlist] Requesting playback for: " << item->text( 0 ) << endl;

        m_cachedTrack = playItem;
        emit playRequest( playItem->metaBundle() );
    }
}


void PlaylistWidget::setCurrentTrack( const KURL &u ) //SLOT
{
    //the engine confirms a new track is playing, lets try and highlight it

    if( m_currentTrack && m_currentTrack->url() == u ) return;
    if( m_cachedTrack == NULL || (m_cachedTrack && m_cachedTrack->url() != u) )
    {
        //FIXME most likely best to start at currentTrack() and be clever
        for( m_cachedTrack = (PlaylistItem *)firstChild();
             m_cachedTrack && m_cachedTrack->url() != u;
             m_cachedTrack = (PlaylistItem *)m_cachedTrack->nextSibling() );
    }

    setCurrentTrack( m_cachedTrack );
    m_cachedTrack = 0;
}


void PlaylistWidget::setCurrentTrack( PlaylistItem *item )
{
    //item has been verified to be the currently playing track

    PlaylistItem *tmp = PlaylistItem::GlowItem;
    PlaylistItem::GlowItem = item;

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

    if( AmarokConfig::playlistFollowActive() && m_currentTrack && item &&
        selectedItems().count() < 2 &&  // do not scroll if more than one item is selected
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

    repaintItem( tmp );
    repaintItem( item );
}


PlaylistItem *PlaylistWidget::restoreCurrentTrack()
{
   if( !pApp->isPlaying() ) return 0;

   KURL url( pApp->m_playingURL );

   if( !(m_currentTrack && m_currentTrack->url() == url) )
   {
      PlaylistItem* item;

      for( item = static_cast<PlaylistItem*>( firstChild() );
           item && item->url() != url;
           item = static_cast<PlaylistItem*>( item->nextSibling() ) )
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
      fileName.append( ".m3u" );

      if ( list.count() >= (uint)AmarokConfig::undoLevels() )
      {
         m_undoDir.remove( list.first() );
         list.pop_front();
      }

      saveM3u( fileName );
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
    //FIXME this is because loaders can't be cancelled, so you disable clear to indicate
    //it's not safe to clear
    if( !m_clearButton->isEnabled() ) return;

    //switch to a previously saved state, remember current state
    KURL url; url.setPath( loadFromMe.last() );
    KURL::List playlist( url );
    loadFromMe.pop_back();

    //save current state to: to
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
        QApplication::clipboard()->setText( item->text( 0 ) );
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

    if( item == NULL ) return; //technically we should show "Remove" but this is far neater

    bool canRename = isRenameable( col );
    bool isCurrent = (item == m_currentTrack);
    bool isPlaying = pApp->isPlaying();

    QPopupMenu popup( this );
    //TODO if paused the play stuff won't do what the user expects
    popup.insertItem( SmallIcon( "player_play" ), isCurrent ? i18n( "&Play (Restart)" ) : i18n( "&Play" ), 0, 0, Key_Enter, PLAY );
    if( !isCurrent && isPlaying )
    {
        //FIXME consider allowing people to play the current track next also
        popup.insertItem( i18n( "&Play Next" ), PLAY_NEXT );
    }
    popup.insertItem( SmallIcon( "info" ), i18n( "&View Meta Information..." ), VIEW ); //TODO rename properties
    popup.insertItem( SmallIcon( "edit" ), i18n( "&Edit Tag: '%1'" ).arg( columnText( col ) ), EDIT );
    if( canRename )
    {
        QListViewItem *below = item->itemBelow();
        if( below && below->isSelected() )
        {
            popup.insertItem( i18n( "Spreadsheet fill down", "&Fill-down" ), FILL_DOWN );
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
        m_nextTrack = (PlaylistItem*)item;
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
            QString newTag = item->text( col );
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
    QString str  = "<html><body><table border=\"1\">";
    QString body = "<tr><td>%1</td><td>%2</td></tr>";

    if( AmarokConfig::showMetaInfo() )
    {
         MetaBundle mb = pItem->metaBundle();

         str += body.arg( i18n( "Title" ),  mb.m_title );
         str += body.arg( i18n( "Artist" ), mb.m_artist );
         str += body.arg( i18n( "Album" ),  mb.m_album );
         str += body.arg( i18n( "Genre" ),  mb.m_genre );
         str += body.arg( i18n( "Year" ),   mb.m_year );
         str += body.arg( i18n( "Comment" ),mb.m_comment );
         str += body.arg( i18n( "Length" ), mb.prettyLength() );
         str += body.arg( i18n( "Bitrate" ),mb.prettyBitrate() );
         str += body.arg( i18n( "Samplerate" ), mb.prettySampleRate() );
    }
    else
    {
        //FIXME this is wrong, see above if statement
        str += body.arg( i18n( "Stream" ), pItem->url().prettyURL() );
        str += body.arg( i18n( "Title" ),  pItem->text( 0 ) );
    }

    str.append( "</table></body></html>" );

    QMessageBox box( i18n( "Meta Information" ), str, QMessageBox::Information,
                     QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton,
                     0, 0, true, Qt::WStyle_DialogBorder );
    box.setTextFormat( Qt::RichText );
    box.exec();
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
            p.setPen( m_GlowColor.light( m_GlowCount ) );

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

    clearSelection();
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

    //NOTE this code straight from KListView::findDrop()
    //TODO use findDrop()!
    //Get the closest item before us ('atpos' or the one above, if any)
    QPoint p = contentsToViewport( e->pos() );
    m_marker = itemAt( p );
    if( NULL == m_marker )
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
    //we only filter the header currently, but the base class has eventFilters in place too

    if( o == header() && e->type() == QEvent::MouseButtonPress &&
        static_cast<QMouseEvent *>(e)->button() == Qt::RightButton )
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

    } else {
        //allow the header to process this
        return KListView::eventFilter( o, e );
    }
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
        QApplication::setOverrideCursor( KCursor::workingCursor() );
        break;

    case PlaylistLoader::MakeItem:

        #define e static_cast<PlaylistLoader::MakeItemEvent*>(e)
        if( PlaylistItem *item = e->makePlaylistItem( this ) )
        {
            if( m_directPlay ) { activate( item ); m_directPlay = false; }

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
        m_weaver->append( new PLStats( m_browser, e->url(), e->contents() ) );
        #undef e
        break;

    case PlaylistLoader::Done:

        //FIXME this doesn't work 100% yet as you can spawn multiple loaders..
        m_directPlay = false; //just in case
        m_clearButton->setEnabled( true );
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
        kdDebug() << "Deleting " << static_cast<QPtrList<QListViewItem>*>(e->data())->count() << " items\n";
        delete (QPtrList<QListViewItem>*)e->data();
        break;

    default: ;
    }
}


void PlaylistWidget::handleStreamMeta( const MetaBundle& bundle )
{
    if ( QListViewItem* pItem = m_currentTrack ) {
        pItem->setText(  0, bundle.prettyURL()     );
        pItem->setText(  1, bundle.prettyTitle()   );
        pItem->setText(  2, bundle.m_artist        );    //this should not get saved with the playlist
        pItem->setText(  6, bundle.m_genre         );
        pItem->setText( 10, bundle.prettyBitrate() );
    }
}


#include "playlistwidget.moc"
