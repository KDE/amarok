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

#include "actionclasses.h"    //see toolbar construction
#include "amarokconfig.h"
#include "app.h"
#include "browserbar.h"
#include "collectionbrowser.h"
#include "contextbrowser.h"
#include "enginecontroller.h" //for actions in ctor
#include "filebrowser.h"
#include "playlist.h"
#include "playlistwindow.h"
#include "searchbrowser.h"
#include "statusbar.h"
#include "streambrowser.h"

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
#include <kfiledialog.h>    //savePlaylist()
#include <khtml_part.h>     //Welcome Tab
#include <kiconloader.h>    //ClearFilter button
#include <klineedit.h>      //m_lineEdit
#include <klocale.h>
#include <kstandarddirs.h>    //Welcome Tab, locate welcome.html
#include <ktoolbar.h>
#include <ktoolbarbutton.h>   //createGUI()
#include <kurlrequester.h>    //slotAddLocation()
#include <kurlrequesterdlg.h> //slotAddLocation()
#include <kxmlguifactory.h>   //XMLGUI
#include <kxmlguibuilder.h>   //XMLGUI



PlaylistWindow::PlaylistWindow()
   : QWidget( 0, "PlaylistWindow", Qt::WNoAutoErase | Qt::WGroupLeader )
   , KXMLGUIClient()
{
    setCaption( "amaroK" );


    KActionCollection* const ac = actionCollection();
    const EngineController* const ec = EngineController::instance();

    KStdAction::configureToolbars( pApp, SLOT( slotConfigToolBars() ), ac );
    KStdAction::keyBindings( pApp, SLOT( slotConfigShortcuts() ), ac );
    KStdAction::keyBindings( pApp, SLOT( slotConfigGlobalShortcuts() ), ac, "options_configure_globals" );
    KStdAction::preferences( pApp, SLOT( slotConfigAmarok() ), ac );
    KStdAction::quit( pApp, SLOT( quit() ), ac );
    KStdAction::open( this, SLOT(slotAddLocation()), ac, "playlist_add" )->setText( i18n("&Add Media") );
    KStdAction::save( this, SLOT(savePlaylist()), ac, "playlist_save" )->setText( i18n("&Save Playlist") );

    ac->action( "options_configure_globals" )->setText( i18n( "Configure &Global Shortcuts..." ) );

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
    new amaroK::VolumeAction( ac );

    ac->readShortcutSettings( QString::null, kapp->config() );


    move( AmarokConfig::playlistWindowPos() );
    resize( AmarokConfig::playlistWindowSize() );
}

PlaylistWindow::~PlaylistWindow()
{
    AmarokConfig::setPlaylistWindowPos( pos() );  //TODO de XT?
    AmarokConfig::setPlaylistWindowSize( size() ); //TODO de XT?
}


///////// public interface

void
PlaylistWindow::init()
{
    //this function is necessary because we reference pApp->actionCollection() are members of
    //this function and some objects we create in this function use pApp->actionCollection()
    //but since pApp->m_pPlaylistWindow is not defined until the above ctor returns it causes a
    //crash unless we do the initialisation in 2 stages.

    kdDebug() << "BEGIN " << k_funcinfo << endl;


    m_browsers = new BrowserBar( this );


    { //<Search LineEdit>
        QHBox *hbox; QToolButton *button;

        hbox       = new QHBox( m_browsers->container() );
        button     = new QToolButton( hbox );
        m_lineEdit = new KLineEdit( hbox );

        hbox->setMargin( 4 );
        button->setIconSet( SmallIconSet( "locationbar_erase.png" ) );
        m_lineEdit->setFrame( QFrame::Sunken );
        m_lineEdit->installEventFilter( this ); //we intercept keyEvents

        connect( button, SIGNAL(clicked()), m_lineEdit, SLOT(clear()) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_lineEdit, i18n( "Enter space-separated terms to filter playlist" ) );
    } //</Search LineEdit>


    m_toolbar   = new amaroK::ToolBar( this, "playlist_toolbar" );
    m_statusbar = new amaroK::StatusBar( this );
    m_playlist  = new Playlist( m_browsers->container(), actionCollection() );


    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_browsers, 1 );
    layV->addWidget( m_toolbar );
    layV->addWidget( m_statusbar );

    //The volume slider later becomes our FocusProxy, so all wheelEvents get redirected to it
    m_toolbar->setFocusPolicy( QWidget::WheelFocus );
    m_statusbar->setShown( AmarokConfig::showStatusBar() );
    m_playlist->setMargin( 2 );
    m_playlist->installEventFilter( this ); //we intercept keyEvents

    //<XMLGUI>
        KConfig* const config = kapp->config();
        config->setGroup( "General" );
        setXMLFile( config->readEntry( "XMLFile", "amarokui.rc" ) );
        createGUI(); //NOTE we implement this
    //</XMLGUI>


    //<Browsers>
        m_browsers->addBrowser( new FileBrowser( "FileBrowser" ), i18n( "Files" ), "hdd_unmount" );
        m_browsers->addBrowser( new CollectionBrowser( "CollectionBrowser" ), i18n( "Collection" ), "contents" );
        m_browsers->addBrowser( new ContextBrowser( "ContextBrowser" ), i18n( "Context" ), "document" );
        m_browsers->addBrowser( new SearchBrowser( "SearchBrowser" ), i18n( "Search" ), "find" );

        #ifdef PLAYLIST_BROWSER
        m_browsers->addBrowser( m_playlist->browser(), i18n( "Playlist" ), "midi" );
        #endif

        { //<StreamBrowser>
            QVBox   *vb = new QVBox( 0, "StreamBrowser" );
            QWidget *b  = new KPushButton( KGuiItem(i18n("&Fetch Stream Information"), "2downarrow"), vb );
            QWidget *sb = new StreamBrowser( vb );
            vb->setSpacing( 3 );
            vb->setMargin( 5 );
            vb->setFocusProxy( sb );
            connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
            connect( b, SIGNAL( clicked() ),  b, SLOT( deleteLater() ) );
            m_browsers->addBrowser( vb, i18n( "Streams" ), "network" );
        } //</StreamBrowser>

        if( AmarokConfig::showWelcomeTab() )
        { //<WelcomePage>
            KHTMLPart *w = new KHTMLPart( (QWidget*)0 );
            KURL url; url.setPath( locate("data", "amarok/data/welcome.html") );
            w->widget()->setName( "WelcomePage" );
            w->openURL( url );
            m_browsers->addBrowser( w->widget(), i18n( "Welcome" ), "help" );

           connect( w->browserExtension(), SIGNAL(openURLRequest( const KURL&, const KParts::URLArgs&) ),
                    this,                    SLOT(welcomeURL( const KURL& )) );
        } //</WelcomePage>

        connect( m_browsers, SIGNAL(  activated( const KURL& )),
                 m_playlist,   SLOT(appendMedia( const KURL& )) );

    //</Browsers>


    connect( m_playlist, SIGNAL(itemCountChanged( int )),
             m_statusbar,  SLOT(slotItemCountChanged( int )) );
    connect( m_playlist, SIGNAL(aboutToClear()),
             m_lineEdit,   SLOT(clear()) );
    connect( m_lineEdit, SIGNAL(textChanged( const QString& )),
             m_playlist,   SLOT(slotTextChanged( const QString& )) );

    kdDebug() << "END " << k_funcinfo << endl;
}


void PlaylistWindow::createGUI()
{
    setUpdatesEnabled( false );

    m_toolbar->clear();

    //KActions don't unplug themselves when the widget that is plugged is deleted!
    //we need to unplug to detect if the menu is plugged in App::applySettings()
    //TODO report to bugs.kde.org
    //we unplug after clear as otherwise it crashes! dunno why..
    KActionPtrList actions = actionCollection()->actions();
    for( KActionPtrList::Iterator it = actions.begin(), end = actions.end(); it != end; ++it )
    {
        (*it)->unplug( m_toolbar );
    }

    KXMLGUIBuilder builder( this );
    KXMLGUIFactory factory( &builder, this );

    //build Toolbar, plug actions
    factory.addClient( this );

    //TEXT ON RIGHT HACK
    //KToolBarButtons have independent settings for their appearance.
    //KToolBarButton::modeChange() causes that button to set its mode to that of its parent KToolBar
    //KToolBar::setIconText() calls modeChange() for children, unless 2nd param is false

    QStringList list;
    list << "toolbutton_playlist_add"
//         << "toolbutton_playlist_clear"
//         << "toolbutton_playlist_shuffle"
         << "toolbutton_playlist_show"
         << "toolbutton_amarok_menu";

    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want some buttons to have text on right

    const QStringList::ConstIterator end  = list.constEnd();
    const QStringList::ConstIterator last = list.fromLast();
    for( QStringList::ConstIterator it = list.constBegin(); it != end; ++it )
    {
        KToolBarButton* const button = (KToolBarButton*)m_toolbar->child( (*it).latin1() );

        if( it == last )
        {
            m_toolbar->setIconText( KToolBar::TextOnly, false );

            //if the user has no PlayerWindow, he MUST have the menu action plugged
            //NOTE this is not saved to the local XMLFile, which is what the user will want
            if( !AmarokConfig::showPlayerWindow() && !button )
            {
                //due to ingenious code design, it will be plugged with
                //correct text formatting too! /me hugs me-self
                actionCollection()->action( "amarok_menu" )->plug( m_toolbar );
            }
        }

        if( button )
        {
            button->modeChange();
            button->setFocusPolicy( QWidget::NoFocus );
        }
    }

    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance

    conserveMemory();

    setUpdatesEnabled( true );
}


void PlaylistWindow::setColors( const QPalette &pal, const QColor &bgAlt )
{
    //TODO optimise bearing in mind ownPalette property and unsetPalette()
    //TODO this doesn't work well with the select your own colours options. SIGH. Is it worth the trouble?

    //this updates all children's palettes recursively (thanks Qt!)
    m_browsers->setPalette( pal );

    QObjectList* const list = m_browsers->queryList( "QWidget" );

    //now we need to search for KListViews so we can set the alternative colours
    //also amaroK's colour scheme has a few issues
    for( QObject *obj = list->first(); obj; obj = list->next() )
    {
        #define widget static_cast<QWidget*>(obj)

        if( obj->inherits("KListView") )
        {
            static_cast<KListView*>(obj)->setAlternateBackground( bgAlt );
        }
        else if( obj->inherits("QLabel") || obj->inherits("QToolBar") )
        {
            widget->setPaletteForegroundColor( Qt::white );
        }
        else if( obj->inherits("QMenuBar") || obj->parent()->isA("QSplitter") )
        {
            //I don't understand the QSplitter one, I got it to work by trial and error

            widget->setPalette( QApplication::palette() );
        }

        #undef widget
    }

    delete list; //heap allocated!

    //TODO perhaps this should be a global member of some singleton (I mean bgAlt not just the filebrowser bgAlt!)
    FileBrowser::altBgColor = bgAlt;
}


void PlaylistWindow::setFont( const QFont &font )
{
    m_browsers->setFont( font );
    m_playlist->setFont( font );
}


bool PlaylistWindow::eventFilter( QObject *o, QEvent *e )
{
    //here we filter some events for the Playlist Search LineEdit and the Playlist
    //this makes life easier since we have more useful functions available from this class

    switch( e->type() )
    {
    case QEvent::FocusIn:
        m_browsers->autoCloseBrowsers();
        break;

    case QEvent::KeyPress:

        //there are a few keypresses that we intercept

        #define e static_cast<QKeyEvent*>(e)

        if( e->key() == Key_F2 )
        {
            // currentItem is ALWAYS visible.
            QListViewItem *item = m_playlist->currentItem();

            // intercept F2 for inline tag renaming
            // NOTE: tab will move to the next tag
            // NOTE: if item is still null don't select first item in playlist, user wouldn't want that. It's silly.
            // TODO: berkus has solved the "inability to cancel" issue with KListView, but it's not in kdelibs yet..

            // item may still be null, but this is safe
            // NOTE: column 0 cannot be edited currently, hence we pick column 1
            m_playlist->rename( item, 1 ); //TODO what if this column is hidden?

            return TRUE;
        }

        if( o == m_lineEdit ) //the search lineedit
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

void PlaylistWindow::closeEvent( QCloseEvent *e )
{
    pApp->genericEventHandler( this, e );
}


void PlaylistWindow::savePlaylist() const //SLOT
{
    FileBrowser *fb = (FileBrowser*)m_browsers->browser( "FileBrowser" );
    QString path = fb ? fb->location() : "~";

    path = KFileDialog::getSaveFileName( path, "*.m3u" );

    if( !path.isEmpty() ) //FIXME unecessary check?
    {
        m_playlist->saveM3U( path );
    }
}


void PlaylistWindow::slotAddLocation() //SLOT
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( kapp->makeStdCaption( i18n( "Enter File, URL or Directory" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    playlist()->insertMedia( dlg.selectedURL() );
}


#include <kwin.h>
void PlaylistWindow::showHide() //SLOT
{
    //show/hide playlist global shortcut and PlayerWindow PlaylistButton connect to this slot

    //RULES:
    //1. hidden & iconified -> deiconify & show
    //2. hidden & deiconified -> show
    //3. shown & iconified -> deiconify
    //4. shown & deiconified -> hide
    //5. don't hide if there is no tray icon or playerWindow //TODO (I can't be arsed)

    //NOTE isMinimized() can only be true if the window isShown()
    //NOTE this has taken me hours to get right, change at your peril!
    //     there are more contingencies than you can believe

    const KWin::WindowInfo info = KWin::windowInfo( winId() );
    const uint desktop = KWin::currentDesktop();
    const bool isOnThisDesktop = info.isOnDesktop( desktop );
    const bool isShaded = info.hasState( NET::Shaded );


    if( isShaded )
    {
        KWin::clearState( winId(), NET::Shaded );
        setShown( true );
    }

    if( !isOnThisDesktop )
    {
        KWin::setOnDesktop( winId(), desktop );
        setShown( true );
    }
    else if( !info.isMinimized() && !isShaded ) setShown( !isShown() );

    if( isShown() ) KWin::deIconifyWindow( winId() );
}


void PlaylistWindow::welcomeURL( const KURL &url )
{
    if( url == QString("amarok://remove_tab") )
    {
        m_browsers->removeBrowser( "WelcomePage" );
        AmarokConfig::setShowWelcomeTab( false );
        return; //to avoid code below
    }

    bool b;
    QString xml;

    if( url == QString( "amarok://default_mode" ) )
    {
        //this is the same as amarokui.rc, but we name it separately to ensure it
        //loads the original and not the user-modified scheme
        xml = "amarokui.rc";
        b = false;
    }
    else if( url == QString("amarok://amaamp_mode") )
    {
        xml = "amarokui_xmms.rc";
        b = true;
    }
    else return;

    setXMLFile( xml );
    createGUI();
    AmarokConfig::setShowPlayerWindow( b );
    AmarokConfig::setShowStatusBar( !b );
    pApp->applySettings();

    KConfig* const config = kapp->config();
    config->setGroup( "General" );
    config->writeEntry( "XMLFile", xml );
}

#include "playlistwindow.moc"
