/***************************************************************************
                       playlistwidget.cpp  -  description
                          -------------------
 begin                : Don Dez 5 2002
 copyright            : (C) 2002 by Mark Kretschmann
 email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include "playerapp.h" //restoreCurrentTrack(), removeSelectedItems(), restoreCurrentTrack() //FIXME remove!
#include "playlistitem.h"
#include "playlistloader.h"
#include "playlistwidget.h"
#include "metabundle.h"

#include <qclipboard.h> //copyToClipboard()
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


#include <kdebug.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <krandomsequence.h>
#include <kurldrag.h>
#include <kcursor.h>



PlaylistWidget::PlaylistWidget( QWidget *parent, /*KActionCollection *ac,*/ const char *name )
    : KListView( parent, name )
    , m_GlowTimer( new QTimer( this ) )
    , m_GlowCount( 100 )
    , m_GlowAdd( 5 )
    , m_currentTrack( 0 )
    , m_cachedTrack( 0 )
    , m_nextTrack( 0 )
    , m_marker( 0 )
    , m_tagReader( new TagReader( this ) )
    , m_undoButton( new QPushButton( i18n( "&Undo" ), 0 ) )
    , m_redoButton( new QPushButton( i18n( "&Redo" ), 0 ) )
    , m_undoDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) )
    , m_undoCounter( 0 )
{
    kdDebug() << "PlaylistWidget::PlaylistWidget()\n";

    setShowSortIndicator( true );
    setDropVisualizer( false );   //we handle the drawing for ourselves
    setDropVisualizerWidth( 3 );
    setItemsRenameable( true );
    KListView::setSorting( 200 ); //use base so we don't saveUndoState() too
    setAcceptDrops( true );
    setSelectionMode( QListView::Extended );
    setAllColumnsShowFocus( true );
    //    setStaticBackground( true );
    //    m_rootPixmap.setFadeEffect( 0.5, Qt::black );
    //    m_rootPixmap.start();

    //NOTE order is critical because we can't set indexes or ids
    addColumn( i18n( "Trackname" ), 280 );
    addColumn( i18n( "Title"     ), 200 );
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
    connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( showContextMenu( QListViewItem*, const QPoint&, int ) ) );
    connect( this, SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
             this,   SLOT( writeTag( QListViewItem*, const QString&, int ) ) );
    connect( this, SIGNAL( aboutToClear() ), SLOT( saveUndoState() ) );
    connect( m_undoButton, SIGNAL( clicked() ), SLOT( undo() ) );
    connect( m_redoButton, SIGNAL( clicked() ), SLOT( redo() ) );


    //IMPORTANT CONNECTS!
    connect( this, SIGNAL( playRequest( const KURL&, const MetaBundle& ) ),
             pApp,   SLOT( play( const KURL&, const MetaBundle& ) ) );

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

    m_GlowColor.setRgb( 0xff, 0x40, 0x40 );

    connect( m_GlowTimer, SIGNAL( timeout() ), this, SLOT( slotGlowTimer() ) );
    m_GlowTimer->start( 70 );

    // Read playlist columns layout
    restoreLayout( KGlobal::config(), "PlaylistColumnsLayout" );
}


PlaylistWidget::~PlaylistWidget()
{
   saveLayout( KGlobal::config(), "PlaylistColumnsLayout" );

   if( m_tagReader->running() )
   {
       kdDebug() << "[TagReader] Halting jobs..\n";
       m_tagReader->halt();
       m_tagReader->wait();
   }
}



//PUBLIC INTERFACE ===================================================

void PlaylistWidget::insertMedia( const KURL::List &list )
{
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
      if( AmarokConfig::randomMode() && recentPtrs.count() > 1 )
      {
          item = (PlaylistItem*)recentPtrs.at( recentPtrs.count() - 2 );
          recentPtrs.remove( recentPtrs.at( recentPtrs.count() - 1 ) );
      }
      else
      {
          item = (PlaylistItem*)item->itemAbove();

          if( item == NULL && AmarokConfig::repeatPlaylist() )
             item = (PlaylistItem*)lastItem();
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
              while ( ( recentPtrs.count() > ( childCount() / 2 ) ) || ( recentPtrs.count() > 50 ) )
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
                stream << url.path();
            else
            {
                stream << "#EXTINF:-1," + item->text( 0 ) + "\n";
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

    saveUndoState();

    // not evil, but corrrrect :)
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
    emit aboutToClear(); //will cause an saveUndoState()

    setCurrentTrack( NULL );
    m_tagReader->cancel(); //stop tag reading (very important!)
    searchTokens.clear();
    searchPtrs.clear();
    KListView::clear();
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

        //keep search system synchronised
        int x = searchPtrs.find( item );
        if ( x >= 0 )
        {
            searchTokens.remove( searchTokens.at( x ) );
            searchPtrs.remove( searchPtrs.at( x ) );
        }

        //if tagreader is running don't let tags be read for this item and delete later
        if ( m_tagReader->running() )
        {
            //FIXME make a customEvent to deleteLater(), can't use QObject::deleteLater() as we don't inherit QObject!
            item->setVisible( false ); //will be removed next time playlist is cleared
            m_tagReader->remove( item );
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
        setSorting( 200 ); //disable sorting and saveState()

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

    if( item )
    {
        #define item static_cast<PlaylistItem *>(item)
        m_cachedTrack = item;
        emit playRequest( item->url(), item->metaBundle() );
        #undef item
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

    m_currentTrack = item;

    //repaint items
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


void PlaylistWidget::setSorting( int i, bool b )
{
    saveUndoState();

    KListView::setSorting( i, b );
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

   //don't store blank intermediates in the undo/redo sets, perhaps a little inconsistent, but
   //probably desired by the user, if you disagree comment this out as it's debatable <mxcl>
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


void PlaylistWidget::switchState( QStringList &loadFromMe, QStringList &saveToMe )
{
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

    insertMedia( playlist );

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
        showTrackInfo( static_cast<const PlaylistItem *>(item) );
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
                    static_cast<PlaylistItem *>(*it)->writeTag( newTag, col );
                    it.current()->setText( col, newTag );
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


void PlaylistWidget::showTrackInfo( const PlaylistItem *pItem ) const //SLOT
{
    QString str( "<html><body><table border=\"1\">" );

    if( AmarokConfig::showMetaInfo() )
    {
         MetaBundle mb = pItem->metaBundle();

         str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Title" ),  mb.m_title );
         str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Artist" ), mb.m_artist );
         str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Album" ), mb.m_album );
         str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Genre" ), mb.m_genre );
         str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Year" ), mb.m_year );
         str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Comment" ), mb.m_comment );
         str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Length" ), QString::number( mb.m_length ) );
         str += QString( "<tr><td>%1</td><td>%2kbps</td></tr>" ).arg( i18n( "Bitrate" ), QString::number( mb.m_bitrate ) );
         str += QString( "<tr><td>%1</td><td>%2Hz</td></tr>" ).arg( i18n( "Samplerate" ), QString::number( mb.m_sampleRate ) );
    }
    else
    {
        //FIXME this is wrong, see above if statement
        str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Stream" ), pItem->url().prettyURL() );
        str += QString( "<tr><td>%1</td><td>%2</td></tr>" ).arg( i18n( "Title" ),  pItem->text( 0 ) );
    }

    str.append( "</table></body></html>" );

    QMessageBox box( i18n( "Track Information" ), str, QMessageBox::Information,
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
    //Surely we don't need to test for NULL here?
    static_cast<PlaylistItem*>(lvi)->writeTag( tag, col );

    QListViewItem *below = lvi->itemBelow();
    //FIXME will result in nesting of this function?
    if( below && below->isSelected() ) { rename( below, col ); }
}

void PlaylistWidget::undo() { switchState( m_undoList, m_redoList ); } //SLOT
void PlaylistWidget::redo() { switchState( m_redoList, m_undoList ); } //SLOT



// PRIVATE EVENTS =======================================================

void PlaylistWidget::contentsDragEnterEvent( QDragEnterEvent* e )
{
    e->accept();
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
        setSorting( 200 ); //disableSorting and saveState()
        movableDropEvent( parent, after );
    }
    else
    {
        KURL::List urlList;
        if( KURLDrag::decode( e, urlList ) )
        {
            insertMediaInternal( urlList, after );
        }
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
    if( o == header() && e->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent *>(e)->button() == Qt::RightButton )
    {
        //currently the only use for this filter is to get mouse clicks on the header()
        KPopupMenu popup;
        popup.setCheckable( true );
        popup.insertTitle( i18n( "Available Columns" ) );

        for( int i = 0; i < columns(); ++i ) //columns() references a property
        {
            popup.insertItem( columnText( i ), i, i );
            popup.setItemChecked( i, columnWidth( i ) != 0 );
        }

        int col = popup.exec( static_cast<QMouseEvent *>(e)->globalPos() );

        if( col != -1 )
        {
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
    switch( e->type() )
    {
    case PlaylistLoader::Started: //LoaderStartedEvent

        //FIXME This is done here rather than startLoader()
        //because Qt DnD sets the overrideCursor and then when it
        //restores the cursor it removes the waitCursor we set!
        //Qt4 may fix this (?) (if we're lucky)
        //FIXME report to Trolltech?

        QApplication::setOverrideCursor( KCursor::workingCursor() );
        break;

    case PlaylistLoader::SomeURL: //LoaderEvent

        if( PlaylistItem *item = static_cast<PlaylistLoader::LoaderEvent*>(e)->makePlaylistItem( this ) )
        {
            if( AmarokConfig::showMetaInfo() )
                m_tagReader->append( item );
            else
            {
                searchTokens.append( item->text( 0 ) );
                searchPtrs.append( item );
            }
        }
        break;

    case PlaylistLoader::Done: //LoaderDoneEvent

        QApplication::restoreOverrideCursor();
        restoreCurrentTrack();
        break;


    case TagReader::SomeTags:

        #define e static_cast<TagReader::TagReaderEvent*>(e)
        e->bindTags();
        e->addSearchTokens( searchTokens, searchPtrs );
        #undef e
        break;

    case TagReader::Done:

        QApplication::restoreOverrideCursor();
        break;


    default: ;
    }
}


#include "playlistwidget.moc"
