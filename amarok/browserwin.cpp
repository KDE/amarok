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

#include <config.h>        //for HAVE_SQLITE check

#include "amarokconfig.h"
#include "amarokmenu.h"    //see toolbar construction
#include "browserwin.h"
#include "browserbar.h"
#include "collectionbrowser.h"
#include "enginecontroller.h" //for actions in ctor
#include "filebrowser.h"
#include "playerapp.h"
#include "playlistwidget.h"
#include "streambrowser.h"
#include "searchbrowser.h"
#include "statusbar.h"

#include <qcolor.h>        //setPalettes()
#include <qevent.h>        //eventFilter()
#include <qlayout.h>
#include <qobjectlist.h>   //setPaletteRecursively()
#include <qpalette.h>      //setPalettes()
#include <qtooltip.h>      //QToolTip::add()
#include <qvbox.h>         //contains the playlist

#include <kaction.h>       //m_actionCollection
#include <kapplication.h>  //kapp
#include <kdebug.h>
#include <kglobal.h>
#include <kfiledialog.h>   //savePlaylist()
#include <klineedit.h>     //m_lineEdit
#include <klocale.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>   //createGUI()
#include <kurlrequester.h>    //slotAddLocation()
#include <kurlrequesterdlg.h> //slotAddLocation()
#include <kxmlguifactory.h>   //XMLGUI
#include <kxmlguibuilder.h>   //XMLGUI



BrowserWin::BrowserWin( QWidget *parent, const char *name )
   : QWidget( parent, name, Qt::WType_TopLevel | Qt::WNoAutoErase )
   , KXMLGUIClient()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    setCaption( "amaroK" );

    pApp->m_pActionCollection = actionCollection(); //FIXME sucks

    //<actions>
        KActionCollection* const ac = actionCollection();
        const EngineController* const ec = EngineController::instance();

        KStdAction::configureToolbars( pApp, SLOT( slotConfigToolBars() ), ac );
        KStdAction::keyBindings( pApp, SLOT( slotConfigShortcuts() ), ac );
        KStdAction::keyBindings( pApp, SLOT( slotConfigGlobalShortcuts() ), ac, "options_configure_globals" );
        KStdAction::preferences( pApp, SLOT( slotShowOptions() ), ac );
        KStdAction::quit( pApp, SLOT( quit() ), ac );
        KStdAction::open( this, SLOT(slotAddLocation()), ac, "playlist_add" )->setText( i18n("&Add Media") );
        KStdAction::save( this, SLOT(savePlaylist()), ac, "playlist_save" )->setText( i18n("&Save Playlist") );

        ac->action( "options_configure_globals" )->setText( i18n( "Configure Global Shortcuts..." ) );

        new KAction( i18n( "Previous Track" ), "player_start", 0, ec, SLOT( previous() ), ac, "prev" );
        new KAction( i18n( "Play" ), "player_play", 0, ec, SLOT( play() ), ac, "play" );
        new KAction( i18n( "Stop" ), "player_stop", 0, ec, SLOT( stop() ), ac, "stop" );
        new KAction( i18n( "Pause" ), "player_pause", 0, ec, SLOT( pause() ), ac, "pause" );
        new KAction( i18n( "Next Track" ), "player_end", 0, ec, SLOT( next() ), ac, "next" );

        new amaroK::MenuAction( ac );
        new amaroK::PlayPauseAction( ac );
        new amaroK::AnalyzerAction( ac );
        new amaroK::RepeatTrackAction( ac );
        new amaroK::RepeatPlaylistAction( ac );
        new amaroK::RandomAction( ac );

        ac->action( "stop" )->setEnabled( false );
        ac->action( "pause" )->setEnabled( false );
        ac->action( "prev" )->setEnabled( false );
        ac->action( "next" )->setEnabled( false );
    //</actions>


    m_browsers = new BrowserBar( this );
    m_toolbar  = new KToolBar( this, "playlist_toolbar" );
    m_lineEdit = new KLineEdit( m_browsers->container() );
    m_playlist = new PlaylistWidget( m_browsers->container(), ac );

    QToolTip::add( m_lineEdit, i18n( "Enter filter string" ) );

    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_browsers, 10 );
    layV->addWidget( m_toolbar );
    amaroK::StatusBar *statusbar = new amaroK::StatusBar( this, "statusbar" );
    layV->addWidget( statusbar );


    m_playlist->installEventFilter( this ); //we intercept keyEvents
    m_lineEdit->installEventFilter( this ); //we intercept keyEvents


    //<XMLGUI>
        setXMLFile( "amarokui.rc" );
        createGUI(); //NOTE we implement this
    //</XMLGUI>


    //<FileBrowser>
        m_browsers->addPage( new KDevFileSelector( 0, "FileBrowser" ), i18n( "File Browser" ), "hdd_unmount" );
    //</FileBrowser>

    //<SearchBrowser>
        m_browsers->addPage( new SearchBrowser( 0, "SearchBrowser" ), i18n( "Search Browser" ), "find" );
    //</SearchBrowser>

    //<PlaylistBrowser>
        //m_browsers->addPage( m_playlist->browser(), i18n( "Playlist Browser" ), "midi" );
    //</PlaylistBrowser>

#ifdef HAVE_SQLITE
    //<CollectionBrowser>
        m_browsers->addPage( new CollectionBrowser( "CollectionBrowser" ), i18n( "Collection Browser" ), "contents" );
    //</CollectionBrowser>
#endif

    { //<StreamBrowser>
        QVBox   *vb = new QVBox( 0, "StreamBrowser" );
        QWidget *b  = new QPushButton( "&Fetch Stream Information", vb );
        QObject *sb = new StreamBrowser( vb );
        connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
        connect( b, SIGNAL( clicked() ),  b, SLOT( deleteLater() ) );
        m_browsers->addPage( vb, i18n( "Stream Browser" ), "network" );
    } //</StreamBrowser>


    connect( m_playlist, SIGNAL(itemCountChanged(int)),
             statusbar,    SLOT(slotItemCountChanged(int)) );
    connect( m_playlist, SIGNAL( aboutToClear() ),
             m_lineEdit,   SLOT( clear() ) );
    //FIXME you need to detect focus out from the sideBar and connect to that..
    connect( m_playlist, SIGNAL( clicked( QListViewItem * ) ),
             m_browsers,   SLOT( autoClosePages() ) );
    connect( m_lineEdit, SIGNAL( textChanged( const QString& ) ),
             m_playlist,   SLOT( slotTextChanged( const QString& ) ) );

    kdDebug() << "END " << k_funcinfo << endl;
}

///////// public interface

void BrowserWin::createGUI()
{
    setUpdatesEnabled( false );

    m_toolbar->clear(); //is necessary

    KXMLGUIBuilder builder( this );
    KXMLGUIFactory factory( &builder, this );

    //build Toolbar, plug actions
    factory.addClient( this );

    //TEXT ON RIGHT HACK
    //KToolBarButtons have independent settings for their appearance.
    //KToolBarButton::modeChange() causes that button to set its mode to that of its parent KToolBar
    //KToolBar::setIconText() calls modeChange() for children, unless 2nd param is false

    typedef QValueList<QCString> QCStringList;

    QCStringList list;
    list << "toolbutton_playlist_add"
//         << "toolbutton_playlist_clear"
//         << "toolbutton_playlist_shuffle"
         << "toolbutton_playlist_show"
         << "toolbutton_amarok_menu";

    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want some buttons to have text on right

    const QCStringList::ConstIterator last = list.fromLast();
    const QCStringList::ConstIterator end  = list.constEnd();

    for( QCStringList::ConstIterator it = list.constBegin(); it != end; )
    {
        KToolBarButton* const button = (KToolBarButton*)m_toolbar->child( *it );
        if( button ) button->modeChange();

        if( ++it == last ) m_toolbar->setIconText( KToolBar::TextOnly, false );
    }

    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance

    conserveMemory();

    setUpdatesEnabled( true );
}


void BrowserWin::insertMedia( const KURL::List &list, bool clearList, bool directPlay )
{
    if( clearList ) m_playlist->clear(); //FIXME clear currently is not 100% bug free, it might not work as expected

    m_playlist->insertMedia( list, directPlay );
}


void BrowserWin::restoreSessionPlaylist()
{
    insertMedia( m_playlist->defaultPlaylistPath() );
}


bool BrowserWin::isAnotherTrack() const
{
    return m_playlist->isTrackAfter();
}


void BrowserWin::setColors( const QPalette &pal, const QColor &bgAlt )
{
    //this updates all children's palettes recursively (thanks Qt!)
    m_browsers->setPalette( pal );

    const bool changeMenuBar = !AmarokConfig::schemeKDE();
    QObjectList *list = m_browsers->queryList( "QWidget" );

    for( QObject *obj = list->first(); obj; obj = list->next() )
    {
        if( changeMenuBar && obj->inherits("QMenuBar") )
        {
            static_cast<QWidget*>(obj)->setPalette( QApplication::palette() );
        }
        else if( obj->inherits("KListView") )
        {
            KListView *lv = dynamic_cast<KListView *>(obj); //slow, but safe
            if( lv ) lv->setAlternateBackground( bgAlt );
        }
    }

    //TODO perhaps this should be a global member of some singleton (I mean bgAlt not just the filebrowser bgAlt!)
    KDevFileSelector::altBgColor = bgAlt;
}


void BrowserWin::setFont( const QFont &newFont )
{
    m_browsers->setFont( newFont );
    m_playlist->setFont( newFont );
}


void BrowserWin::saveConfig()
{
    //FIXME sucks a little to get ptr this way
    //FIXME instead force the widgets to derive from SideBarWidget or something
    // this method is good as it saves having duplicate pointers to the fileBrowser
    KDevFileSelector *fileBrowser = (KDevFileSelector *)m_browsers->page( "FileBrowser" );
    fileBrowser->writeConfig();
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

            case Key_PageDown:
            case Key_PageUp:
                QApplication::sendEvent( m_playlist, e );
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
    QWidget *fb = m_browsers->page( "FileBrowser" );
    QString path = fb ? static_cast<KDevFileSelector *>(fb)->location() : "~";

    path = KFileDialog::getSaveFileName( path, "*.m3u" );

    if( !path.isEmpty() ) //FIXME unecessary check
    {
        m_playlist->saveM3U( path );
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
