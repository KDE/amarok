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

#include <vector>

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
#include <kstandarddirs.h>
#include <ktip.h>
#include <kurl.h>
#include <kurlcompletion.h>
#include <kurlrequesterdlg.h>
#include <kcombobox.h>


// CLASS BrowserWin =====================================================================

BrowserWin::BrowserWin( QWidget *parent, const char *name )
    : QWidget( parent, name, Qt::WType_TopLevel | Qt::WType_Dialog | Qt::WPaintUnclipped )
    , m_pActionCollection( new KActionCollection( this ) )
{
    setCaption( kapp->makeStdCaption( i18n( "Playlist" ) ) );
    setAcceptDrops( true );

    initChildren();

    KStdAction::undo( m_pPlaylistWidget, SLOT( doUndo() ), m_pActionCollection );
    KStdAction::redo( m_pPlaylistWidget, SLOT( doRedo() ), m_pActionCollection );

    connect( m_pBrowserWidget, SIGNAL( doubleClicked( QListViewItem* ) ),
             this, SLOT( slotBrowserDoubleClicked( QListViewItem* ) ) );
    connect( m_pBrowserWidget, SIGNAL( browserDrop() ),
             m_pPlaylistWidget, SLOT( removeSelectedItems() ) );
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

    connect( m_pButtonClear, SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( clear() ) );

    //FIXME <mxcl> MAKE_IT_CLEAN: kaction-ify
    connect( m_pButtonShuffle,  SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( shuffle() ) );
}


BrowserWin::~BrowserWin()
{}


// INIT -------------------------------------------------------------

void BrowserWin::initChildren()
{
    m_pButtonAdd     = new ExpandButton( i18n( "Add Item" ), this );

    m_pButtonClear   = new ExpandButton( i18n( "Clear" ), this );
    m_pButtonShuffle = new ExpandButton( i18n( "Shuffle" ), m_pButtonClear );
    m_pButtonSave    = new ExpandButton( i18n( "Save Playlist" ), m_pButtonClear );

    m_pButtonUndo = new ExpandButton( i18n( "Undo" ), this );
    m_pButtonRedo = new ExpandButton( i18n( "Redo" ), this );
    m_pButtonUndo->setEnabled( false );
    m_pButtonRedo->setEnabled( false );

    m_pButtonPlay    = new ExpandButton( i18n( "Play" ), this );
    m_pButtonPause   = new ExpandButton( i18n( "Pause" ), m_pButtonPlay );
    m_pButtonStop    = new ExpandButton( i18n( "Stop" ), m_pButtonPlay );
    m_pButtonNext    = new ExpandButton( i18n( "Next" ), m_pButtonPlay );
    m_pButtonPrev    = new ExpandButton( i18n( "Previous" ), m_pButtonPlay );

    m_pSplitter      = new QSplitter( this );
    m_pJanusWidget   = new KJanusWidget( m_pSplitter, 0, KJanusWidget::IconList );
    
    //HACK Traverse childrenlist of KJanusWidget in order to find members which are not exposed in API
    //<mxcl> heh, this is so cheeky, take that encapsulation! Nice one Markey ;-)
    QObject *pIconBox = m_pJanusWidget->child( 0, "KListBox" );
    if ( pIconBox )    static_cast<QWidget*>( pIconBox )->setFocusPolicy( QWidget::NoFocus );
    QObject *pHeader = m_pJanusWidget->child( "KJanusWidgetTitleLabel" );
    if ( pHeader )     static_cast<QWidget*>( pHeader )->hide();
    QObject *pSeparator = m_pJanusWidget->child( 0, "KSeparator" );
    if ( pHeader )     static_cast<QWidget*>( pSeparator )->hide();
            
    QWidget *pBrowserBox = m_pJanusWidget->addPage( QString( i18n( "files" ) ), QString::null,
                           KGlobal::iconLoader()->loadIcon( "hdd_unmount", KIcon::NoGroup,
                           KIcon::SizeMedium ) );
/*  QWidget *pStreamBox =*/m_pJanusWidget->addPage( QString( i18n( "streams" ) ), QString::null,
                           KGlobal::iconLoader()->loadIcon( "network", KIcon::NoGroup,
                           KIcon::SizeMedium ) );
/*  QWidget *pMediaBox =*/ m_pJanusWidget->addPage( QString( i18n( "media" ) ), QString::null,
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
//    connect( m_pPlaylistLineEdit, SIGNAL( returnPressed() ),
//             m_pPlaylistWidget, SIGNAL( returnPressed() ) );

    QBoxLayout *layBrowserWidget = new QVBoxLayout( pBrowserBox );
    layBrowserWidget->addWidget( m_pBrowserLineEdit );
    layBrowserWidget->addWidget( m_pBrowserWidget );

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
    // TODO:  wait for damage extension and new xserver?
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

        else m_pPlaylistWidget->insertMedia( fileItem.url() );
    }
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
    popup.insertItem( i18n( "Remove Selected" ), m_pPlaylistWidget, SLOT( removeSelectedItems() ) );

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


void BrowserWin::keyPressEvent( QKeyEvent *e )
{
  //if the keypress is given to this widget then nothing is in focus
  //if the keypress was passed here from a childWidget that couldn't handle it then
  //we should note what is in focus and not send it back!

  //FIXME WARNING! there is a substantial risk of infinite looping here if the event is ignored by child event
  //               handlers it will be passed back to this function!

  //FIXME, you managed an infinite loop here. Damn (using filebrowserlineedit)

  kdDebug() << "BrowserWin::keyPressEvent()\n";

  switch( e->key() )
  {
  case Qt::Key_Up:
  case Qt::Key_Down:
  case Qt::Key_Left:
  case Qt::Key_Right:
  case Qt::Key_Prior: //don't work
  case Qt::Key_Next:  //don't work
  case Qt::Key_Return:
  case Qt::Key_Enter:
  case Qt::Key_Delete:
     if( !m_pPlaylistWidget->hasFocus() )
     {
        //if hasFocus() then this event came from there, and we don't want to risk an infinite loop!
        m_pPlaylistWidget->setFocus();
        QApplication::sendEvent( m_pPlaylistWidget, e );
     }
     break;

  default:
     if( !m_pPlaylistLineEdit->hasFocus() )
     {
        //if hasFocus() then this event came from there (99% sure)
        m_pPlaylistLineEdit->setFocus();
        QApplication::sendEvent( m_pPlaylistLineEdit, e );
     }
  }

  e->accept(); //consume the event
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
