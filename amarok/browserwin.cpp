/***************************************************************************
                        browserwin.cpp  -  description
                           -------------------
  begin                : Fre Nov 15 2002
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

#include "browserwin.h"
#include "browserwidget.h"
#include "playlistwidget.h"
#include "playlistitem.h"
#include "expandbutton.h"
#include "playerapp.h"

#include "debugareas.h"

#include <vector>

#include <qbitmap.h>
#include <qcolor.h>
#include <qfile.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qsplitter.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qwidget.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcompletion.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kfilemetainfo.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <klineedit.h>
#include <klistview.h>
#include <krandomsequence.h>
#include <kstandarddirs.h>
#include <ktip.h>
#include <kurl.h>
#include <kurlcompletion.h>
#include <kurlrequesterdlg.h>


// CLASS BrowserWin =====================================================================

BrowserWin::BrowserWin( QWidget *parent, const char *name ) :
        QWidget( parent, name, Qt::WPaintUnclipped )
{
    setName( "BrowserWin" );
    setCaption( kapp->makeStdCaption( i18n( "Playlist" ) ) );
    setAcceptDrops( true );

    initChildren();
    
    m_pActionCollection = new KActionCollection( this );
    KStdAction::undo( m_pPlaylistWidget, SLOT( doUndo() ), m_pActionCollection );
    KStdAction::redo( m_pPlaylistWidget, SLOT( doRedo() ), m_pActionCollection );
    KStdAction::prior( this, SLOT( slotKeyPageUp() ), m_pActionCollection );
    KStdAction::next( this, SLOT( slotKeyPageDown() ), m_pActionCollection );
    
    new KAction( "Go one item up", Key_Up,
                 this, SLOT( slotKeyUp() ), m_pActionCollection, "up" );
    new KAction( "Go one item down", Key_Down,
                 this, SLOT( slotKeyDown() ), m_pActionCollection, "down" );
    new KAction( "Enter directory / Play Track", ALT + Key_Return,
                 this, SLOT( slotKeyEnter() ), m_pActionCollection, "enter" );
    new KAction( "Remove item", ALT + Key_Delete,
                 this, SLOT( slotKeyDelete() ), m_pActionCollection, "delete" );
    
    connect( m_pBrowserWidget, SIGNAL( doubleClicked( QListViewItem* ) ),
             this, SLOT( slotBrowserDoubleClicked( QListViewItem* ) ) );

    connect( m_pBrowserWidget, SIGNAL( browserDrop() ),
             this, SLOT( slotBrowserDrop() ) );

    connect( m_pPlaylistWidget, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this, SLOT( slotPlaylistRightButton( QListViewItem*, const QPoint& ) ) );

    connect( m_pPlaylistWidget, SIGNAL( sigUndoState( bool ) ),
             m_pButtonUndo, SLOT( setEnabled( bool ) ) );

    connect( m_pPlaylistWidget, SIGNAL( sigRedoState( bool ) ),
             m_pButtonRedo, SLOT( setEnabled( bool ) ) );

    connect( m_pPlaylistWidget, SIGNAL( cleared() ),
             m_pPlaylistLineEdit, SLOT( clear() ) );

    connect( m_pButtonShuffle, SIGNAL( clicked() ),
             this, SLOT( slotShufflePlaylist() ) );

    /*    connect( pApp, SIGNAL( sigUpdateFonts() ),
                 this, SLOT( slotUpdateFonts() ) );*/
}


BrowserWin::~BrowserWin()
{}


// INIT -------------------------------------------------------------

void BrowserWin::initChildren()
{
    m_pButtonAdd = new ExpandButton( i18n( "Add Item" ), this );

    m_pButtonClear = new ExpandButton( i18n( "Clear" ), this );
    QToolTip::add( m_pButtonClear, i18n( "Keep button pressed for sub-menu" ) );
    m_pButtonShuffle = new ExpandButton( i18n( "Shuffle" ), m_pButtonClear );
    m_pButtonSave = new ExpandButton( i18n( "Save Playlist" ), m_pButtonClear );

    m_pButtonUndo = new ExpandButton( i18n( "Undo" ), this );

    m_pButtonRedo = new ExpandButton( i18n( "Redo" ), this );

    m_pButtonPlay = new ExpandButton( i18n( "Play" ), this );
    QToolTip::add( m_pButtonPlay, i18n( "Keep button pressed for sub-menu" ) );
    m_pButtonPause = new ExpandButton( i18n( "Pause" ), m_pButtonPlay );
    m_pButtonStop = new ExpandButton( i18n( "Stop" ), m_pButtonPlay );
    m_pButtonNext = new ExpandButton( i18n( "Next" ), m_pButtonPlay );
    m_pButtonPrev = new ExpandButton( i18n( "Previous" ), m_pButtonPlay );

    m_pSplitter = new QSplitter( this );

    QWidget *pBrowserWidgetContainer = new QWidget( m_pSplitter );
    m_pBrowserWidget = new BrowserWidget( pBrowserWidgetContainer );
    m_pBrowserWidget->setAcceptDrops( true );
    m_pBrowserWidget->setSorting( -1 );
    m_pBrowserWidget->setSelectionMode( QListView::Extended );

    QWidget *pPlaylistWidgetContainer = new QWidget( m_pSplitter );
    m_pPlaylistWidget = new PlaylistWidget( pPlaylistWidgetContainer );
    m_pPlaylistWidget->setAcceptDrops( true );
    m_pPlaylistWidget->setSelectionMode( QListView::Extended );

    m_pBrowserLineEdit = new KLineEdit( pBrowserWidgetContainer );
    QToolTip::add( m_pBrowserLineEdit, i18n( "Enter directory/URL" ) );
    m_pBrowserLineEdit->setPaletteBackgroundColor( pApp->m_bgColor );
    m_pBrowserLineEdit->setPaletteForegroundColor( pApp->m_fgColor );
    KURLCompletion *compBrowser = new KURLCompletion( KURLCompletion::DirCompletion );
    //disabled because the popup combo is useful
    //    m_pBrowserLineEdit->setCompletionMode( KGlobalSettings::CompletionAuto );
    m_pBrowserLineEdit->setCompletionObject( compBrowser );
    connect( m_pBrowserLineEdit, SIGNAL( returnPressed( const QString& ) ),
             m_pBrowserWidget, SLOT( slotReturnPressed( const QString& ) ) );

    m_pPlaylistLineEdit = new KLineEdit( pPlaylistWidgetContainer );
    QToolTip::add( m_pPlaylistLineEdit, i18n( "Enter Filter String" ) );
    m_pPlaylistLineEdit->setPaletteBackgroundColor( pApp->m_bgColor );
    m_pPlaylistLineEdit->setPaletteForegroundColor( pApp->m_fgColor );
    connect( m_pPlaylistLineEdit, SIGNAL( textChanged( const QString& ) ),
             m_pPlaylistWidget, SLOT( slotTextChanged( const QString& ) ) );

    QBoxLayout *layBrowserWidget = new QVBoxLayout( pBrowserWidgetContainer );
    layBrowserWidget->addWidget( m_pBrowserLineEdit );
    layBrowserWidget->addWidget( m_pBrowserWidget );

    QBoxLayout *layPlaylistWidget = new QVBoxLayout( pPlaylistWidgetContainer );
    layPlaylistWidget->addWidget( m_pPlaylistLineEdit );
    layPlaylistWidget->addWidget( m_pPlaylistWidget );

    m_pSplitter->setResizeMode( pBrowserWidgetContainer, QSplitter::Stretch );
    m_pSplitter->setResizeMode( pPlaylistWidgetContainer, QSplitter::Stretch );

    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_pSplitter );

    QBoxLayout *layH = new QHBoxLayout( layV );
    layH->addWidget( m_pButtonAdd );
    layH->addWidget( m_pButtonClear );
    layH->addWidget( m_pButtonUndo );
    layH->addWidget( m_pButtonRedo );
    layH->addWidget( m_pButtonPlay );
}


// METHODS -----------------------------------------------------------------

void BrowserWin::closeEvent( QCloseEvent *e )
{
    e->accept();

    emit signalHide();
}


void BrowserWin::moveEvent( QMoveEvent * )
{
    // FIXME: needed for PlaylistWidget transparency
    /*    m_pPlaylistWidget->repaint();
        m_pPlaylistWidget->viewport()->repaint();*/
}


void BrowserWin::paintEvent( QPaintEvent * )
{
    /*    m_pPlaylistWidget->repaint();
        m_pPlaylistWidget->viewport()->repaint();*/
}


// SLOTS --------------------------------------------------------------------

void BrowserWin::slotBrowserDoubleClicked( QListViewItem* pItem )
{
    if ( pItem )
    {
        PlaylistItem * pPlayItem = static_cast<PlaylistItem*>( pItem );
        KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, pPlayItem->url() );

        if ( pPlayItem->text( 0 ) == ".." )
        {
            m_pBrowserWidget->readDir( m_pBrowserWidget->m_pDirLister->url().upURL() );
        }

        else if ( pPlayItem->isDir() )
        {
            m_pBrowserWidget->readDir( fileItem.url() );
        }

        else if ( pApp->isFileValid( fileItem.url() ) )
            m_pPlaylistWidget->addItem( ( PlaylistItem* ) 1, fileItem.url() );
    }
}


void BrowserWin::slotShufflePlaylist()
{
    // not evil, but corrrrect :)
    QPtrList<QListViewItem> list;

    while ( m_pPlaylistWidget->childCount() )
    {
        list.append( m_pPlaylistWidget->firstChild() );
        m_pPlaylistWidget->takeItem( m_pPlaylistWidget->firstChild() );
    }

    // initalize with seed
    KRandomSequence seq( static_cast<long>( KApplication::random() ) );
    seq.randomize( &list );

    for ( unsigned int i = 0; i < list.count(); i++ )
    {
        m_pPlaylistWidget->insertItem( list.at( i ) );
    }
}


void BrowserWin::slotBrowserDrop()
{
    QListViewItem * item, *item1;
    item = m_pPlaylistWidget->firstChild();

    while ( item != NULL )
    {
        item1 = item;
        item = item->nextSibling();

        if ( item1->isSelected() )
            delete item1;
    }

    m_pPlaylistWidget->writeUndo();
}


void BrowserWin::slotPlaylistRightButton( QListViewItem * /*pItem*/, const QPoint &rPoint )
{
    QPopupMenu popup( this );
    int item1 = popup.insertItem( i18n( "Show File Info" ), this, SLOT( slotShowInfo() ) );

    // only enable when file is selected
    if ( !m_pPlaylistWidget->currentItem() )
        popup.setItemEnabled( item1, false );

    popup.insertItem( i18n( "Play Track" ), this, SLOT( slotMenuPlay() ) );
    popup.insertItem( i18n( "Remove Selected" ), this, SLOT( slotBrowserDrop() ) );

    popup.exec( rPoint );
}


void BrowserWin::slotShowInfo()
{
    PlaylistItem * pItem = static_cast<PlaylistItem*>( m_pPlaylistWidget->currentItem() );

    // FIXME KMessageBoxize
    QMessageBox *box = new QMessageBox( "Track Information", 0,
                                        QMessageBox::Information, QMessageBox::Ok, QMessageBox::NoButton,
                                        QMessageBox::NoButton, 0, "Track Information", true,
                                        Qt::WDestructiveClose | Qt::WStyle_DialogBorder );

    QString str( "<html><body><table border=""1"">" );

    if ( pItem->url().protocol() == "file" )
    {
        KFileMetaInfo metaInfo( pItem->url().path(), QString::null, KFileMetaInfo::Everything );
        //    KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, pItem->url() );
        //    KFileMetaInfo metaInfo = fileItem.metaInfo();
        if ( metaInfo.isValid() && !metaInfo.isEmpty() )
        {
            str += "<tr><td>Title   </td><td>" + metaInfo.item( "Title" ).string() + "</td></tr>";
            str += "<tr><td>Artist  </td><td>" + metaInfo.item( "Artist" ).string() + "</td></tr>";
            str += "<tr><td>Album   </td><td>" + metaInfo.item( "Album" ).string() + "</td></tr>";
            str += "<tr><td>Genre   </td><td>" + metaInfo.item( "Genre" ).string() + "</td></tr>";
            str += "<tr><td>Date    </td><td>" + metaInfo.item( "Date" ).string() + "</td></tr>";
            str += "<tr><td>Comment </td><td>" + metaInfo.item( "Comment" ).string() + "</td></tr>";
            str += "<tr><td>Length  </td><td>" + metaInfo.item( "Length" ).string() + "</td></tr>";
            str += "<tr><td>Bitrate </td><td>" + metaInfo.item( "Bitrate" ).string() + "</td></tr>";
            str += "<tr><td>Samplerate  </td><td>" + metaInfo.item( "Sample Rate" ).string() + "</td></tr>";
        }
        else
        {
            str += "<tr><td>Error    </td><td>No metaInfo found    </td></tr>";
        }
    }
    else
    {
        str += "<tr><td>Stream   </td><td>" + pItem->url().url() + "</td></tr>";
        str += "<tr><td>Title    </td><td>" + pItem->text( 0 ) + "</td></tr>";
    }

    str.append( "</table></body></html>" );
    box->setText( str );
    box->setTextFormat( Qt::RichText );
    box->show();
}


void BrowserWin::slotMenuPlay()
{
    m_pPlaylistWidget->setCurrentTrack( m_pPlaylistWidget->currentItem() );
    pApp->slotPlay();
}


void BrowserWin::slotKeyUp()
{
    KListView * pListView = 0L;

    if ( m_pPlaylistLineEdit->hasFocus() )
        pListView = m_pPlaylistWidget;

    if ( m_pBrowserLineEdit->hasFocus() )
        pListView = m_pBrowserWidget;

    QListViewItem *item = pListView->currentItem();

    if ( item->itemAbove() )
    {
        item = item->itemAbove();

        pListView->setSelected( pListView->currentItem(), false );
        pListView->ensureItemVisible( item );
        pListView->setSelected( item, true );
        pListView->setCurrentItem( item );
    }
}


void BrowserWin::slotKeyDown()
{
    KListView * pListView = 0L;

    if ( m_pPlaylistLineEdit->hasFocus() )
        pListView = m_pPlaylistWidget;

    if ( m_pBrowserLineEdit->hasFocus() )
        pListView = m_pBrowserWidget;

    QListViewItem *item = pListView->currentItem();

    if ( item->itemBelow() )
    {
        item = item->itemBelow();

        pListView->setSelected( pListView->currentItem(), false );
        pListView->ensureItemVisible( item );
        pListView->setSelected( item, true );
        pListView->setCurrentItem( item );
    }
}


void BrowserWin::slotKeyPageUp()
{
    KListView * pListView = 0L;

    if ( m_pPlaylistLineEdit->hasFocus() )
        pListView = m_pPlaylistWidget;

    if ( m_pBrowserLineEdit->hasFocus() )
        pListView = m_pBrowserWidget;

    QListViewItem *item = pListView->currentItem();

    for ( int i = 1; i < pListView->visibleHeight() / item->height(); i++ )
    {
        if ( item->itemAbove() == NULL )
            break;
        item = item->itemAbove();
    }

    if ( item )
    {
        pListView->setSelected( pListView->currentItem(), false );
        pListView->ensureItemVisible( item );
        pListView->setSelected( item, true );
        pListView->setCurrentItem( item );
    }
}


void BrowserWin::slotKeyPageDown()
{
    KListView * pListView = 0L;

    if ( m_pPlaylistLineEdit->hasFocus() )
        pListView = m_pPlaylistWidget;

    if ( m_pBrowserLineEdit->hasFocus() )
        pListView = m_pBrowserWidget;

    QListViewItem *item = pListView->currentItem();

    for ( int i = 1; i < pListView->visibleHeight() / item->height(); i++ )
    {
        if ( item->itemBelow() == NULL )
            break;
        item = item->itemBelow();
    }

    if ( item )
    {
        pListView->setSelected( pListView->currentItem(), false );
        pListView->ensureItemVisible( item );
        pListView->setSelected( item, true );
        pListView->setCurrentItem( item );
    }
}


void BrowserWin::slotKeyEnter()
{
    if ( m_pPlaylistLineEdit->hasFocus() )
    {
        if ( m_pPlaylistWidget->currentItem() )
        {
            pApp->slotStop();
            m_pPlaylistWidget->setCurrentTrack( m_pPlaylistWidget->currentItem() );
            pApp->slotPlay();
        }
    }

    if ( m_pBrowserLineEdit->hasFocus() )
    {
        if ( m_pBrowserWidget->currentItem() )
        {
            slotBrowserDoubleClicked( m_pBrowserWidget->currentItem() );
        }
    }
}


void BrowserWin::slotKeyDelete()
{
    if ( m_pPlaylistLineEdit->hasFocus() )
    {
        slotBrowserDrop();
    }

    if ( m_pBrowserLineEdit->hasFocus() )
    {}
}


void BrowserWin::slotUpdateFonts()
{
    m_pBrowserWidget->setFont( pApp->m_optBrowserWindowFont );
    m_pPlaylistWidget->setFont( pApp->m_optBrowserWindowFont );
}


#include "browserwin.moc"
