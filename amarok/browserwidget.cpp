/***************************************************************************
                          browserwidget.cpp  -  description
                             -------------------
    begin                : Don Nov 14 2002
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

#include "browserwidget.h"
#include "browserwin.h"
#include "playerapp.h"
#include "playlistitem.h"
#include "playlistwidget.h"

#include <qcstring.h>
#include <qmap.h>
#include <qpixmap.h>
#include <qwidget.h>

#include <kdebug.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klistview.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>

BrowserWidget::BrowserWidget( QWidget *parent, const char *name ) : KListView( parent,name )
{
    setName( "BrowserWidget" );
    setFocusPolicy( QWidget::ClickFocus );

    addColumn( i18n("Filebrowser") );
    setFullWidth( true );
    setAcceptDrops( true );
    m_Count = 0;

    m_pDirLister = new KDirLister();
    m_pDirLister->setAutoUpdate( true );
    connect( m_pDirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
}


BrowserWidget::~BrowserWidget()
{
}


// METHODS ------------------------------------------------------------------

void BrowserWidget::readDir( KURL url )
{
    m_pDirLister->openURL( url );
}


void BrowserWidget::contentsDragMoveEvent( QDragMoveEvent* e)
{
    e->acceptAction();

}


void BrowserWidget::contentsDropEvent( QDropEvent* e)
{
    if ( e->source() == NULL )                    // dragging from inside amarok or outside?
    {
        pApp->m_pBrowserWin->m_pPlaylistWidget->contentsDropEvent( e ); // from outside -> append URLs to playlist
        return;
    }

    if ( e->source()->parent() == this )          // reject drop if source is this widget
        return;

    e->acceptAction();
    emit browserDrop();
}


void BrowserWidget::focusInEvent( QFocusEvent *e )
{
    pApp->m_pBrowserWin->m_pBrowserLineEdit->setFocus();

    KListView::focusInEvent( e );
}


// SLOTS ------------------------------------------------------------------

void BrowserWidget::slotCompleted()
{
    clear();

    pApp->m_pBrowserWin->m_pBrowserLineEdit->setURL( m_pDirLister->url() );

    KFileItemList myItems = m_pDirLister->items();
    KFileItemListIterator it( myItems );

    QMap<QString, KFileItem*> itemMap;

// put KFileItems in a QMap for sorting
    while ( *it )
    {
        itemMap[ ( *it )->url().path().lower() ] = *it;
        ++it;
    }

    PlaylistItem *item;
    QMap<QString, KFileItem*>::Iterator itMap;

// iterate over the map twice: 1. fetch dirs 2. fetch files
    bool fetchDirs = true;
    for ( int i = 0; i < 2; i++ )
    {
        for ( itMap = itemMap.begin(); itMap != itemMap.end(); ++itMap )
        {
            if ( itMap.data()->isDir() == fetchDirs )
            {
                item = new PlaylistItem( this, lastChild(), itMap.data()->url() );
                item->setDir( fetchDirs );
                item->setDragEnabled( true );
                item->setDropEnabled( true );

                QString iconName( ( itMap.data() )->determineMimeType()->icon( QString::null, true ) );
                item->setPixmap( 0, KGlobal::iconLoader()->loadIcon( iconName, KIcon::NoGroup, KIcon::SizeSmall ) );
            }
        }
        fetchDirs = false;
    }
    if ( m_pDirLister->url().path() != "/" )
        new PlaylistItem( this, ".." );

    clearSelection();
    setCurrentItem( firstChild() );
    setSelected( firstChild(), true );
    triggerUpdate();
}


void BrowserWidget::slotReturnPressed( const QString &str )
{
    readDir( str );
}


#include "browserwidget.moc"
