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
#include <qvbox.h>
#include <qwidget.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcompletion.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kjanuswidget.h>
#include <klocale.h>
#include <klineedit.h>
#include <klistview.h>
#include <krandomsequence.h>
#include <kstandarddirs.h>
#include <ktip.h>
#include <kurl.h>
#include <kurlcompletion.h>
#include <kurlrequesterdlg.h>
#include <kcombobox.h>


// CLASS BrowserWin =====================================================================

BrowserWin::BrowserWin( QWidget *parent, const char *name ) :
        QWidget( parent, name, Qt::WType_TopLevel | Qt::WType_Dialog | Qt::WPaintUnclipped ),
        m_pActionCollection( new KActionCollection( this ) )
{
    setCaption( kapp->makeStdCaption( i18n( "Playlist" ) ) );
    setAcceptDrops( true );

    initChildren();

    KStdAction::undo( m_pPlaylistWidget, SLOT( doUndo() ), m_pActionCollection );
    KStdAction::redo( m_pPlaylistWidget, SLOT( doRedo() ), m_pActionCollection );
    KStdAction::prior( this, SLOT( slotKeyPageUp() ), m_pActionCollection );
    KStdAction::next( this, SLOT( slotKeyPageDown() ), m_pActionCollection );

    //These slots are EVIL!
    //FIXME: rely on widgets themselves handle these events when they are in focus - unless really necessary!
    //       the reason I say this as I just spent an hour hunting for the cause of a bug that caused
    //       enter to make the browser go up a directory, and the cause was the ENTER KAction below
    //Question is do we really need to always catch the delete key? Personally I don't see why <mxcl>
    new KAction( i18n( "Go one item up" ), Key_Up,
                 this, SLOT( slotKeyUp() ), m_pActionCollection, "up" );
    new KAction( i18n( "Go one item down" ), Key_Down,
                 this, SLOT( slotKeyDown() ), m_pActionCollection, "down" );
    new KAction( i18n( "Enter directory / Play Track" ), /*ALT + */Key_Return,
                 this, SLOT( slotKeyEnter() ), m_pActionCollection, "enter" );
    new KAction( i18n( "Remove item" ), ALT + Key_Delete,
                 this, SLOT( slotKeyDelete() ), m_pActionCollection, "delete" );

    connect( m_pBrowserWidget, SIGNAL( doubleClicked( QListViewItem* ) ),
             this, SLOT( slotBrowserDoubleClicked( QListViewItem* ) ) );
    connect( m_pBrowserWidget, SIGNAL( browserDrop() ),
             this, SLOT( slotBrowserDrop() ) );
    connect( m_pBrowserWidget, SIGNAL( directoryChanged( const KURL& ) ),
             this, SLOT( setBrowserURL( const KURL& ) ) );
    connect( m_pBrowserWidget, SIGNAL( focusIn() ),
             m_pBrowserLineEdit, SLOT( setFocus() ) );

    connect( m_pPlaylistWidget, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this, SLOT( slotPlaylistRightButton( QListViewItem*, const QPoint& ) ) );
    connect( m_pPlaylistWidget, SIGNAL( sigUndoState( bool ) ),
             m_pButtonUndo, SLOT( setEnabled( bool ) ) );
    connect( m_pPlaylistWidget, SIGNAL( sigRedoState( bool ) ),
             m_pButtonRedo, SLOT( setEnabled( bool ) ) );
    connect( m_pPlaylistWidget, SIGNAL( cleared() ),
             m_pPlaylistLineEdit, SLOT( clear() ) );

    //FIXME <mxcl> MAKE_IT_CLEAN: kaction-ify
    connect( m_pButtonShuffle, SIGNAL( clicked() ),
             this, SLOT( slotShufflePlaylist() ) );
}


BrowserWin::~BrowserWin()
{}


// INIT -------------------------------------------------------------

void BrowserWin::initChildren()
{
    m_pButtonAdd = new ExpandButton( i18n( "Add Item" ), this );

    m_pButtonClear   = new ExpandButton( i18n( "Clear" ), this );
    m_pButtonShuffle = new ExpandButton( i18n( "Shuffle" ), m_pButtonClear );
    m_pButtonSave    = new ExpandButton( i18n( "Save Playlist" ), m_pButtonClear );

    m_pButtonUndo = new ExpandButton( i18n( "Undo" ), this );

    m_pButtonRedo = new ExpandButton( i18n( "Redo" ), this );

    m_pButtonPlay  = new ExpandButton( i18n( "Play" ), this );
    m_pButtonPause = new ExpandButton( i18n( "Pause" ), m_pButtonPlay );
    m_pButtonStop  = new ExpandButton( i18n( "Stop" ), m_pButtonPlay );
    m_pButtonNext  = new ExpandButton( i18n( "Next" ), m_pButtonPlay );
    m_pButtonPrev  = new ExpandButton( i18n( "Previous" ), m_pButtonPlay );

    m_pSplitter = new QSplitter( this );
    m_pJanusWidget = new KJanusWidget( m_pSplitter, 0, KJanusWidget::IconList );
    
    //Traverse childrenlist of KJanusWidget to find and hide that darn QLabel and KSeparator
    QObject *pHeader = m_pJanusWidget->child( "KJanusWidgetTitleLabel" );
    if ( pHeader )     static_cast<QWidget*>( pHeader )->hide();
    QObject *pSeparator = m_pJanusWidget->child( 0, "KSeparator" );
    if ( pHeader )     static_cast<QWidget*>( pSeparator )->hide();
            
    QVBox *pBrowserBox = m_pJanusWidget->addVBoxPage( QString( "Filebrowser" ), QString::null,
                         KGlobal::iconLoader()->loadIcon( "hdd_unmount", KIcon::NoGroup,
                         KIcon::SizeMedium ) );
    QVBox *pStreamBox =  m_pJanusWidget->addVBoxPage( QString( "Streambrowser" ), QString::null,
                         KGlobal::iconLoader()->loadIcon( "network", KIcon::NoGroup,
                         KIcon::SizeMedium ) );
    QVBox *pVirtualBox = m_pJanusWidget->addVBoxPage( QString( "Virtual Folders" ), QString::null,
                         KGlobal::iconLoader()->loadIcon( "folder_sound", KIcon::NoGroup,
                         KIcon::SizeMedium ) );

    //<mxcl> MAKE_IT_CLEAN: move to browserWidget, also use a validator to make sure has trailing /
    m_pBrowserLineEdit = new KHistoryCombo( true, pBrowserBox );
    m_pBrowserLineEdit->setPaletteBackgroundColor( pApp->m_bgColor );
    m_pBrowserLineEdit->setPaletteForegroundColor( pApp->m_fgColor );
    m_pBrowserLineEdit->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
    m_pBrowserLineEdit->setDuplicatesEnabled( false );
    m_pBrowserLineEdit->setMinimumWidth( 1 );
                            
    m_pBrowserWidget = new BrowserWidget( pBrowserBox );
    m_pBrowserWidget->setAcceptDrops( true );
    m_pBrowserWidget->setSorting( -1 );
    m_pBrowserWidget->setSelectionMode( QListView::Extended );

    QWidget *pPlaylistWidgetContainer = new QWidget( m_pSplitter );
    m_pPlaylistWidget = new PlaylistWidget( pPlaylistWidgetContainer );
    m_pPlaylistWidget->setAcceptDrops( true );
    m_pPlaylistWidget->setSelectionMode( QListView::Extended );

   //<mxcl> MAKE_IT_CLEAN: move to playlistWidget implementation
    m_pPlaylistLineEdit = new KLineEdit( pPlaylistWidgetContainer );
    m_pPlaylistLineEdit->setPaletteBackgroundColor( pApp->m_bgColor );
    m_pPlaylistLineEdit->setPaletteForegroundColor( pApp->m_fgColor );
        
    connect( m_pBrowserLineEdit, SIGNAL( activated( const QString& ) ),
             m_pBrowserWidget, SLOT( slotReturnPressed( const QString& ) ) );
    connect( m_pBrowserLineEdit, SIGNAL( returnPressed( const QString& ) ),
             m_pBrowserLineEdit, SLOT( addToHistory( const QString& ) ) );

    connect( m_pPlaylistLineEdit, SIGNAL( textChanged( const QString& ) ),
             m_pPlaylistWidget, SLOT( slotTextChanged( const QString& ) ) );
    connect( m_pPlaylistLineEdit, SIGNAL( returnPressed() ),
             m_pPlaylistWidget, SLOT( slotReturnPressed() ) );

//     QBoxLayout *layBrowserWidget = new QVBoxLayout( pBrowserBox );
//     layBrowserWidget->addWidget( m_pBrowserLineEdit );
//     layBrowserWidget->addWidget( m_pBrowserWidget );

    QBoxLayout *layPlaylistWidget = new QVBoxLayout( pPlaylistWidgetContainer );
    layPlaylistWidget->addWidget( m_pPlaylistLineEdit );
    layPlaylistWidget->addWidget( m_pPlaylistWidget );

    m_pSplitter->setResizeMode( m_pJanusWidget, QSplitter::Stretch );
    m_pSplitter->setResizeMode( pPlaylistWidgetContainer, QSplitter::Stretch );

    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_pSplitter );

    QBoxLayout *layH = new QHBoxLayout( layV );
    layH->addWidget( m_pButtonAdd );
    layH->addWidget( m_pButtonClear );
    layH->addWidget( m_pButtonUndo );
    layH->addWidget( m_pButtonRedo );
    layH->addWidget( m_pButtonPlay );

    QToolTip::add( m_pBrowserLineEdit, i18n( "Enter directory/URL" ) );
    QToolTip::add( m_pPlaylistLineEdit, i18n( "Enter Filter String" ) );
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

//<mxcl> MAKE_IT_CLEAN: move to browserWidget
void BrowserWin::setBrowserURL( const KURL& url )
{
   m_pBrowserLineEdit->setEditURL( url.prettyURL( 1 ) );
}

//<mxcl> MAKE_IT_CLEAN: some should be in playlistWidget some in browserWidget
void BrowserWin::slotBrowserDoubleClicked( QListViewItem* pItem )
{
    if ( pItem )
    {
        PlaylistItem *pPlayItem = static_cast<PlaylistItem*>( pItem );
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

//<mxcl> MAKE_IT_CLEAN: playlistWidget can shuffle itself
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

    m_pPlaylistWidget->writeUndo();
}


//<mxcl> MAKE_IT_CLEAN: move to playlistWidget, perhaps use QDragObject (not much point though)
void BrowserWin::slotBrowserDrop()
{
    QListViewItem * item, *item1;
    item = m_pPlaylistWidget->firstChild();

    while ( item != NULL )
    {
        item1 = item;
        item = item->nextSibling();

        if ( item1->isSelected() )
        {
            PlaylistItem *pItem = static_cast<PlaylistItem*>( item1 );
            m_pPlaylistWidget->removeItem(pItem);
        }
    }

    m_pPlaylistWidget->writeUndo();
}

//<mxcl> MAKE_IT_CLEAN: playlist can do this itself
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

//<mxcl> MAKE_IT_CLEAN: playlist can do this itself
void BrowserWin::slotShowInfo()
{
    PlaylistItem * pItem = static_cast<PlaylistItem*>( m_pPlaylistWidget->currentItem() );

    // FIXME KMessageBoxize?
    QMessageBox *box = new QMessageBox( "Track Information", 0,
                                        QMessageBox::Information, QMessageBox::Ok, QMessageBox::NoButton,
                                        QMessageBox::NoButton, 0, "Track Information", true,
                                        Qt::WDestructiveClose | Qt::WStyle_DialogBorder );

    QString str( "<html><body><table border=""1"">" );

    if ( pItem->hasMetaInfo() )
    {
         str += "<tr><td>" + i18n( "Title"   ) + "</td><td>" + pItem->title()   + "</td></tr>";
         str += "<tr><td>" + i18n( "Artist"  ) + "</td><td>" + pItem->artist()  + "</td></tr>";
         str += "<tr><td>" + i18n( "Album"   ) + "</td><td>" + pItem->album()   + "</td></tr>";
         str += "<tr><td>" + i18n( "Genre"   ) + "</td><td>" + pItem->genre()   + "</td></tr>";
         str += "<tr><td>" + i18n( "Year"    ) + "</td><td>" + pItem->year()    + "</td></tr>";
         str += "<tr><td>" + i18n( "Comment" ) + "</td><td>" + pItem->comment() + "</td></tr>";
         str += "<tr><td>" + i18n( "Length"  ) + "</td><td>" + pItem->length()  + "</td></tr>";
         str += "<tr><td>" + i18n( "Bitrate" ) + "</td><td>" + QString::number(pItem->bitrate()) + " kbps</td></tr>";
         str += "<tr><td>" + i18n( "Samplerate" ) + "</td><td>" + QString::number(pItem->samplerate()) + " Hz</td></tr>";
    }
    else
    {
        str += "<tr><td>" + i18n( "Stream" ) + "</td><td>" + pItem->url().prettyURL() + "</td></tr>";
        str += "<tr><td>" + i18n( "Title"  ) + "</td><td>" + pItem->text( 0 ) + "</td></tr>";
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
/*

    FIXME: this means that pushing enter in the BrowserLineEdit activates the lineEdit AND the listView!
    FIXME: don't repair this code until we move the lineEdit and listview into a single (layout derived) widget class
    FIXME: and even then, why are we taking this kind of event out of the widgets themselves and doing it at this level, this is just asking for bugs isn't it?

    if ( m_pBrowserLineEdit->hasFocus() )
    {
        if ( m_pBrowserWidget->currentItem() )
        {
            slotBrowserDoubleClicked( m_pBrowserWidget->currentItem() );
        }
    }
*/
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
    QFont font;

    if ( pApp->m_optUseCustomFonts )
    {
      font = pApp->m_optBrowserWindowFont;
    }

    m_pBrowserWidget->setFont( font );
    m_pPlaylistWidget->setFont( font );
}


#include "browserwin.moc"
