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
#include "playerapp.h"

#include "amarokconfig.h"

#include <qapplication.h>
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
#include <klistview.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kpopupmenu.h>
#include <kurl.h>
#include <kurldrag.h>


BrowserWidget::BrowserWidget( QWidget *parent, const char *name )
   : KListView( parent,name )
   , m_pDirLister( new KDirLister() )
{
    addColumn( i18n( "Filebrowser" ) );

    setFocusPolicy( QWidget::ClickFocus );
    setAcceptDrops( true );
    setDragEnabled( true ); //NEW
    header()->setMovingEnabled( false );
    m_pDirLister->setAutoUpdate( true );

    connect( header(), SIGNAL( clicked( int ) ), this, SLOT( slotHeaderClicked( int ) ) );
    connect( m_pDirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
}


BrowserWidget::~BrowserWidget()
{
  delete m_pDirLister;
}


// METHODS ------------------------------------------------------------------

void BrowserWidget::readDir( const KURL &url )
{
    m_pDirLister->openURL( url );
}


void BrowserWidget::contentsDragEnterEvent( QDragEnterEvent* e)
{
    //FIXME, I guess we need to derive our own mimetype and do this properly at some point
    //       and only accept drags from the playlist
    if( e->source() && QString( e->source()->parent()->name() ) == "PlaylistWidget" )
    {
        e->accept();
    }
    else
    {
        e->ignore();
    }
}


QDragObject *BrowserWidget::dragObject()
{
  KURL::List::List list;

  for( QListViewItem *item = firstChild(); item; item = item->nextSibling() )
    if( item->isSelected() )
      list.append( static_cast<FileBrowserItem *>(item)->url() );

  return new KURLDrag( list, this, "FileBrowserDragObject" );
}


void BrowserWidget::contentsDropEvent( QDropEvent* e)
{
    //for some reason the source is qt_viewport and the parent is the playlistWidget
    if( e->source() && QString( e->source()->parent()->name() ) == "PlaylistWidget" )
    {
        e->acceptAction();
        emit browserDrop();
    }
}


#include <kcombobox.h>
#include "browserwin.h"
void BrowserWidget::keyPressEvent( QKeyEvent *e )
{
   switch( e->key() )
   {
   case Key_Backspace: case Key_Colon: case Key_Semicolon: case Key_Slash:
   case Key_A: case Key_B: case Key_C: case Key_D: case Key_E: case Key_F: case Key_G: case Key_H: case Key_I: case Key_J: case Key_K: case Key_L: case Key_M: case Key_N: case Key_O: case Key_P: case Key_Q: case Key_R: case Key_S: case Key_T: case Key_U: case Key_V: case Key_W: case Key_X: case Key_Y: case Key_Z:
     {
      //by ignoring these key presses we propagate them to the LineEdit
/*      QLineEdit *le = pApp->m_pBrowserWin->m_pBrowserLineEdit->lineEdit();
      le->setFocus();
      QApplication::sendEvent( le, e );*/
      break;
     }
   default:
      KListView::keyPressEvent( e );
      //the base handler will set accept() or ignore()
      //TODO if ignored by above pass to LineEdit too?
   }
}



// SLOTS ------------------------------------------------------------------

void BrowserWidget::slotCompleted()
{
    clear();

    AmarokFileList fileList( m_pDirLister->items(), pApp->config()->browserSortingSpec() );
    KFileItemListIterator it( fileList );
    FileBrowserItem *item;

    while ( *it )
    {
        item = new FileBrowserItem( this, lastChild(), (*it)->url(), (*it)->isDir() );

        QString iconName( (*it)->determineMimeType()->icon( QString::null, true ) );
        item->setPixmap( 0, KGlobal::iconLoader()->loadIcon( iconName, KIcon::NoGroup, KIcon::SizeSmall ) );
        ++it;
    }

    if ( m_pDirLister->url().path() != "/" )
        new FileBrowserItem( this );

    clearSelection();

    if ( !cachedPath.isEmpty() )
    {
        //FIXME why is slotCompleted() called twice after a directory change? cachedPath (normally) needs to be emptied after this calls
        setCurrentItem( findItem( cachedPath, 0, Qt::ExactMatch ) );
        setSelected( findItem( cachedPath, 0, Qt::ExactMatch ), true );
        ensureItemVisible( findItem( cachedPath, 0, Qt::ExactMatch ) );
    } else
    {
        setCurrentItem( firstChild() );
        setSelected( firstChild(), true );
    }

    triggerUpdate();

    emit directoryChanged( m_pDirLister->url() );
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
    popup.setItemChecked( MENU_NAME, ( pApp->config()->browserSortingSpec() & QDir::SortByMask ) == QDir::Name );
    int MENU_DATE = popup.insertItem( i18n( "Date" ) );
    popup.setItemChecked( MENU_DATE, ( pApp->config()->browserSortingSpec() & QDir::SortByMask ) == QDir::Time );
    int MENU_SIZE = popup.insertItem( i18n( "Size" ) );
    popup.setItemChecked( MENU_SIZE, ( pApp->config()->browserSortingSpec() & QDir::SortByMask ) == QDir::Size );
    int MENU_UNSORTED = popup.insertItem( i18n( "Unsorted" ) );
    popup.setItemChecked( MENU_UNSORTED, ( pApp->config()->browserSortingSpec() & QDir::SortByMask ) == QDir::Unsorted );

    popup.insertSeparator();

    int MENU_REVERSE = popup.insertItem( i18n( "Reverse" ) );
    popup.setItemChecked( MENU_REVERSE, pApp->config()->browserSortingSpec() & QDir::Reversed );
    int MENU_DIRSFIRST = popup.insertItem( i18n( "Directories First" ) );
    popup.setItemChecked( MENU_DIRSFIRST, pApp->config()->browserSortingSpec() & QDir::DirsFirst );
    int MENU_CASE = popup.insertItem( i18n( "Case Insensitive" ) );
    popup.setItemChecked( MENU_CASE, pApp->config()->browserSortingSpec() & QDir::IgnoreCase );

    if ( popup.isItemChecked( MENU_UNSORTED ) )
    {
        popup.setItemEnabled( MENU_REVERSE, false );
        popup.setItemEnabled( MENU_DIRSFIRST, false );
        popup.setItemEnabled( MENU_CASE, false );
    }

    QPoint menuPos = QCursor::pos();
    //menuPos.setX( menuPos.x() - 20 );
    //menuPos.setY( menuPos.y() + 10 );

    int result = popup.exec( menuPos );

    if ( result == MENU_NAME )
    {
        pApp->config()->setBrowserSortingSpec((pApp->config()->browserSortingSpec() & ~QDir::SortByMask) | QDir::Name);
    }

    if ( result == MENU_DATE )
    {
        pApp->config()->setBrowserSortingSpec((pApp->config()->browserSortingSpec() & ~QDir::SortByMask) | QDir::Time);
    }

    if ( result == MENU_SIZE )
    {
        pApp->config()->setBrowserSortingSpec((pApp->config()->browserSortingSpec() & ~QDir::SortByMask) | QDir::Size);
    }

    if ( result == MENU_UNSORTED )
    {
        pApp->config()->setBrowserSortingSpec(QDir::Unsorted);
    }

    if ( result == MENU_REVERSE )
    {
        header()->setSortIndicator( 0, !popup.isItemChecked( MENU_REVERSE ) );

        if ( popup.isItemChecked( MENU_REVERSE ) )
            pApp->config()->setBrowserSortingSpec(pApp->config()->browserSortingSpec() & ~QDir::Reversed);
        else
            pApp->config()->setBrowserSortingSpec(pApp->config()->browserSortingSpec() | QDir::Reversed);
    }

    if ( result == MENU_DIRSFIRST )
    {
        if ( popup.isItemChecked( MENU_DIRSFIRST ) )
            pApp->config()->setBrowserSortingSpec(pApp->config()->browserSortingSpec() & ~QDir::DirsFirst);
        else
            pApp->config()->setBrowserSortingSpec(pApp->config()->browserSortingSpec() | QDir::DirsFirst);
    }

    if ( result == MENU_CASE )
    {
        if ( popup.isItemChecked( MENU_CASE ) )
            pApp->config()->setBrowserSortingSpec(pApp->config()->browserSortingSpec() & ~QDir::IgnoreCase);
        else
            pApp->config()->setBrowserSortingSpec(pApp->config()->browserSortingSpec() | QDir::IgnoreCase);
    }

    // update view
    slotCompleted();
}


#include "browserwidget.moc"
