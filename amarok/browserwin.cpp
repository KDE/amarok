/***************************************************************************
                        browserwin.cpp  -  description
                           -------------------
  begin                : Fre Nov 15 2002
  copyright            : (C) 2002 by Mark Kretschmann
  email                : markey@web.de
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
#include <qsplitter.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qsignalmapper.h> //PlaylistSideBar
#include <qobjectlist.h>   //setPaletteRecursively()

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <klineedit.h>
#include <klistview.h>
#include <kmultitabbar.h>
#include <kstandarddirs.h>
#include <kurl.h>


static void setPaletteRecursively( QWidget*, const QPalette&, const QColor& );


PlaylistSideBar::PlaylistSideBar( QWidget *parent )
    : QHBox( parent )
    , m_current( -1 )
    , m_MTB( new KMultiTabBar( KMultiTabBar::Vertical, this ) )
    , m_mapper( new QSignalMapper( this ) )
    , m_widgets( 2, (QWidget*)0 )
    , m_sizes( 2, 200 ) //basically this is the default value for the sidebar widths
{
    m_MTB->setStyle( KMultiTabBar::VSNET );
    m_MTB->setPosition( KMultiTabBar::Left );
    m_MTB->showActiveTabTexts( true ); 
    
    connect( m_mapper, SIGNAL( mapped( int ) ), this, SLOT( showHide( int ) ) );
}

QWidget *PlaylistSideBar::page( const QString &widgetName )
{
    for( uint i = 0; i <= m_widgets.size(); ++i )
        if( m_widgets[i] )
            if( widgetName == m_widgets[i]->name() )
                return m_widgets[i];
}

void PlaylistSideBar::setPageFont( const QFont &font )
{
    //note - untested because font system was broken when I made this
    for( uint i = 0; i <= m_widgets.size(); ++i )
        if( m_widgets[i] )
            m_widgets[i]->setFont( font );
}
    
QSize PlaylistSideBar::sizeHint() const
{
    //return a sizeHint that will make the splitter space our pages as the user expects
    return ( m_current != -1 ) ? QSize( m_sizes[ m_current ], 100 ) : m_MTB->sizeHint();
}

void PlaylistSideBar::addPage( QWidget *widget, const QString& text, const QString& icon, bool show )
{
    //we need to get count this way, currently it's the only way to do it coz KMultiTabBar sux0rs
    int id = m_MTB->tabs()->count();
    
    //FIXME can we do this with signals/slots?
    m_MTB->appendTab( KGlobal::iconLoader()->loadIcon( icon, KIcon::NoGroup, KIcon::SizeSmall ), id, text );
    QObject *tab = m_MTB->tab( id );
    connect( tab, SIGNAL( clicked() ), m_mapper, SLOT( map() ) );
    m_mapper->setMapping( tab, id );
    
    m_widgets[ id ] = widget;
    widget->hide();
    if( show ) showHide( id );
}

void PlaylistSideBar::showHide( int index )
{
    //FIXME please make me prettier!
    
    if( m_current != -1 && m_current != index )
    {
        m_sizes[ m_current ] = width();        
        m_widgets[ m_current ]->hide();
        m_MTB->tab( m_current )->setState( false );
    }
    
    QWidget  *w = m_widgets[ index ];
    bool isShut = w->isHidden();
    
    m_MTB->tab( index )->setState( isShut );
    if( isShut ) { w->show(); m_current = index; setMaximumWidth( 2000 ); }
    else         { w->hide(); m_current = -1; m_sizes[ index ] = width(); setMaximumWidth( m_MTB->maximumWidth() ); }
}

void PlaylistSideBar::close()
{
    if( m_current != -1 )
    {
        showHide( m_current );
    }
}

   

// CLASS BrowserWin =====================================================================

BrowserWin::BrowserWin( QWidget *parent, const char *name )
    : QWidget( parent, name, Qt::WType_TopLevel | Qt::WPaintUnclipped )
    , m_pActionCollection( new KActionCollection( this ) )
    , m_pSplitter( new QSplitter( this ) )
    , m_pSideBar( new PlaylistSideBar( m_pSplitter ) )
{
    setCaption( kapp->makeStdCaption( i18n( "Playlist" ) ) );
    setAcceptDrops( true );
    m_pSplitter->setResizeMode( m_pSideBar, QSplitter::FollowSizeHint );
    m_pSideBar ->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

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
             m_pSideBar, SLOT( close() ) );
          

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
{
    //FIXME sucks a little to get ptr this way
    // this method is good as it saves duplicate pointer to the fileBrowser
    KDevFileSelector *fileBrowser = (KDevFileSelector *)m_pSideBar->page( "FileBrowser" );
    
    //NOTE this doesn't seem to save anything yet..
    if( fileBrowser != NULL ) fileBrowser->writeConfig( kapp->sessionConfig(), "filebrowser" );
}

/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void BrowserWin::initChildren()
{
    //<Buttons>
    m_pButtonAdd     = new ExpandButton( i18n( "Add Item" ), this );

    m_pButtonClear   = new ExpandButton( i18n( "Clear" ), this );
    m_pButtonShuffle = new ExpandButton( i18n( "Shuffle" ), m_pButtonClear );
    m_pButtonSave    = new ExpandButton( i18n( "Save Playlist" ), m_pButtonClear );

    m_pButtonUndo    = new ExpandButton( i18n( "Undo" ), this );
    m_pButtonRedo    = new ExpandButton( i18n( "Redo" ), this );
    m_pButtonUndo      ->  setEnabled  ( false );
    m_pButtonRedo      ->  setEnabled  ( false );

    m_pButtonPlay    = new ExpandButton( i18n( "Play" ), this );
    m_pButtonPause   = new ExpandButton( i18n( "Pause" ), m_pButtonPlay );
    m_pButtonStop    = new ExpandButton( i18n( "Stop" ), m_pButtonPlay );
    m_pButtonNext    = new ExpandButton( i18n( "Next" ), m_pButtonPlay );
    m_pButtonPrev    = new ExpandButton( i18n( "Previous" ), m_pButtonPlay );
    //</Buttons>
    
    { //</FileBrowser>
        KDevFileSelector *w = new KDevFileSelector( m_pSideBar, "FileBrowser" );
        w->readConfig( kapp->sessionConfig(), "filebrowser" );
        m_pSideBar->addPage( w, "FileBrowser", "hdd_unmount", true );
    } //</FileBrowser>
        
    { //<StreamBrowser>
        QVBox   *vb = new QVBox( m_pSideBar );
        QWidget *b  = new QPushButton( "&Fetch Stream Information", vb );
        QObject *sb = new StreamBrowser( vb, "StreamBrowser" );    
        connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
        connect( b, SIGNAL( clicked() ),  b, SLOT( hide() ) );
        m_pSideBar->addPage( vb, "StreamBrowser", "network" );
    } //</StreamBrowser>
 
    { //<Playlist>
        QVBox *vb = new QVBox( m_pSplitter );
        m_pPlaylistLineEdit = new KLineEdit( vb );
        m_pPlaylistWidget   = new PlaylistWidget( vb );
        connect( m_pPlaylistLineEdit, SIGNAL( textChanged( const QString& ) ),
                m_pPlaylistWidget, SLOT( slotTextChanged( const QString& ) ) );
        connect( m_pPlaylistLineEdit, SIGNAL( returnPressed() ),
                m_pPlaylistWidget, SLOT( slotReturnPressed() ) );
        m_pSplitter->setResizeMode( vb, QSplitter::Auto );
        QToolTip::add( m_pPlaylistLineEdit, i18n( "Enter filter string" ) );             
    } //</Playlist>
    
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

/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

void BrowserWin::slotUpdateFonts()
{
    QFont font;

    if ( pApp->config()->useCustomFonts() )
        font = pApp->config()->browserWindowFont();

    m_pSideBar->setPageFont( font );
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
    QPalette pal( fg, bg );
    
    setPaletteRecursively( m_pPlaylistWidget,   pal, altbg );
    setPaletteRecursively( m_pPlaylistLineEdit, pal, altbg );
    setPaletteRecursively( m_pSideBar,          pal, altbg );
    
    update();
    m_pPlaylistWidget->triggerUpdate();
}


//Routine for setting palette recursively in a widget and all its childen
static void setPaletteRecursively( QWidget* widget, const QPalette &pal, const QColor& altbg )
{
    QObjectList *list = widget->queryList( "QWidget" );
    list->append( widget );
    
    for( QObject *obj = list->first(); obj; obj = list->next() )
    {
        static_cast<QWidget*>(obj)->setPalette( pal );
        if( obj->inherits( "KListView" ) )
        {
            KListView *lv = dynamic_cast<KListView *>(obj);
            if( lv ) lv->setAlternateBackground( altbg );
        }
    }        
}

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

void BrowserWin::closeEvent( QCloseEvent *e )
{
    e->accept();
    emit signalHide();
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


#include "browserwin.moc"
