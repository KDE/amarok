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
#include <qevent.h>
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
#include <kurl.h>
#include <kurldrag.h>
#include <klineedit.h>
#include <kaccel.h>


PlaylistWidget::PlaylistWidget(QWidget *parent, const char *name ) : KListView(parent,name)
{
    setName( "PlaylistWidget" );
    setFocusPolicy( QWidget::ClickFocus );
    setPaletteBackgroundColor( pApp->m_bgColor );
    setFullWidth( true );
    setCurrentTrack( NULL );
    m_playlistDirty = false;

    mGlowCount = 100;
    mGlowAdd = 5;
    mGlowColor.setRgb( 0xff, 0x40, 0x40 );

    mGlowTimer = new QTimer( this );
    connect( mGlowTimer, SIGNAL( timeout() ), this, SLOT( slotGlowTimer() ) );
    mGlowTimer->start( 50 );

    m_pDirLister = new KDirLister();
    m_pDirLister->setAutoUpdate( false );
}



PlaylistWidget::~PlaylistWidget()
{
}



// METHODS -----------------------------------------------------------------

void PlaylistWidget::contentsDragMoveEvent( QDragMoveEvent* e)
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

    if ( e->source() == NULL )                    // dragging from inside amarok or outside?
    {
        if( !KURLDrag::decode( e, urlList ) || urlList.isEmpty() )
            return;

        setUpdatesEnabled( false );
        m_pDropCurrentItem = static_cast<PlaylistItem*>( itemAt( e->pos() ) );
        playlistDrop( urlList );
    }
    else
    {
        PlaylistItem *srcItem, *newItem;

        if ( e->source()->parent() == this )
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
                if ( e->source()->parent() == this )
                    delete srcItem;
            }
            srcItem = newItem;
        }
        if ( containsDirs && pApp->m_optDropMode == "Ask" )
        {
            QPopupMenu popup( this );
            popup.insertItem( "Add Recursively", this, SLOT( slotSetRecursive() ) );
            popup.exec( mapToGlobal( QPoint( e->pos().x() - 120, e->pos().y() - 20 ) ) );
        }

        setUpdatesEnabled( false );

        m_pDropCurrentItem = static_cast<PlaylistItem*>( itemAt( e->pos() ) );

        if ( !m_pDropCurrentItem )
            m_pDropCurrentItem = (PlaylistItem*) 1;

        playlistDrop( urlList );
    }

    setUpdatesEnabled( true );
    triggerUpdate();
    e->acceptAction();
}



void PlaylistWidget::playlistDrop( KURL::List urlList )
{
    ++m_dropRecursionCounter;

    for( KURL::List::Iterator it = urlList.begin(); it != urlList.end(); it++ )
    {
        m_pDirLister->openURL( ( *it ).upURL(), false, false );   // URL; keep = true, reload = true
        while( !m_pDirLister->isFinished() )
            kapp->processEvents( 300 );

        KFileItem *fileItem = m_pDirLister->findByURL( *it );

        if ( !fileItem )
            kdDebug() << "fileItem is 0!" << endl;

        if ( fileItem && fileItem->isDir() )
        {
            if ( fileItem->isLink() && !pApp->m_optFollowSymlinks && m_dropRecursionCounter >= 2 )
                continue;

            if ( m_dropRecursionCounter >= 50 )     //no infinite loops, please
                continue;

            if ( !m_dropRecursively && m_dropRecursionCounter >= 2 )
                continue;

            m_pDirLister->openURL( *it, false, false );  // URL; keep = false, reload = true
            while( !m_pDirLister->isFinished() )
                kapp->processEvents( 300 );

            KURL::List dirList;
            KFileItemList myItems = m_pDirLister->items();
            KFileItemListIterator it( myItems );

            while ( *it )
            {
                if ( ( (*it)->url().path() != "." ) && ( (*it)->url().path() != ".." ) )
                    dirList.append( (*it)->url() );
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
    PlaylistItem *item = static_cast<PlaylistItem*>( firstChild() );

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



void PlaylistWidget::fetchMetaInfo()
{
    if ( m_playlistDirty )
    {
        PlaylistItem *item = static_cast<PlaylistItem*>( firstChild() );

        while( item )
        {
            if ( !item->metaInfo() )
            {
                item->readMetaInfo();
                item->setMetaTitle();
                break;
            }

            item = static_cast<PlaylistItem*>( item->nextSibling() );
        }

        if ( !item )
            m_playlistDirty = false;
    }
}



PlaylistItem* PlaylistWidget::addItem( PlaylistItem *after, KURL url )
{
    m_playlistDirty = true;

    if ( (unsigned long) after == 1 )
    {
        return new PlaylistItem( this, lastItem(), url );
    }
    else
    {
        return new PlaylistItem( this, after, url );
    }
}



// SLOTS ----------------------------------------------

void PlaylistWidget::slotGlowTimer()
{
    if ( !isVisible() )
        return;

    PlaylistItem *item = static_cast<PlaylistItem*>( currentTrack() );

    if ( item != NULL )
    {
        item->setGlowing( true );

        if ( mGlowCount > 120 )
        {
            mGlowAdd = -mGlowAdd;
        }
        if ( mGlowCount < 90 )
        {
            mGlowAdd = -mGlowAdd;
        }
        item->setGlowCol( mGlowColor.light( mGlowCount ) );
        repaintItem( item );
        mGlowCount += mGlowAdd;
    }
}



void PlaylistWidget::slotSetRecursive()
{
    m_dropRecursively = true;
    kdDebug() << "slotSetRecursive()" << endl;
}



void PlaylistWidget::slotTextChanged( const QString &str )
{
    QListViewItem *pVisibleItem = NULL;
    QListViewItemIterator it( lastItem() );
    
    while ( *it )
    {
        if ( (*it)->text( 0 ).lower().contains( str.lower() ) )
        {
            (*it)->setVisible( true );
            pVisibleItem = (*it);
        }
        else
            (*it)->setVisible( false );
        
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
