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

#include "playlistwidget.h"
#include "playerapp.h" //FIXME remove the need for this please!
#include "playlistitem.h"
#include "playlistloader.h"

#include <qcolor.h>
#include <qevent.h>
#include <qfile.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstringlist.h>
#include <qtimer.h>

#include <kdebug.h>
#include <klistview.h>
#include <klocale.h>
#include <kpopupmenu.h>
//#include <krootpixmap.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <krandomsequence.h>
#include <kurldrag.h>
#include <kcursor.h>


//TODO give UNDO action standard icon too
//TODO need to add tooltip to undo/redo buttons
//TODO move undo system to browserWin eventually


PlaylistWidget::PlaylistWidget( QWidget *parent, const char *name )
    : KListView( parent, name )
//    , m_rootPixmap( viewport()
    , m_GlowTimer( new QTimer( this ) )
    , m_GlowCount( 100 )
    , m_GlowAdd( 5 )
    , m_undoCounter( 0 )
    , m_tagReader( new TagReader( this ) ) //QThreads crash when not heap allocated
{
    kdDebug() << "PlaylistWidget::PlaylistWidget()" << endl;

    setName( "PlaylistWidget" );
    setFocusPolicy( QWidget::ClickFocus );
    setShowSortIndicator( true );
    setDropVisualizer( false );      // we handle the drawing for ourselves
    setDropVisualizerWidth( 3 );
    setItemsRenameable( true );
    setSorting( 200 );
    //    setStaticBackground( true );
    //    m_rootPixmap.setFadeEffect( 0.5, Qt::black );
    //    m_rootPixmap.start();

    addColumn( i18n( "Trackname" ), 280 );
    addColumn( i18n( "Title"     ), 200 );
    addColumn( i18n( "Artist"    ), 100 );
    addColumn( i18n( "Album"     ), 100 );
    addColumn( i18n( "Year"      ),  40 );
    addColumn( i18n( "Comment"   ),  80 );
    addColumn( i18n( "Genre"     ),  80 );
    addColumn( i18n( "Directory" ),  80 );

    connect( this, SIGNAL( contentsMoving( int, int ) ), this, SLOT( slotEraseMarker() ) );

    setCurrentTrack( NULL );
    m_GlowColor.setRgb( 0xff, 0x40, 0x40 );

    connect( m_GlowTimer, SIGNAL( timeout() ), this, SLOT( slotGlowTimer() ) );
    m_GlowTimer->start( 70 );

    initUndo();
}


PlaylistWidget::~PlaylistWidget()
{
   delete m_tagReader;
}



//PUBLIC INTERFACE -----------------------------------------------------------------

void PlaylistWidget::insertMedia( const QString &path )
{
   insertMedia( KURL( path ) );
}

void PlaylistWidget::insertMedia( const KURL &url )
{
   if( !url.isEmpty() )
   {
      insertMedia( KURL::List( url ), (QListViewItem *)0 );
   }
}

void PlaylistWidget::insertMedia( const KURL::List &list, bool doclear )
{
  if( !list.isEmpty() )
  {
     if( doclear )
        clear( false );

     insertMedia( list, (QListViewItem *)0 );
  }
}

void PlaylistWidget::insertMedia( const KURL::List &list, QListViewItem *after )
{
   kdDebug() << "PlaylistWidget::insertMedia()\n";

   if( !list.isEmpty() )
   {
      writeUndo();       //remember current state
      setSorting( 200 ); //disable sorting or will not be inserted where we expect

      startLoader( list, after );
      QApplication::setOverrideCursor( KCursor::workingCursor() );
   }
}


bool PlaylistWidget::restoreCurrentTrack()
{
   if( m_pCurrentTrack && static_cast<PlaylistItem *>(m_pCurrentTrack)->url() == pApp->m_playingURL )
      return true;
   else
     return setCurrentTrack( pApp->m_playingURL );
}


bool PlaylistWidget::setCurrentTrack( const KURL &url )
{
   PlaylistItem* item;

   for( item = static_cast<PlaylistItem*>( firstChild() );
        item && item->url() != url;
        item = static_cast<PlaylistItem*>( item->nextSibling() ) )
   {}

   setCurrentTrack( item ); //set even if == NULL; we don't want to be pointing to something that isn't playing!

   return ( item != NULL );
}


void PlaylistWidget::saveM3u( QString fileName )
{
    QFile file( fileName );

    if ( !file.open( IO_WriteOnly ) )
        return ;

    PlaylistItem* item = static_cast<PlaylistItem*>( firstChild() );
    QTextStream stream( &file );
    stream << "#EXTM3U\n";

    while ( item != NULL )
    {
        if ( item->url().protocol() == "file" )
            stream << item->url().path();
        else
        {
            stream << "#EXTINF:-1," + item->text( 0 ) + "\n";
            stream << item->url().url();
        }

        stream << "\n";
        item = static_cast<PlaylistItem*>( item->nextSibling() );
    }

    file.close();
}




// EVENTS -----------------------------------------------------------------

void PlaylistWidget::contentsDragMoveEvent( QDragMoveEvent* e )
{
    e->acceptAction();

    QListViewItem *parent;
    QListViewItem *after;
    findDrop( e->pos(), parent, after );

    QRect tmpRect = drawDropVisualizer( 0, parent, after );

    if ( tmpRect != m_marker )
    {
        slotEraseMarker();
        m_marker = tmpRect;
        viewport() ->repaint( tmpRect );
    }
}


void PlaylistWidget::contentsDragLeaveEvent( QDragLeaveEvent* )
{
    slotEraseMarker();
}


void PlaylistWidget::contentsDropEvent( QDropEvent* e )
{
    slotEraseMarker();

    //FIXME perhaps we should drop where the marker was rather than drop point is as if there
    //is an inconsistency we should give the user at least visual coherency

    //FIXME do we need to accept this event?

    QListViewItem *parent, *after;
    findDrop( e->pos(), parent, after );

    if ( e->source() == viewport() )
    {
        setSorting( 200 );
        writeUndo();
        movableDropEvent( parent, after );
    }
    else
    {
       KURL::List urlList;

       if ( KURLDrag::decode( e, urlList ) || urlList.isEmpty() )
       {
/*
          if ( pApp->m_optDropMode == "Ask" )
          {
             for ( KURL::List::Iterator url = media.begin(); url != media.end(); ++url )
             {
                KFileItem file( KFileItem::Unknown, KFileItem::Unknown, *url, true );
                if( file->isDir() ) break;
             }

             if ( url != media.end() )
             {
                QPopupMenu popup( this );
                popup.insertItem( i18n( "Add Recursively" ), this, 0, 1 ) );
                if( popup.exec( mapToGlobal( QPoint( e->pos().x() - 120, e->pos().y() - 20 ) ) );
             }

             //FIXME inform thread of user-decision
          }
*/
          insertMedia( urlList, after );
       }
    }

    restoreCurrentTrack();
}


void PlaylistWidget::customEvent( QCustomEvent *e )
{
   switch( e->type() )
   {
   case 65432: //LoaderEvent

      if( PlaylistItem *item = static_cast<PlaylistLoader::LoaderEvent*>(e)->makePlaylistItem( this ) ) //this is thread-safe
      {
         m_tagReader->append( item );

         //nonlocal downloads can fail
         searchTokens.append( item->text( 0 ) );
         searchPtrs.append( item );
      }
      break;

   case 65433: //LoaderDoneEvent

      static_cast<PlaylistLoader::LoaderDoneEvent*>(e)->dispose();
      QApplication::restoreOverrideCursor();
      break;

   case 65434: //TagReaderEvent

      static_cast<TagReader::TagReaderEvent*>(e)->bindTags();
      break;

   default: ;

   }
}


void PlaylistWidget::viewportPaintEvent( QPaintEvent *e )
{
    QListView::viewportPaintEvent( e );

    if ( m_marker.isValid() && e->rect().intersects( m_marker ) )
    {
        QPainter painter( viewport() );
        QBrush brush( QBrush::Dense4Pattern );
        brush.setColor( Qt::red );

        // This is where we actually draw the drop-visualizer
        painter.fillRect( m_marker, brush );
    }
}


//FIXME we don't want to have to include these :)
#include "browserwin.h"
#include <klineedit.h>

void PlaylistWidget::keyPressEvent( QKeyEvent *e )
{
   kdDebug() << "PlaylistWidget::keyPressEvent()\n";

   switch ( e->key() )
   {
   case Qt::Key_Delete:
      removeSelectedItems();
      e->accept();
      break;

   //trust me, I wish there was a better way to do this!
   case Key_0: case Key_1: case Key_2: case Key_3: case Key_4: case Key_5: case Key_6: case Key_7: case Key_8: case Key_9:
   case Key_A: case Key_B: case Key_C: case Key_D: case Key_E: case Key_F: case Key_G: case Key_H: case Key_I: case Key_J: case Key_K: case Key_L: case Key_M: case Key_N: case Key_O: case Key_P: case Key_Q: case Key_R: case Key_S: case Key_T: case Key_U: case Key_V: case Key_W: case Key_X: case Key_Y: case Key_Z:
     {
      KLineEdit *le = pApp->m_pBrowserWin->m_pPlaylistLineEdit;
      le->setFocus();
      QApplication::sendEvent( le, e );
      break;
     }
   default:
      KListView::keyPressEvent( e );
      //the base handler will set accept() or ignore()
   }
}




// PRIVATE METHODS -----------------------------------------------------------------

void PlaylistWidget::startLoader( const KURL::List &list, QListViewItem *after )
{
   //FIXME lastItem() has to go through entire list to find lastItem! Not scalable!
   PlaylistLoader *loader = new PlaylistLoader( list, this, ( after == 0 ) ? lastItem() : after );

   //FIXME remove meta option if that is the way things go
   if( loader ) {
      loader->setOptions( ( pApp->m_optDropMode == "Recursively" ), pApp->m_optFollowSymlinks, pApp->m_optReadMetaInfo );
      loader->start();
   } else
      kdDebug() << "[loader] Unable to create loader-thread!\n";
}


//FIXME deprecate, or maybe not.. can't decide
QListViewItem* PlaylistWidget::currentTrack() const
{
    return m_pCurrentTrack;
}

//FIXME deprecate
void PlaylistWidget::setCurrentTrack( QListViewItem *item )
{
    unglowItems(); //FIXME this iterates over all playlist items! very bad.
    //ensureItemVisible( item ); //can't think of any instance where this is necessary!
    m_pCurrentTrack = item;
}


void PlaylistWidget::unglowItems()
{
    PlaylistItem * item = static_cast<PlaylistItem*>( firstChild() );

    while ( item != NULL )
    {
        if ( item->isGlowing() )
        {
            item->setGlowing( false );
            repaintItem( item );
        }

        item = static_cast<PlaylistItem*>( item->nextSibling() );
    }
}




// SLOTS ----------------------------------------------

void PlaylistWidget::clear( bool full )
{
    if( full )
    {
       //FIXME this is unecessary now we have undo functionality!
       if ( pApp->m_optConfirmClear && KMessageBox::questionYesNo( 0, i18n( "Really clear playlist?" ) ) == KMessageBox::No )
          return;

       writeUndo();
    }

    m_tagReader->cancel(); //stop tag reading (very important!)
    searchTokens.clear();
    searchPtrs.clear();
    KListView::clear();
    setCurrentTrack( NULL );

    emit cleared();
}


void PlaylistWidget::slotGlowTimer()
{
    if ( !isVisible() )
        return ;

    PlaylistItem *item = static_cast<PlaylistItem*>( currentTrack() );

    if ( item != NULL )
    {
        item->setGlowing( true );

        if ( m_GlowCount > 120 )
        {
            m_GlowAdd = -m_GlowAdd;
        }
        if ( m_GlowCount < 90 )
        {
            m_GlowAdd = -m_GlowAdd;
        }
        item->setGlowCol( m_GlowColor.light( m_GlowCount ) );
        repaintItem( item );
        m_GlowCount += m_GlowAdd;
    }
}


void PlaylistWidget::slotTextChanged( const QString &str )
{
    QListViewItem * pVisibleItem = NULL;
    unsigned int x = 0;
    bool b;

    if ( str.contains( lastSearch ) && !lastSearch.isEmpty() )
        b = true;
    else
        b = false;

    lastSearch = str;

    if (b)
    {
        pVisibleItem = firstChild();
        while ( pVisibleItem )
        {
            if ( !pVisibleItem -> text(0).lower().contains( str.lower() ) )
                pVisibleItem -> setVisible( false );

            // iterate
            pVisibleItem = pVisibleItem -> itemBelow();
        }
    }
    else
        for ( QStringList::Iterator it = searchTokens.begin(); it != searchTokens.end(); ++it )
        {
            pVisibleItem = searchPtrs.at( x );

            if ( !(*it).lower().contains( str.lower() ) )
                pVisibleItem -> setVisible( false );
            else
                pVisibleItem -> setVisible( true );

            x++;
        }

    clearSelection();
    triggerUpdate();

    /* if ( pVisibleItem )
    {
        setCurrentItem( pVisibleItem );
        setSelected( pVisibleItem, true );
    }*/
}


void PlaylistWidget::setSorting( int i, bool b )
{
  //TODO consider removing this if and relying on the fact you always call setSorting to write the undo (?)

  //we overide so we can always write an undo
  if( i < 200 ) //FIXME 200 is arbituray, use sensible number like sizeof(short)
  {
    writeUndo();
  }

  KListView::setSorting( i, b );
}


void PlaylistWidget::slotEraseMarker()
{
    if ( m_marker.isValid() )
    {
        QRect rect = m_marker;
        m_marker = QRect();
        viewport()->repaint( rect, true );
    }
}


void PlaylistWidget::shuffle()
{
    writeUndo();

    // not evil, but corrrrect :)
    QPtrList<QListViewItem> list;

    while ( childCount() )
    {
        list.append( firstChild() );
        takeItem( firstChild() );
    }

    // initalize with seed
    KRandomSequence seq( static_cast<long>( KApplication::random() ) );
    seq.randomize( &list );

    for ( unsigned int i = 0; i < list.count(); i++ )
    {
        insertItem( list.at( i ) );
    }
}


void PlaylistWidget::removeSelectedItems()
{
  //FIXME this is the only method with which the user can remove items, however to properly future proof
  //      you need to somehow make it so creation and deletion of playlistItems handle the search
  //      tokens and pointers (and removal from tagReader queue!)

    writeUndo();

    QListViewItem *item, *item1;
    item = firstChild();

    while ( item != NULL )
    {
        item1 = item;
        item = item->nextSibling();

        if ( item1->isSelected() )
        {
           int x = searchPtrs.find( item1 );

           if ( x >= 0 )
           {
              searchTokens.remove( searchTokens.at( x ) );
              searchPtrs.remove( searchPtrs.at( x ) );
           }

           m_tagReader->remove( static_cast<PlaylistItem *>(item) ); //FIXME slow, nasty, not scalable

           delete item1;
        }
    }
}



// UNDO SYSTEM ==========================================================

void PlaylistWidget::initUndo()
{
    // create undo buffer directory
    m_undoDir.setPath( kapp->dirs()->saveLocation( "data", kapp->instanceName() + "/" ) );

    if ( !m_undoDir.exists( "undo", false ) )
        m_undoDir.mkdir( "undo", false );

    m_undoDir.cd( "undo" );

    // clean directory
    QStringList dirList = m_undoDir.entryList();
    for ( QStringList::Iterator it = dirList.begin(); it != dirList.end(); ++it )
        m_undoDir.remove( *it );
}


void PlaylistWidget::writeUndo()
{
   if ( saveState( m_undoList ) )
   {
      m_redoList.clear();

      emit sigUndoState( true );
      emit sigRedoState( false );
   }
}


bool PlaylistWidget::saveState( QStringList &list )
{
   //don't store blank intermediates in the undo/redo sets, perhaps a little inconsistent, but
   //probably desired by the user, if you disagree comment this out as it's debatable <mxcl>
   if ( childCount() > 0 )
   {
      QString fileName;
      m_undoCounter %= pApp->m_optUndoLevels;
      fileName.setNum( m_undoCounter++ );
      fileName.prepend( m_undoDir.absPath() + "/" );
      fileName += ".m3u";

      if ( list.count() >= pApp->m_optUndoLevels )
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


//inline
bool PlaylistWidget::canUndo()
{
    return !m_undoList.isEmpty();
}


//inline
bool PlaylistWidget::canRedo()
{
    return !m_redoList.isEmpty();
}


//TODO replace these two functions with one undoRedo( int ) and use a QSignalMapper
void PlaylistWidget::doUndo()
{
    if ( canUndo() )
    {
        //restore previous playlist
        kdDebug() << "loading state: " << m_undoList.last() << endl;

        KURL::List playlist( KURL( m_undoList.last() ) );
        m_undoList.pop_back();   //pop one of undo stack
        saveState( m_redoList ); //save currentState to redo stack
        clear( false );
        setSorting( 200 );
        startLoader( playlist, 0 );

        restoreCurrentTrack();
        //triggerUpdate();
    }

    emit sigUndoState( canUndo() );
    emit sigRedoState( canRedo() );
}


void PlaylistWidget::doRedo()
{
    if ( canRedo() )
    {
        //restore previous playlist
        KURL::List playlist( KURL( m_redoList.last() ) );
        m_redoList.pop_back();
        saveState( m_undoList );
        clear( false );
        setSorting( 200 );
        startLoader( playlist, 0 );

        restoreCurrentTrack();
        //triggerUpdate();
    }

    emit sigUndoState( canUndo() );
    emit sigRedoState( canRedo() );
}


#include "playlistwidget.moc"
