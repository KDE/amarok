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

#include "amarokfilelist.h"
#include "browserwidget.h"
#include "browserwin.h"
#include "playerapp.h"
#include "playlistitem.h"
#include "playlistwidget.h"

#include <qcstring.h>
#include <qcursor.h>
#include <qdir.h>
#include <qheader.h>
#include <qmap.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qwidget.h>

#include <kdebug.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kcombobox.h> //FIXME: we shouldn't have to include this, instead make signals that can connect with this when required
#include <klistview.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kpopupmenu.h>
#include <kurl.h>
#include <kurldrag.h>


BrowserWidget::BrowserWidget( QWidget *parent, const char *name )
   : KListView( parent,name ),
     m_pDirLister( new KDirLister() ),
     m_Count( 0 )
{
    setFocusPolicy( QWidget::ClickFocus );

    addColumn( i18n( "Filebrowser" ) );
    setFullWidth( true );
    setAcceptDrops( true );
    setDragEnabled( true ); //NEW

    connect( header(), SIGNAL( clicked( int ) ), this, SLOT( slotHeaderClicked( int ) ) );

    m_pDirLister->setAutoUpdate( true );
    connect( m_pDirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
}


BrowserWidget::~BrowserWidget()
{
  delete m_pDirLister;
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


QDragObject *BrowserWidget::dragObject()
{
  KURL::List::List list;

  for( PlaylistItem *item = (PlaylistItem *)firstChild(); item != NULL; item = (PlaylistItem *)item->nextSibling() )
    if( item->isSelected() )
      list.append( item->url() );

  return new KURLDrag( list, this, "FileBrowserDragObject" );
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
    pApp->m_pBrowserWin->m_pBrowserLineEdit->setEditURL( m_pDirLister->url() );

    AmarokFileList fileList( m_pDirLister->items(), pApp->m_optBrowserSortSpec );
    KFileItemListIterator it( fileList );
    PlaylistItem *item;

    while ( *it )
    {
        item = new PlaylistItem( this, lastChild(), (*it)->url() );
        item->setDir( (*it)->isDir() );
        item->setDragEnabled( true );
        item->setDropEnabled( true );

        QString iconName( (*it)->determineMimeType()->icon( QString::null, true ) );
        item->setPixmap( 0, KGlobal::iconLoader()->loadIcon( iconName, KIcon::NoGroup, KIcon::SizeSmall ) );
        ++it;
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


void BrowserWidget::slotHeaderClicked( int )
{
    KPopupMenu popup( this );

    popup.insertTitle( i18n( "Sorted by" ) );

    int MENU_NAME = popup.insertItem( i18n( "Name" ) );
    popup.setItemChecked( MENU_NAME, ( pApp->m_optBrowserSortSpec & QDir::SortByMask ) == QDir::Name );
    int MENU_DATE = popup.insertItem( i18n( "Date" ) );
    popup.setItemChecked( MENU_DATE, ( pApp->m_optBrowserSortSpec & QDir::SortByMask ) == QDir::Time );
    int MENU_SIZE = popup.insertItem( i18n( "Size" ) );
    popup.setItemChecked( MENU_SIZE, ( pApp->m_optBrowserSortSpec & QDir::SortByMask ) == QDir::Size );
    int MENU_UNSORTED = popup.insertItem( i18n( "Unsorted" ) );
    popup.setItemChecked( MENU_UNSORTED, ( pApp->m_optBrowserSortSpec & QDir::SortByMask ) == QDir::Unsorted );

    popup.insertSeparator();

    int MENU_REVERSE = popup.insertItem( i18n( "Reverse" ) );
    popup.setItemChecked( MENU_REVERSE, pApp->m_optBrowserSortSpec & QDir::Reversed );
    int MENU_DIRSFIRST = popup.insertItem( i18n( "Directories First" ) );
    popup.setItemChecked( MENU_DIRSFIRST, pApp->m_optBrowserSortSpec & QDir::DirsFirst );
    int MENU_CASE = popup.insertItem( i18n( "Case Insensitive" ) );
    popup.setItemChecked( MENU_CASE, pApp->m_optBrowserSortSpec & QDir::IgnoreCase );

    if ( popup.isItemChecked( MENU_UNSORTED ) )
    {
        popup.setItemEnabled( MENU_REVERSE, false );
        popup.setItemEnabled( MENU_DIRSFIRST, false );
        popup.setItemEnabled( MENU_CASE, false );
    }

    QPoint menuPos = QCursor::pos();
    menuPos.setX( menuPos.x() - 20 );
    menuPos.setY( menuPos.y() + 10 );

    int result = popup.exec( menuPos );

    if ( result == MENU_NAME )
    {
        pApp->m_optBrowserSortSpec &= ~QDir::SortByMask;
        pApp->m_optBrowserSortSpec |= QDir::Name;
    }

    if ( result == MENU_DATE )
    {
        pApp->m_optBrowserSortSpec &= ~QDir::SortByMask;
        pApp->m_optBrowserSortSpec |= QDir::Time;
    }

    if ( result == MENU_SIZE )
    {
        pApp->m_optBrowserSortSpec &= ~QDir::SortByMask;
        pApp->m_optBrowserSortSpec |= QDir::Size;
    }

    if ( result == MENU_UNSORTED )
    {
        pApp->m_optBrowserSortSpec = QDir::Unsorted;
    }

    if ( result == MENU_REVERSE )
    {
        if ( popup.isItemChecked( MENU_REVERSE ) )
            pApp->m_optBrowserSortSpec &= ~QDir::Reversed;
        else
            pApp->m_optBrowserSortSpec |= QDir::Reversed;
    }

    if ( result == MENU_DIRSFIRST )
    {
        if ( popup.isItemChecked( MENU_DIRSFIRST ) )
            pApp->m_optBrowserSortSpec &= ~QDir::DirsFirst;
        else
            pApp->m_optBrowserSortSpec |= QDir::DirsFirst;
    }

    if ( result == MENU_CASE )
    {
        if ( popup.isItemChecked( MENU_CASE ) )
            pApp->m_optBrowserSortSpec &= ~QDir::IgnoreCase;
        else
            pApp->m_optBrowserSortSpec |= QDir::IgnoreCase;
    }

    // update view
    slotCompleted();
}


#include "browserwidget.moc"
