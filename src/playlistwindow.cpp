/***************************************************************************
  begin                : Fre Nov 15 2002
  copyright            : (C) Mark Kretschmann <markey@web.de>
                       : (C) Max Howell <max.howell@methylblue.com>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "config.h"           //HAVE_XMMS definition

#include "actionclasses.h"    //see toolbar construction
#include "amarok.h"
#include "amarokconfig.h"
#include "browserbar.h"
#include "clicklineedit.h"    //m_lineEdit
#include "collectionbrowser.h"
#include "contextbrowser.h"
#include "enginecontroller.h" //for actions in ctor
#include "filebrowser.h"
#include "k3bexporter.h"
#include "mediabrowser.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistwindow.h"
#include "scriptmanager.h"
#include "searchbrowser.h"
#include "statusbar.h"

#include <qcolor.h>           //setPalettes()
#include <qevent.h>           //eventFilter()
#include <qlayout.h>
#include <qobjectlist.h>      //setPaletteRecursively()
#include <qpalette.h>         //setPalettes()
#include <qtimer.h>           //search filter timer
#include <qtooltip.h>         //QToolTip::add()
#include <qvbox.h>            //contains the playlist

#include <kaction.h>          //m_actionCollection
#include <kapplication.h>     //kapp
#include <kdebug.h>
#include <kfiledialog.h>      //savePlaylist()
#include <kglobal.h>
#include <khtml_part.h>       //Welcome Tab
#include <kiconloader.h>      //ClearFilter button
#include <klocale.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>    //Welcome Tab, locate welcome.html
#include <ktoolbar.h>
#include <ktoolbarbutton.h>   //createGUI()
#include <kurlrequester.h>    //slotAddLocation()
#include <kurlrequesterdlg.h> //slotAddLocation()
#include <kxmlguibuilder.h>   //XMLGUI
#include <kxmlguifactory.h>   //XMLGUI

#include <fixx11h.h>


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS amaroK::ToolBar
//////////////////////////////////////////////////////////////////////////////////////////

namespace amaroK
{
    class ToolBar : public KToolBar
    {
    public:
        ToolBar( QWidget *parent, const char *name )
            : KToolBar( parent, name )
        {}

    protected:
        virtual void
        mousePressEvent( QMouseEvent *e ) {
            if( e->button() == RightButton ) amaroK::Menu::instance()->popup( e->globalPos() );
        }

        virtual void
        wheelEvent( QWheelEvent *e ) {
            EngineController::instance()->increaseVolume( e->delta() / 18 );
        }
    };
}



//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS PlaylistWindow
//////////////////////////////////////////////////////////////////////////////////////////

PlaylistWindow *PlaylistWindow::s_instance = 0;

#include <time.h>
template <class B> void
PlaylistWindow::addBrowser( const char *name, const QString &title, const QString &icon )
{
    kdDebug() << "  Init: " << name << endl;

    clock_t start  = clock();
        m_browsers->addBrowser( new B( name ), title, icon );
    clock_t finish = clock();

    const double duration = (double) (finish - start) / CLOCKS_PER_SEC;

    kdDebug() << "    Time: " << duration << "s\n";
}


PlaylistWindow::PlaylistWindow()
   : QWidget( 0, "PlaylistWindow", Qt::WGroupLeader )
   , KXMLGUIClient()
   , m_lastBrowser( 0 )
{
    s_instance = this;

    // Sets caption and icon correctly (needed e.g. for GNOME)
    kapp->setTopWidget( this );

    KActionCollection* const ac = actionCollection();
    const EngineController* const ec = EngineController::instance();
    EngineController::instance()->attach( this );
    ( void ) new K3bExporter();

    KStdAction::configureToolbars( kapp, SLOT( slotConfigToolBars() ), ac );
    KStdAction::keyBindings( kapp, SLOT( slotConfigShortcuts() ), ac );
    KStdAction::keyBindings( kapp, SLOT( slotConfigGlobalShortcuts() ), ac, "options_configure_globals" );
    KStdAction::preferences( kapp, SLOT( slotConfigAmarok() ), ac );
    KStdAction::quit( kapp, SLOT( quit() ), ac );
    KStdAction::open( this, SLOT(slotAddLocation()), ac, "playlist_add" )->setText( i18n("&Add Media...") );
    KStdAction::save( this, SLOT(savePlaylist()), ac, "playlist_save" )->setText( i18n("&Save Playlist As...") );
    KStdAction::showMenubar( this, SLOT(slotToggleMenu()), ac );

    new KAction( i18n("Play Media..."), "fileopen", 0, this, SLOT(slotPlayMedia()), ac, "playlist_playmedia" );
    new KAction( i18n("Play Audio CD"), "cdaudio_unmount", 0, this, SLOT(playAudioCD()), ac, "play_audiocd" );
    new KAction( i18n("Scripts..."), "pencil", 0, this, SLOT(showScriptSelector()), ac, "script_manager" );

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
    new amaroK::AppendSuggestionsAction( ac );
    new amaroK::VolumeAction( ac );

    if ( K3bExporter::isAvailable() )
        new amaroK::BurnMenuAction( ac );

    ac->readShortcutSettings( QString::null, kapp->config() );


    //if first ever run let KWin place us
    if ( AmarokConfig::playlistWindowPos() != QPoint(-1,-1) )
        move( AmarokConfig::playlistWindowPos() );

    // On first ever run, use sizeHint
    // TODO sizeHint is stupid currently
    //if ( AmarokConfig::playlistWindowSize() != QSize(-1,-1) )
        resize( AmarokConfig::playlistWindowSize() );
}

PlaylistWindow::~PlaylistWindow()
{
    amaroK::config( "PlaylistWindow" )->writeEntry( "showMenuBar", m_menubar->isShown() );

    AmarokConfig::setPlaylistWindowPos( pos() );  //TODO de XT?
    AmarokConfig::setPlaylistWindowSize( size() ); //TODO de XT?
}


///////// public interface

void
PlaylistWindow::init()
{
    //this function is necessary because amaroK::actionCollection() returns our actionCollection
    //via the App::m_pPlaylistWindow pointer since App::m_pPlaylistWindow is not defined until
    //the above ctor returns it causes a crash unless we do the initialisation in 2 stages.

    m_browsers = new BrowserBar( this );

    { //<Search LineEdit>
        KToolBar *bar = new KToolBar( m_browsers->container() );
        bar->setIconSize( 22, false ); //looks more sensible
        QWidget *button = new KToolBarButton( "locationbar_erase", 1, bar );
        m_lineEdit = new ClickLineEdit( bar, i18n( "Filter here..." ) );

        bar->setStretchableWidget( m_lineEdit );

        m_lineEdit->setFrame( QFrame::Sunken );
        m_lineEdit->installEventFilter( this ); //we intercept keyEvents

        connect( button, SIGNAL(clicked()), m_lineEdit, SLOT(clear()) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_lineEdit, i18n( "Enter space-separated terms to filter playlist" ) );
    } //</Search LineEdit>

    m_playlist  = new Playlist( m_browsers->container(), actionCollection() );
    m_toolbar   = new amaroK::ToolBar( m_browsers->container(), "playlist_toolbar" );
    QWidget *m_statusbar = new amaroK::StatusBar( this );

    connect( m_lineEdit, SIGNAL(textChanged( const QString& )), m_playlist, SLOT(setFilter( const QString& )) );

    m_menubar = new KMenuBar( this );
    m_menubar->setShown( AmarokConfig::showMenuBar() );

    //BEGIN Actions menu
    KPopupMenu *actionsMenu = new KPopupMenu( m_menubar );
    actionCollection()->action("playlist_playmedia")->plug( actionsMenu );
    actionCollection()->action("play_audiocd")->plug( actionsMenu );
    actionsMenu->insertSeparator();
    actionCollection()->action("prev")->plug( actionsMenu );
    actionCollection()->action("play_pause")->plug( actionsMenu );
    actionCollection()->action("stop")->plug( actionsMenu );
    actionCollection()->action("next")->plug( actionsMenu );
    actionsMenu->insertSeparator();
    actionCollection()->action(KStdAction::name(KStdAction::Quit))->plug( actionsMenu );
    //END Play menu

    //BEGIN Playlist menu
    KPopupMenu *playlistMenu = new KPopupMenu( m_menubar );
    actionCollection()->action("playlist_add")->plug( playlistMenu );
    actionCollection()->action("playlist_save")->plug( playlistMenu );
    playlistMenu->insertSeparator();
    actionCollection()->action("playlist_undo")->plug( playlistMenu );
    actionCollection()->action("playlist_redo")->plug( playlistMenu );
    playlistMenu->insertSeparator();
    actionCollection()->action("playlist_clear")->plug( playlistMenu );
    actionCollection()->action("playlist_shuffle")->plug( playlistMenu );
    actionCollection()->action("playlist_show")->plug( playlistMenu );
    //this one has no real context with regard to the menu
    //actionCollection()->action("playlist_copy")->plug( playlistMenu );
    playlistMenu->insertSeparator();
    actionCollection()->action("playlist_remove_duplicates")->plug( playlistMenu );
    actionCollection()->action("playlist_select_all")->plug( playlistMenu );
    //END Playlist menu

    //BEGIN Tools menu
    m_toolsMenu = new KPopupMenu( m_menubar );
    m_toolsMenu->insertItem( QPixmap( locate( "data", "amarok/images/covermanager.png" ) ), i18n("&Cover Manager..."), amaroK::Menu::ID_SHOW_COVER_MANAGER );
    m_toolsMenu->insertItem( i18n("&First-Run Wizard..."), amaroK::Menu::ID_SHOW_WIZARD );
    m_toolsMenu->insertItem( i18n("&Visualizations..."), amaroK::Menu::ID_SHOW_VIS_SELECTOR );
    m_toolsMenu->insertItem( i18n("&Equalizer..."), kapp, SLOT( slotConfigEqualizer() ), 0, amaroK::Menu::ID_CONFIGURE_EQUALIZER );
    actionCollection()->action("script_manager")->plug( m_toolsMenu );
    m_toolsMenu->insertSeparator();
    m_toolsMenu->insertItem( i18n("&Rescan Collection"), ID_RESCAN_COLLECTION );

    m_toolsMenu->setItemEnabled( amaroK::Menu::ID_SHOW_VIS_SELECTOR, false );
    #ifdef HAVE_XMMS
    m_toolsMenu->setItemEnabled( amaroK::Menu::ID_SHOW_VIS_SELECTOR, true );
    #endif
    #ifdef HAVE_LIBVISUAL
    m_toolsMenu->setItemEnabled( amaroK::Menu::ID_SHOW_VIS_SELECTOR, true );
    #endif

    connect( m_toolsMenu, SIGNAL( aboutToShow() ), SLOT( toolsMenuAboutToShow() ) );
    connect( m_toolsMenu, SIGNAL( activated(int) ), SLOT( slotMenuActivated(int) ) );
    //END

    //BEGIN Settings menu
    m_settingsMenu = new KPopupMenu( m_menubar );
    //TODO use KStdAction or KMainWindow
    static_cast<KToggleAction *>(actionCollection()->action(KStdAction::name(KStdAction::ShowMenubar)))->setChecked( AmarokConfig::showMenuBar() );
    actionCollection()->action(KStdAction::name(KStdAction::ShowMenubar))->plug( m_settingsMenu );
    m_settingsMenu->insertItem( i18n( "Hide Toolbar" ), ID_SHOW_TOOLBAR );
    m_settingsMenu->insertItem( AmarokConfig::showPlayerWindow() ? i18n("Hide Player &Window") : i18n("Show Player &Window"), ID_SHOW_PLAYERWINDOW );
    m_settingsMenu->insertSeparator();
    //this should be only a context menu option and use next-queue graphics with an infinity symbol or something
    actionCollection()->action("repeat_track")->plug( m_settingsMenu );
    actionCollection()->action("repeat_playlist")->plug( m_settingsMenu );
    actionCollection()->action("random_mode")->plug( m_settingsMenu );
    actionCollection()->action("append_suggestions")->plug( m_settingsMenu );
    m_settingsMenu->insertSeparator();
    m_settingsMenu->insertItem( i18n( "Configure &Effects..." ), kapp, SLOT( slotConfigEffects() ), 0, amaroK::Menu::ID_SHOW_EFFECTS );
    actionCollection()->action("options_configure_globals")->plug( m_settingsMenu );
    actionCollection()->action(KStdAction::name(KStdAction::KeyBindings))->plug( m_settingsMenu );
    actionCollection()->action(KStdAction::name(KStdAction::ConfigureToolbars))->plug( m_settingsMenu );
    actionCollection()->action(KStdAction::name(KStdAction::Preferences))->plug( m_settingsMenu );

    m_settingsMenu->setItemEnabled( amaroK::Menu::ID_SHOW_EFFECTS, EngineController::engine()->hasEffects() );

    connect( m_settingsMenu, SIGNAL( activated(int) ), SLOT( slotMenuActivated(int) ) );
    //END Settings menu

    m_menubar->insertItem( i18n( "&Actions" ), actionsMenu );
    m_menubar->insertItem( i18n( "&Playlist" ), playlistMenu );
    m_menubar->insertItem( i18n( "&Tools" ), m_toolsMenu );
    m_menubar->insertItem( i18n( "&Settings" ), m_settingsMenu );
    m_menubar->insertItem( i18n( "&Help" ), amaroK::Menu::helpMenu() );


    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_menubar );
    layV->addWidget( m_browsers, 1 );
    layV->addWidget( m_toolbar );
    layV->addWidget( m_statusbar );
    layV->setSpacing( 2 );

    //The volume slider later becomes our FocusProxy, so all wheelEvents get redirected to it
    m_toolbar->setFocusPolicy( QWidget::WheelFocus );
    m_playlist->setMargin( 2 );
    m_playlist->installEventFilter( this ); //we intercept keyEvents


    //<XMLGUI>
        setXMLFile( amaroK::config()->readEntry( "XMLFile", "amarokui.rc" ) );
        createGUI(); //NOTE we implement this
    //</XMLGUI>


    //<Browsers>
        kdDebug() << "[browserBar] Initialisation statistics:\n";
        addBrowser<ContextBrowser>( "ContextBrowser", i18n( "Context" ), "info" );
        addBrowser<CollectionBrowser>( "CollectionBrowser", i18n( "Collection" ), "kfm" );
        addBrowser<PlaylistBrowser>( "PlaylistBrowser", i18n( "Playlists" ), "player_playlist_2" );
        addBrowser<SearchBrowser>( "SearchBrowser", i18n( "Search" ), "find" );
        if ( MediaBrowser::isAvailable() )
            addBrowser<MediaBrowser>( "MediaBrowser", i18n( "Media Device" ), "usbpendrive_unmount" );
        addBrowser<FileBrowser>( "FileBrowser", i18n( "Files" ), "hdd_unmount" );
    //</Browsers>


    connect( m_playlist, SIGNAL( itemCountChanged( int, int, int, int ) ), m_statusbar, SLOT( slotItemCountChanged( int, int, int, int ) ) );
    connect( m_playlist, SIGNAL( aboutToClear() ), m_lineEdit, SLOT( clear() ) );
}


void PlaylistWindow::recreateGUI()
{
    //mainly just used by amaroK::Menu
    setXMLFile( amaroK::config()->readEntry( "XMLFile", "amarokui.rc" ) );
    reloadXML();
    createGUI();
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
         << "toolbutton_burn_menu"
         << "toolbutton_amarok_menu";

    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want some buttons to have text on right

    const QStringList::ConstIterator end  = list.constEnd();
    const QStringList::ConstIterator last = list.fromLast();
    for( QStringList::ConstIterator it = list.constBegin(); it != end; ++it )
    {
        KToolBarButton* const button = (KToolBarButton*)m_toolbar->child( (*it).latin1() );

        if ( it == last ) {
            //if the user has no PlayerWindow, he MUST have the menu action plugged
            //NOTE this is not saved to the local XMLFile, which is what the user will want
            if ( !AmarokConfig::showPlayerWindow() && !AmarokConfig::showMenuBar() && !button )
                actionCollection()->action( "amarok_menu" )->plug( m_toolbar );
        }

        if ( button ) {
            button->modeChange();
            button->setFocusPolicy( QWidget::NoFocus );
        }
    }

    if ( AmarokConfig::showMenuBar() ) {
        if( actionCollection()->action( "amarok_menu" )->isPlugged() )
            actionCollection()->action( "amarok_menu" )->unplugAll();
    }

    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance
    conserveMemory();
    setUpdatesEnabled( true );
}


void PlaylistWindow::setColors( const QPalette &pal, const QColor &bgAlt )
{
    setPalette( pal ); //this updates all children's palettes recursively (thanks Qt!)

    QObjectList* const list = queryList( "KListView" );
    for( QObject *o = list->first(); o; o = list->next() )
        static_cast<KListView*>(o)->setAlternateBackground( bgAlt );
    delete list; //heap allocated!

    //TODO perhaps this should be a global member of some singleton (I mean bgAlt not just the filebrowser bgAlt!)
    FileBrowser::altBgColor = bgAlt;
}


void PlaylistWindow::setFont( const QFont &font, const QFont &contextfont )
{
    //m_browsers->setFont( font );
    m_playlist->setFont( font );
    m_browsers->browser( "ContextBrowser" )->setFont( contextfont ); //virtual so works without cast
}


bool PlaylistWindow::eventFilter( QObject *o, QEvent *e )
{
    //here we filter some events for the Playlist Search LineEdit and the Playlist
    //this makes life easier since we have more useful functions available from this class

    switch( e->type() )
    {
    case 8/*QEvent::FocusIn*/:
        m_browsers->autoCloseBrowsers();
        break;

    case 6/*QEvent::KeyPress*/:

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
            typedef QListViewItemIterator It;

            // If ctrl key is pressed, propagate keyEvent to the playlist
            if ( e->key() & Qt::CTRL ) {
                QApplication::sendEvent( m_playlist, e );
                return TRUE;
            }

            switch( e->key() )
            {
            case Key_Down:
                if( QListViewItem *item = *It( m_playlist, It::Visible ) )
                {
                    m_playlist->setFocus();
                    m_playlist->setCurrentItem( item );
                    item->setSelected( true );
                    return TRUE;
                }
                return FALSE;

            case Key_PageDown:
            case Key_PageUp:
                QApplication::sendEvent( m_playlist, e );
                return TRUE;

            case Key_Return:
            case Key_Enter:
                m_playlist->activate( *It( m_playlist, It::Visible ) );
                m_playlist->showCurrentTrack();
                m_lineEdit->clear();
                return TRUE;

            case Key_Escape:
                m_lineEdit->clear();
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

    return QWidget::eventFilter( o, e );
}

void PlaylistWindow::closeEvent( QCloseEvent *e )
{
    amaroK::genericEventHandler( this, e );
}


void PlaylistWindow::engineStateChanged( Engine::State state )
{
    if ( !AmarokConfig::autoShowContextBrowser() ) return;

    const int context = 0;

    switch ( state )
    {
        case Engine::Playing:
            m_lastBrowser = m_browsers->currentIndex();
            m_browsers->showBrowser( context );
            break;

        case Engine::Empty:
            m_browsers->showBrowser( m_lastBrowser );
            break;

        case Engine::Idle:
            break;

        case Engine::Paused:
            break;
    }
}


void PlaylistWindow::savePlaylist() const //SLOT
{
    FileBrowser *fb = (FileBrowser*)m_browsers->browser( "FileBrowser" );
    QString path = fb ? fb->location() : "~";

    path = KFileDialog::getSaveFileName( path, "*.m3u" );

    if( !path.isEmpty() ) //FIXME unecessary check?
    {
        m_playlist->saveM3U( path );
        //add the saved playlist to playlist browser
        PlaylistBrowser *pb = (PlaylistBrowser*)m_browsers->browser( "PlaylistBrowser" );
        pb->addPlaylist( path, true );
    }
}


void PlaylistWindow::slotPlayMedia() //SLOT
{
    // Request location and immediately start playback
    slotAddLocation( true );
}


void PlaylistWindow::slotAddLocation( bool directPlay ) //SLOT
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    kapp->setTopWidget( &dlg );
    dlg.setCaption( kapp->makeStdCaption( i18n( "Enter File, URL or Directory" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    if ( !dlg.selectedURL().isEmpty() ) {
        int options = Playlist::Append;
        if ( directPlay ) options |= Playlist::DirectPlay;
        m_playlist->insertMedia( dlg.selectedURL(), options );
    }
}


void PlaylistWindow::playAudioCD() //SLOT
{
    m_browsers->showBrowser( "FileBrowser" );
    FileBrowser *fb = static_cast<FileBrowser *>( m_browsers->browser("FileBrowser") );
    fb->setDir( KURL("audiocd:/") );
}


void PlaylistWindow::showScriptSelector() //SLOT
{
    ScriptManager::instance()->show();
}


void PlaylistWindow::slotToggleMenu() //SLOT
{
    if( static_cast<KToggleAction *>(actionCollection()->action(KStdAction::name(KStdAction::ShowMenubar)))->isChecked() ) {
        AmarokConfig::setShowMenuBar( true );
        m_menubar->setShown( true );

        if( amaroK::actionCollection()->action( "amarok_menu" )->isPlugged() )
            amaroK::actionCollection()->action( "amarok_menu" )->unplugAll();
    }
    else
    {
        AmarokConfig::setShowMenuBar( false );
        m_menubar->setShown( false );
        recreateGUI();
    }
}

void PlaylistWindow::slotMenuActivated( int index ) //SLOT
{
    switch( index )
    {
    default:
        //saves duplicating the code and header requirements
        amaroK::Menu::instance()->slotActivated( index );
        break;
    case ID_SHOW_TOOLBAR:
        m_toolbar->setShown( !m_toolbar->isShown() );
        actionCollection()->action(KStdAction::name(KStdAction::ShowMenubar))->setEnabled( m_toolbar->isShown() );
        m_settingsMenu->changeItem( index, m_toolbar->isShown() ? i18n("Hide Toolbar") : i18n("Show Toolbar") );
        break;
    case ID_SHOW_PLAYERWINDOW:
        AmarokConfig::setShowPlayerWindow( !AmarokConfig::showPlayerWindow() );
        m_settingsMenu->changeItem( index, AmarokConfig::showPlayerWindow() ? i18n("Hide Player &Window") : i18n("Show Player &Window") );
        QTimer::singleShot( 0, kapp, SLOT( applySettings() ) );
        break;
    case ID_RESCAN_COLLECTION:
        CollectionDB::instance()->startScan();
        break;
    }
}


void PlaylistWindow::toolsMenuAboutToShow()
{
    m_toolsMenu->setItemEnabled( amaroK::Menu::ID_CONFIGURE_EQUALIZER, EngineController::hasEngineProperty( "HasEqualizer" ) );
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
    #if KDE_IS_VERSION(3,2,1)
    const bool isShaded = info.hasState( NET::Shaded );
    #else
    const bool isShaded = false;
    #endif


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

#include "playlistwindow.moc"
