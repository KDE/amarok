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

#include "playerapp.h"
#include "browserwin.h"
#include "browserwidget.h"
#include "playlistitem.h"

#include <qcolor.h>
#include <qcursor.h>
#include <qevent.h>
#include <qheader.h>
#include <qmessagebox.h>
#include <qpoint.h>
#include <qpopupmenu.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qvaluelist.h>
#include <qwidget.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <klistview.h>
#include <klocale.h>
#include <kurl.h>
#include <kurldrag.h>
#include <klineedit.h>
#include <kaccel.h>

// CLASS PlaylistWidget --------------------------------------------------------

PlaylistWidget::PlaylistWidget( QWidget *parent, const char *name ) : KListView( parent, name )
{
    setName( "PlaylistWidget" );
    setFocusPolicy( QWidget::ClickFocus );
    setPaletteBackgroundColor( pApp->m_bgColor );
    setShowSortIndicator( true );

    addColumn( i18n("Trackname"), 280 );
    addColumn( i18n("Title"), 200 );
    addColumn( i18n("Album"), 100 );
    addColumn( i18n("Artist"), 100 );
    addColumn( i18n("Year"), 40 );
    addColumn( i18n("Comment"), 80 );
    addColumn( i18n("Genre"), 80 );
    addColumn( i18n("Directory"), 80 );

    setSorting( -1 );
    connect( header(), SIGNAL( clicked( int ) ), this, SLOT( slotHeaderClicked( int ) ) );

    setCurrentTrack( NULL );

    m_GlowCount = 100;
    m_GlowAdd = 5;
    m_GlowColor.setRgb( 0xff, 0x40, 0x40 );

    m_GlowTimer = new QTimer( this );
    connect( m_GlowTimer, SIGNAL( timeout() ), this, SLOT( slotGlowTimer() ) );
    m_GlowTimer->start( 50 );

    m_pDirLister = new KDirLister();
    m_pDirLister->setAutoUpdate( false );
}



PlaylistWidget::~PlaylistWidget()
{}


// METHODS -----------------------------------------------------------------

void PlaylistWidget::contentsDragMoveEvent( QDragMoveEvent* e )
{
    e->acceptAction();
}


void PlaylistWidget::contentsDropEvent( QDropEvent* e )
{
    m_dropRecursionCounter = 0;

    if ( pApp->m_optDropMode == "Recursively" )
        m_dropRecursively = true;
    else
        m_dropRecursively = false;

    KURL::List urlList;

    if ( e->source() == NULL )                     // dragging from inside amarok or outside?
    {
        if ( !KURLDrag::decode( e, urlList ) || urlList.isEmpty() )
            return ;

        setUpdatesEnabled( false );
        m_pDropCurrentItem = static_cast<PlaylistItem*>( itemAt( e->pos() ) );
        playlistDrop( urlList );
    }
    else
    {
        PlaylistItem *srcItem, *newItem;

        if ( e->source() ->parent() == this )
            srcItem = static_cast<PlaylistItem*>( firstChild() );
        else
            srcItem = static_cast<PlaylistItem*>( pApp->m_pBrowserWin->m_pBrowserWidget->firstChild() );

        bool containsDirs = false;

        while ( srcItem != NULL )
        {
            newItem = static_cast<PlaylistItem*>( srcItem->nextSibling() );

            if ( srcItem->isSelected() )
            {
                urlList.append( srcItem->url() );

                if ( srcItem->isDir() )
                    containsDirs = true;
                // if drag is inside this widget, do a move operation
                if ( e->source() ->parent() == this )
                    delete srcItem;
            }
            srcItem = newItem;
        }
        if ( containsDirs && pApp->m_optDropMode == "Ask" )
        {
            QPopupMenu popup( this );
            popup.insertItem( i18n("Add Recursively"), this, SLOT( slotSetRecursive() ) );
            popup.exec( mapToGlobal( QPoint( e->pos().x() - 120, e->pos().y() - 20 ) ) );
        }

        setUpdatesEnabled( false );

        m_pDropCurrentItem = static_cast<PlaylistItem*>( itemAt( e->pos() ) );

        if ( !m_pDropCurrentItem )
            m_pDropCurrentItem = ( PlaylistItem* ) 1;

        playlistDrop( urlList );
    }

    setUpdatesEnabled( true );
    triggerUpdate();
    e->acceptAction();
}


void PlaylistWidget::playlistDrop( KURL::List urlList )
{
    ++m_dropRecursionCounter;

    for ( KURL::List::Iterator it = urlList.begin(); it != urlList.end(); it++ )
    {
        m_pDirLister->openURL( ( *it ).upURL(), false, false );   // URL; keep = true, reload = true
        while ( !m_pDirLister->isFinished() )
            kapp->processEvents( 300 );

        KFileItem *fileItem = m_pDirLister->findByURL( *it );

        if ( !fileItem )
            kdDebug() << "fileItem is 0!" << endl;

        if ( fileItem && fileItem->isDir() )
        {
            if ( fileItem->isLink() && !pApp->m_optFollowSymlinks && m_dropRecursionCounter >= 2 )
                continue;

            if ( m_dropRecursionCounter >= 50 )      //no infinite loops, please
                continue;

            if ( !m_dropRecursively && m_dropRecursionCounter >= 2 )
                continue;

            m_pDirLister->openURL( *it, false, false );  // URL; keep = false, reload = true
            while ( !m_pDirLister->isFinished() )
                kapp->processEvents( 300 );

            KURL::List dirList;
            KFileItemList myItems = m_pDirLister->items();
            KFileItemListIterator it( myItems );

            while ( *it )
            {
                if ( ( ( *it ) ->url().path() != "." ) && ( ( *it ) ->url().path() != ".." ) )
                    dirList.append( ( *it ) ->url() );
                ++it;
            }

            playlistDrop( dirList );
        }
        else
        {
            if ( pApp->m_pBrowserWin->isFileValid( *it ) )
            {
                m_pDropCurrentItem = addItem( m_pDropCurrentItem, *it );
            }
            else
            {
                if ( m_dropRecursionCounter <= 1 )
                    pApp->loadPlaylist( *it, m_pDropCurrentItem );
            }
        }
    }
    --m_dropRecursionCounter;
}



QListViewItem* PlaylistWidget::currentTrack()
{
    return m_pCurrentTrack;
}


void PlaylistWidget::setCurrentTrack( QListViewItem *item )
{
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


void PlaylistWidget::triggerSignalPlay()
{
    pApp->slotPlay();
}


void PlaylistWidget::focusInEvent( QFocusEvent *e )
{
    pApp->m_pBrowserWin->m_pPlaylistLineEdit->setFocus();

    KListView::focusInEvent( e );
}


PlaylistItem* PlaylistWidget::addItem( PlaylistItem *after, KURL url )
{
    PlaylistItem *pNewItem;

// we're abusing *after as a flag. value 1 == append to list
    if ( ( unsigned long ) after == 1 )
    {
        pNewItem = new PlaylistItem( this, lastItem(), url );
    }
    else
    {
        pNewItem = new PlaylistItem( this, after, url );
    }

    if ( pApp->m_optReadMetaInfo )
    {
        pNewItem->readMetaInfo();
        pNewItem->setMetaTitle();
    }

    return pNewItem;
}


// SLOTS ----------------------------------------------

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


void PlaylistWidget::slotSetRecursive()
{
    m_dropRecursively = true;
    kdDebug() << "slotSetRecursive()" << endl;
}


void PlaylistWidget::slotTextChanged( const QString &str )
{
    QListViewItem * pVisibleItem = NULL;
    QListViewItemIterator it( lastItem() );

    while ( *it )
    {
        if ( ( *it ) ->text( 0 ).lower().contains( str.lower() ) )
        {
            ( *it ) ->setVisible( true );
            pVisibleItem = ( *it );
        }
        else
            ( *it ) ->setVisible( false );

        --it;
    }

    clearSelection();
    triggerUpdate();

    if ( pVisibleItem )
    {
        setCurrentItem( pVisibleItem );
        setSelected( pVisibleItem, true );
    }
}


void PlaylistWidget::slotHeaderClicked( int section )
{
    QPopupMenu popup( this );
    int MENU_ASCENDING = popup.insertItem( i18n("Sort Ascending") );
    int MENU_DESCENDING = popup.insertItem( i18n("Sort Descending") );

    QPoint menuPos = QCursor::pos();
    menuPos.setX( menuPos.x() - 20 );

    int result = popup.exec( menuPos );

    if ( result == MENU_ASCENDING )
    {
        setSorting( section, true );
        sort();
        setSorting( -1 );
    }
    if ( result == MENU_DESCENDING )
    {
        setSorting( section, false );
        sort();
        setSorting( -1 );
    }
}


#include "playlistwidget.moc"
