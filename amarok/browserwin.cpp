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
#include "expandbutton.h"
#include "filebrowser.h"
#include "playerapp.h"
#include "playlistwidget.h"
#include "streambrowser.h"

#include "amarokconfig.h"

#include <qcolor.h>
#include <qfile.h>
#include <qlayout.h>
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
#include <klocale.h>
#include <klineedit.h>
#include <klistview.h>
#include <kmultitabbar.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kurlcompletion.h>
#include <kcombobox.h>



//the reason to make this class is because we need to overide sizeHint
//eventually expand it so things are more OO, currently this is very messsy

//NOTE I've tried to avoid the Q_OBJECT macro to save some bloat
class PlaylistSideBar : public QHBox
{
public:
    PlaylistSideBar( QWidget *parent ) : QHBox ( parent ), m_open( true ), m_savedSize( 100 ) {}

    virtual QSize sizeHint() const { return ( m_open ) ? QSize( m_savedSize, 100 ) : QHBox::sizeHint(); }
    
    void close() { if( m_open ) { m_savedSize = width(); m_open = false; } }
    void open()  { if( m_open ) { m_savedSize = width(); } m_open = true;  }

private:
    int  m_savedSize;
    bool m_open;
};
    


// CLASS BrowserWin =====================================================================

BrowserWin::BrowserWin( QWidget *parent, const char *name )
    : QWidget( parent, name, Qt::WType_TopLevel | Qt::WPaintUnclipped )
    , m_pActionCollection( new KActionCollection( this ) )
{
    setCaption( kapp->makeStdCaption( i18n( "Playlist" ) ) );
    setAcceptDrops( true );

    initChildren();

    KStdAction::undo( m_pPlaylistWidget, SLOT( doUndo() ), m_pActionCollection );
    KStdAction::redo( m_pPlaylistWidget, SLOT( doRedo() ), m_pActionCollection );
    KStdAction::copy( m_pPlaylistWidget, SLOT( copyAction() ), m_pActionCollection );

    connect( m_pPlaylistWidget, SIGNAL( sigUndoState( bool ) ),
             m_pButtonUndo, SLOT( setEnabled( bool ) ) );
    connect( m_pPlaylistWidget, SIGNAL( sigRedoState( bool ) ),
             m_pButtonRedo, SLOT( setEnabled( bool ) ) );
    connect( m_pPlaylistWidget, SIGNAL( cleared() ),
             m_pPlaylistLineEdit, SLOT( clear() ) );
    connect( m_pPlaylistWidget, SIGNAL( clicked( QListViewItem * ) ),
             this, SLOT( closeAllTabs() ) );
          

    connect( m_pButtonClear, SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( clear() ) );

    //FIXME <mxcl> MAKE_IT_CLEAN: kaction-ify
    connect( m_pButtonShuffle,  SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( shuffle() ) );

    //moved from playerapp
    connect( m_pButtonAdd, SIGNAL( clicked() ),
             this, SLOT( slotAddLocation() ) );

    connect( m_pButtonSave, SIGNAL( clicked() ),
             this, SLOT( savePlaylist() ) );

    connect( m_pButtonUndo, SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( doUndo() ) );

    connect( m_pButtonRedo, SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( doRedo() ) );
}


BrowserWin::~BrowserWin()
{}


// INIT -------------------------------------------------------------

void BrowserWin::initChildren()
{
    //<Containers>
    m_pSplitter   = new QSplitter( this );
    m_sideBar     = new PlaylistSideBar( m_pSplitter );
    QHBox *boxMTB = m_sideBar;
    QVBox *boxPL  = new QVBox( m_pSplitter );
    
    m_pSplitter->setResizeMode( boxMTB,  QSplitter::FollowSizeHint );
    m_pSplitter->setResizeMode( boxPL, QSplitter::Auto );    
    boxMTB->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    //</Containers>
    
    //<Buttons>
    m_pButtonAdd     = new ExpandButton( i18n( "Add Item" ), this );

    m_pButtonClear   = new ExpandButton( i18n( "Clear" ), this );
    m_pButtonShuffle = new ExpandButton( i18n( "Shuffle" ), m_pButtonClear );
    m_pButtonSave    = new ExpandButton( i18n( "Save Playlist" ), m_pButtonClear );

    m_pButtonUndo    = new ExpandButton( i18n( "Undo" ), this );
    m_pButtonRedo    = new ExpandButton( i18n( "Redo" ), this );
    m_pButtonUndo->        setEnabled( false );
    m_pButtonRedo->        setEnabled( false );

    m_pButtonPlay    = new ExpandButton( i18n( "Play" ), this );
    m_pButtonPause   = new ExpandButton( i18n( "Pause" ), m_pButtonPlay );
    m_pButtonStop    = new ExpandButton( i18n( "Stop" ), m_pButtonPlay );
    m_pButtonNext    = new ExpandButton( i18n( "Next" ), m_pButtonPlay );
    m_pButtonPrev    = new ExpandButton( i18n( "Previous" ), m_pButtonPlay );
    //</Buttons>

    //<MultTabBar>
    #define BROWSERBOX_ID 0
    #define STREAMBOX_ID 1
    m_pMultiTabBar = new KMultiTabBar( KMultiTabBar::Vertical, boxMTB );
    m_pMultiTabBar->setStyle( KMultiTabBar::VSNET );
    m_pMultiTabBar->setPosition( KMultiTabBar::Left );
    m_pMultiTabBar->showActiveTabTexts( true ); 
    m_pMultiTabBar->appendTab( KGlobal::iconLoader()->loadIcon( "hdd_unmount", KIcon::NoGroup,
                                                                KIcon::SizeSmall ), BROWSERBOX_ID, "Filebrowser" );
    m_pMultiTabBar->appendTab( KGlobal::iconLoader()->loadIcon( "network"    , KIcon::NoGroup,
                                                                KIcon::SizeSmall ), STREAMBOX_ID, "Streambrowser" );
    m_pMultiTabBar->tab( BROWSERBOX_ID  )->setState( true  );
    m_pMultiTabBar->tab( STREAMBOX_ID   )->setState( false );
    connect( m_pMultiTabBar->tab( BROWSERBOX_ID ), SIGNAL( clicked() ), this, SLOT( buttonBrowserClicked() ) );
    connect( m_pMultiTabBar->tab( STREAMBOX_ID ), SIGNAL( clicked() ), this, SLOT( buttonStreamClicked() ) );
    //<MultTabBar>
    
    //<StreamBrowser>
    m_pStreamBox        = new QVBox( boxMTB );
    QPushButton *button = new QPushButton( "&Fetch Stream Information", m_pStreamBox );
    m_pStreamBrowser    = new StreamBrowser( m_pStreamBox, "StreamBrowser" );
    m_pStreamBox->hide();    
    connect( button, SIGNAL( clicked() ), m_pStreamBrowser, SLOT( slotUpdateStations() ) );
    connect( button, SIGNAL( clicked() ), button, SLOT( hide() ) );
    //</StreamBrowser>
    
    //</FileBrowser>    
    m_pBrowserBox  = new KDevFileSelector( boxMTB );
    m_pFileBrowser = m_pBrowserBox;
    m_pBrowserBox->readConfig( kapp->sessionConfig(), "filebrowser" );
    m_pBrowserBox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    //</FileBrowser>

    //<Playlist>
    m_pPlaylistLineEdit = new KLineEdit( boxPL );
    m_pPlaylistWidget   = new PlaylistWidget( boxPL );
    connect( m_pPlaylistLineEdit, SIGNAL( textChanged( const QString& ) ),
             m_pPlaylistWidget, SLOT( slotTextChanged( const QString& ) ) );
    connect( m_pPlaylistLineEdit, SIGNAL( returnPressed() ),
             m_pPlaylistWidget, SLOT( slotReturnPressed() ) );
    QToolTip::add( m_pPlaylistLineEdit, i18n( "Enter filter string" ) );             
    //</Playlist>
    
    //<Layout>
    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_pSplitter );

    QBoxLayout *layH = new QHBoxLayout( layV );
    layH->addWidget( m_pButtonAdd );
    layH->addWidget( m_pButtonClear );
    layH->addWidget( m_pButtonUndo );
    layH->addWidget( m_pButtonRedo );
    layH->addWidget( m_pButtonPlay );
    //</Layout>
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
//    m_pBrowserLineEdit->setEditURL( url.prettyURL( 1 ) );
}


void BrowserWin::slotBrowserDoubleClicked( QListViewItem* pItem )
{
/*    if ( pItem )
    {
        FileBrowserItem *pBrowserItem = static_cast<FileBrowserItem *>( pItem );
        KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, pBrowserItem->url() );

        if ( pBrowserItem->text( 0 ) == ".." )
        {
            m_pBrowserWidget->cachedPath = m_pBrowserWidget->m_pDirLister->url().fileName(true);
            m_pBrowserWidget->readDir( m_pBrowserWidget->m_pDirLister->url().upURL() );
        }

        else if ( pBrowserItem->isDir() )
        {
            m_pBrowserWidget->readDir( fileItem.url() );
        }

        else m_pPlaylistWidget->insertMedia( fileItem.url() );
    }*/
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
  case Qt::Key_Prior:
  case Qt::Key_Next:
//  case Qt::Key_Return:
//  case Qt::Key_Enter:
  case Qt::Key_Delete:
     if( !m_pPlaylistWidget->hasFocus() )
     {
        //if hasFocus() then this event came from there, and we don't want to risk an infinite loop!
        m_pPlaylistWidget->setFocus();
        QApplication::sendEvent( m_pPlaylistWidget, e );
     }
     break;
/*
  //removed as risky, although useful
  default:
     if( !m_pPlaylistLineEdit->hasFocus() )
     {
        //if hasFocus() then this event came from there (99% sure)
        m_pPlaylistLineEdit->setFocus();
        QApplication::sendEvent( m_pPlaylistLineEdit, e );
     }
*/
  }

  e->accept(); //consume the event, ALERT! keypresses won't propagate to parent (good thing)
}


void BrowserWin::slotUpdateFonts()
{
    QFont font;

    if ( pApp->config()->useCustomFonts() )
    {
      font = pApp->config()->browserWindowFont();
    }

    m_pFileBrowser   ->setFont( font );
    m_pStreamBrowser ->setFont( font );
    m_pPlaylistWidget->setFont( font );
}


#include <kfiledialog.h>
#include <kurlrequester.h>
#include <kurlrequesterdlg.h>

void BrowserWin::savePlaylist()
{
/*    QString path = KFileDialog::getSaveFileName( m_pBrowserWidget->m_pDirLister->url().path(), "*.m3u" );

    if ( !path.isEmpty() )
    {
        if ( path.right( 4 ) != ".m3u" ) // <berkus> FIXME: 3.2 KFileDialog has a [x] Append file extension automagically, so we should obey the user choice
            path += ".m3u";

        m_pPlaylistWidget->saveM3u( path );
    }*/
}


void BrowserWin::slotAddLocation()
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( kapp->makeStdCaption( i18n( "Enter file or URL" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    m_pPlaylistWidget->insertMedia( dlg.selectedURL() );
}


void BrowserWin::setPalettes( const QColor &fg, const QColor &bg, const QColor &altbg )
{
    m_pPlaylistWidget->setPaletteBackgroundColor( bg );
    m_pPlaylistWidget->setPaletteForegroundColor( fg );
    
    m_pStreamBrowser->setPaletteBackgroundColor( bg );
    m_pStreamBrowser->setPaletteForegroundColor( fg );
    m_pStreamBrowser->setAlternateBackground( altbg );

    m_pPlaylistLineEdit->setPaletteBackgroundColor( bg );
    m_pPlaylistLineEdit->setPaletteForegroundColor( fg );

/*    m_pFileBrowser->setPaletteBackgroundColor( bg );
    m_pFileBrowser->setPaletteForegroundColor( fg );*/
    
    update();
    m_pFileBrowser->update();
    m_pPlaylistWidget->triggerUpdate();
}


//<KMultiTabBar handling>    FIXME I am ugly, please rewrite me! 
void BrowserWin::buttonBrowserClicked()
{
    if ( m_pBrowserBox->isHidden() )
    {    
        m_sideBar->open();
        m_pBrowserBox->show();
        m_pStreamBox->hide();
        m_pMultiTabBar->tab( BROWSERBOX_ID )->setState( true  );
        m_pMultiTabBar->tab( STREAMBOX_ID  )->setState( false );
    }
    else
    {
        m_sideBar->close();
        m_pBrowserBox->hide();
        m_pMultiTabBar->tab( BROWSERBOX_ID )->setState( false  );
    }
}


void BrowserWin::buttonStreamClicked()
{
    if ( m_pStreamBox->isHidden() )
    {    
        m_sideBar->open();
        m_pStreamBox->show();
        m_pBrowserBox->hide();
        m_pMultiTabBar->tab( BROWSERBOX_ID )->setState( false  );
        m_pMultiTabBar->tab( STREAMBOX_ID  )->setState( true );
    }
    else
    {
        m_sideBar->close();
        m_pStreamBox->hide();
        m_pMultiTabBar->tab( STREAMBOX_ID )->setState( false  );
    }
}
//</KMultiTabBar handling>
      

void BrowserWin::closeAllTabs()
{
    //Like KDevelop, hide browsers when user manipulates playlist
    //Feel free to revert this if it proves undesirable
    //FIXME not all inclusive behavior yet

    m_sideBar->close();
    m_pBrowserBox ->hide();
    m_pMultiTabBar->tab( BROWSERBOX_ID )->setState( false  );
    m_pStreamBox  ->hide();
    m_pMultiTabBar->tab( STREAMBOX_ID  )->setState( false );
}


#include "browserwin.moc"
