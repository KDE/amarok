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

#include "amarokconfig.h"
#include "browserwin.h"
#include "filebrowser.h"
#include "playerapp.h"
#include "playlistsidebar.h"
#include "playlistwidget.h"
#include "streambrowser.h"

#include <qcolor.h>        //setPalettes()
#include <qevent.h>        //eventFilter()
#include <qlayout.h>
#include <qobjectlist.h>   //setPaletteRecursively()
#include <qpalette.h>      //setPalettes()
#include <qpopupmenu.h>    //BrowserWin ctor
#include <qsplitter.h>     //m_splitter
#include <qtooltip.h>      //QToolTip::add()
#include <qvbox.h>         //contains the playlist

#include <kaction.h>       //m_actionCollection
#include <kapplication.h>  //kapp
#include <kcursor.h>       //customEvent()
#include <kdebug.h>
#include <kglobal.h>
#include <kfiledialog.h>   //savePlaylist()
#include <kiconloader.h>   //multiTabBar icons
#include <klineedit.h>     //m_lineEdit
#include <klocale.h>
#include <kstandarddirs.h> //KGlobal::dirs()
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kurldrag.h>      //eventFilter()
#include <kurlrequester.h>    //slotAddLocation()
#include <kurlrequesterdlg.h> //slotAddLocation()


//Routine for setting palette recursively in a widget and all its childen
//NOTE I didn't make this a member as there was no need and we may like to move it at some point
static void setPaletteRecursively( QWidget* widget, const QPalette &pal, const QColor& bgAlt )
{
    QObjectList *list = widget->queryList( "QWidget" );
    list->append( widget );

    for( QObject *obj = list->first(); obj; obj = list->next() )
    {
        static_cast<QWidget*>(obj)->setPalette( pal );

        if( obj->inherits( "KListView" ) )
        {
            KListView *lv = dynamic_cast<KListView *>(obj); //slow, but safe
            if( lv ) lv->setAlternateBackground( bgAlt );
        }
    }
}


BrowserWin::BrowserWin( QWidget *parent, const char *name )
   : QWidget( parent, name, Qt::WType_TopLevel | Qt::WNoAutoErase )
   , m_pActionCollection( new KActionCollection( this ) )
   , m_splitter( new QSplitter( this ) )
   , m_sideBar( new PlaylistSideBar( m_splitter ) )
   , m_playlist( 0 )
   , m_lineEdit( 0 )
{
    setCaption( kapp->makeStdCaption( i18n( "Playlist" ) ) );

    //TODO pass it an engine pointer and it'll connect up various signals
    //this is cool because Qt is cool and not compile check is neccessary!

    /*
    QToolButton *clearButton = new QToolButton( this );
    //KApplication::reverseLayout() ? "clear_left" : "locationbar_erase"
    clearButton->setIconSet( SmallIconSet( "locationbar_erase" ) );
    connect( clearButton, SIGNAL( clicked() ), m_lineEdit, SLOT( clear() ) );
    */
    KToolBar* toolbar = new KToolBar( this );
    toolbar->setIconText( KToolBar::IconTextBottom );

    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_splitter );
    layV->addWidget( toolbar );

    QVBox *box  = new QVBox( m_splitter );

    QHBox *boxH = new QHBox( box );
    m_lineEdit  = new KLineEdit( boxH );

    m_playlist = new PlaylistWidget( box, m_pActionCollection );
    m_splitter->setResizeMode( m_sideBar, QSplitter::FollowSizeHint );
    m_splitter->setResizeMode( box,       QSplitter::Auto );
    m_sideBar->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    /* Here because we need m_playlist initialized. */
    QPushButton *showCurrentTrack = new QPushButton( boxH );
    showCurrentTrack->setPixmap( KGlobal::iconLoader()->loadIcon( "2uparrow", KIcon::NoGroup, KIcon::SizeSmall ) );
    QToolTip::add( showCurrentTrack, i18n( "Scroll to currently playing item" ) );
    connect( showCurrentTrack, SIGNAL( clicked() ), m_playlist, SLOT(showCurrentTrack()) );

    {//<ToolBar>
        QPopupMenu* actions_popup = new QPopupMenu( this );
        m_playlist->m_clearButton->plug( actions_popup );
        actionCollection()->action( "shuffle_playlist" )->plug( actions_popup  );
        actionCollection()->action( "save_playlist" )->plug( actions_popup  );

        toolbar->insertButton( "fileopen",    id_addItem,         true, i18n( "Add Item" ) );
        connect( toolbar->getButton( id_addItem ), SIGNAL( clicked() ), this, SLOT( slotAddLocation() ) );

        toolbar->insertButton( "midi",        id_playlistActions, true, i18n( "Playlist Actions" ) );
        toolbar->getButton( id_playlistActions )->setDelayedPopup( actions_popup );
        actionCollection()->action( "show_current_track" )->plug( toolbar );

        toolbar->insertLineSeparator();

        m_playlist->m_undoButton->plug( toolbar );
        m_playlist->m_redoButton->plug( toolbar );

        toolbar->insertLineSeparator();

        //FIXME replace with actions provided by engine
        toolbar->insertButton( "player_rew", id_prev,            true, "Previous" );
        connect( toolbar->getButton( id_prev ), SIGNAL( clicked() ), pApp, SLOT( slotPrev() ) );

        toolbar->insertButton( "player_play", id_play,            true, "Play" );
        connect( toolbar->getButton( id_play ), SIGNAL( clicked() ), pApp, SLOT( slotPlay() ) );

        toolbar->insertButton( "player_pause", id_pause,          true, "Pause" );
        connect( toolbar->getButton( id_pause ), SIGNAL( clicked() ), pApp, SLOT( slotPause() ) );

        toolbar->insertButton( "player_stop", id_stop,            true, "Stop" );
        connect( toolbar->getButton( id_stop ), SIGNAL( clicked() ), pApp, SLOT( slotStop() ) );

        toolbar->insertButton( "player_fwd", id_next,             true, "Next" );
        connect( toolbar->getButton( id_next ), SIGNAL( clicked() ), pApp, SLOT( slotNext() ) );
    }//</ToolBar>


    //</FileBrowser>
        m_sideBar->addPage( new KDevFileSelector( 0, "FileBrowser" ), i18n( "File Browser" ), "hdd_unmount" );
    //</FileBrowser>

    //</PlaylistBrowser>
    //    m_sideBar->addPage( m_playlist->browser(), i18n( "Playlist Browser" ), "midi" );
    //</PlaylistBrowser>

    { //<StreamBrowser>
        QVBox   *vb = new QVBox( 0, "StreamBrowser" );
        QWidget *b  = new QPushButton( "&Fetch Stream Information", vb );
        QObject *sb = new StreamBrowser( vb );
        connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
        connect( b, SIGNAL( clicked() ),  b, SLOT( deleteLater() ) );
        m_sideBar->addPage( vb, i18n( "Stream Browser" ), "network" );
    } //</StreamBrowser>


    //we intercept keyEvents to the playlist
    m_playlist->installEventFilter( this );
    //we intercept keyEvents to the lineEdit
    m_lineEdit->installEventFilter( this );


    connect( m_playlist, SIGNAL( aboutToClear() ),
             m_lineEdit,   SLOT( clear() ) );
    //FIXME you need to detect focus out from the sideBar and connect to that..
    connect( m_playlist, SIGNAL( clicked( QListViewItem * ) ),
             m_sideBar,    SLOT( autoClosePages() ) );
    connect( m_lineEdit, SIGNAL( textChanged( const QString& ) ),
             m_playlist,   SLOT( slotTextChanged( const QString& ) ) );

    QToolTip::add( m_lineEdit, i18n( "Enter filter string" ) );
}


BrowserWin::~BrowserWin()
{
    //TODO save at regular intervals, (use the QWidget built in timer as they have less overhead)

    if( AmarokConfig::savePlaylist() )
        m_playlist->saveM3u( defaultPlaylistPath() );
}



///////// public interface

void BrowserWin::insertMedia( const KURL::List &list, bool clearList, bool directPlay )
{
    if( clearList ) m_playlist->clear(); //FIXME clear currently is not 100% bug free, it might not work as expected

    m_playlist->insertMedia( list, directPlay );
}


bool BrowserWin::isAnotherTrack() const
{
    return m_playlist->isAnotherTrack() || AmarokConfig::repeatPlaylist();
}


void BrowserWin::setColors( const QPalette &pal, const QColor &bgAlt )
{
    m_lineEdit->setPalette( pal );
    m_playlist->setColors( pal, bgAlt ); //due to private inheritance nasty
    setPaletteRecursively( m_sideBar, pal, bgAlt );

    //update()
    //m_playlist->triggerUpdate();

    //TODO perhaps this should be a global member of some singleton (I mean bgAlt not just the filebrowser bgAlt!)
    KDevFileSelector::altBgColor = bgAlt;
}


void BrowserWin::setFont( const QFont &newFont )
{
    m_sideBar ->setFont( newFont );
    m_playlist->setFont( newFont );
}


void BrowserWin::saveConfig()
{
    //FIXME sucks a little to get ptr this way
    //FIXME instead force the widgets to derive from SideBarWidget or something
    // this method is good as it saves having duplicate pointers to the fileBrowser
    KDevFileSelector *fileBrowser = (KDevFileSelector *)m_sideBar->page( "FileBrowser" );
    fileBrowser->writeConfig();
}


QString BrowserWin::defaultPlaylistPath() const
{
    return KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "current.m3u";
}



//////// private interface

bool BrowserWin::eventFilter( QObject *o, QEvent *e )
{
    //filters events for a few of the widgets we are parent to
    //this makes life easier since we have more useful functions available from here

    switch( e->type() )
    {
    case QEvent::KeyPress:

        //there are a few keypresses that we override

        #define e static_cast<QKeyEvent*>(e)
        if( o == m_lineEdit )
        {
            //FIXME inefficient to always construct this
            QListViewItemIterator it( m_playlist, QListViewItemIterator::Visible );
            if( 0 == it.current() ) return FALSE;

            switch( e->key() )
            {
            case Key_Down:
                m_playlist->setFocus();
                m_playlist->setCurrentItem( it.current() );
                it.current()->setSelected( true ); //FIXME why doesn't it do this for us?
                return TRUE;

            case Key_Return:
            case Key_Enter:
                m_lineEdit->clear();
                m_playlist->activate( it.current() );
                m_playlist->ensureItemVisible( it.current() );
                return TRUE;

            default:
                return FALSE;
            }
        }

        //following are for m_playlist only
        //we don't handle these in the playlist because often we manipulate the lineEdit too

        if( e->key() == Key_Up && m_playlist->currentItem()->itemAbove() == 0 )
        {
            m_playlist->currentItem()->setSelected( false );
            m_lineEdit->setFocus();
            return TRUE;
        }

        if( e->key() == Key_Delete )
        {
            m_playlist->removeSelectedItems();
            return TRUE;
        }

        if( ( e->key() >= Key_0 && e->key() <= Key_Z ) || e->key() == Key_Backspace )
        {
            m_lineEdit->setFocus();
            QApplication::sendEvent( m_lineEdit, e );
            return TRUE;
        }
        #undef e
        break;

    default:
        break;
    }

    return FALSE;
}


void BrowserWin::savePlaylist() const //SLOT
{
    QWidget *fb = m_sideBar->page( "FileBrowser" );
    QString path = fb ? static_cast<KDevFileSelector *>(fb)->location() : "~";

    path = KFileDialog::getSaveFileName( path, "*.m3u" );

    if( !path.isEmpty() )
    {
        m_playlist->saveM3u( path );
    }
}


void BrowserWin::slotAddLocation() //SLOT
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( kapp->makeStdCaption( i18n( "Enter File, URL or Directory" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    insertMedia( dlg.selectedURL() );
}


#include "browserwin.moc"

