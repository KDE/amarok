/***************************************************************************
                      app.cpp  -  description
                         -------------------
begin                : Mit Okt 23 14:35:18 CEST 2002
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

#include "amarok.h"
#include "amarokconfig.h"
#include "amarokdcophandler.h"
#include "app.h"
#include "config.h"
#include "configdialog.h"
#include "collectionbrowser.h"
#include "debug.h"
#include "effectwidget.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizersetup.h"
#include "firstrunwizard.h"
#include "metabundle.h"
#include "osd.h"
#include "playerwindow.h"
#include "playlist.h"
#include "playlistwindow.h"
#include "pluginmanager.h"
#include "scriptmanager.h"
#include "scrobbler.h"
#include "statusbar.h"
#include "systray.h"
#include "tracktooltip.h"        //engineNewMetaData()

#include <iostream>

#include <kcmdlineargs.h>        //initCliArgs()
#include <kcursor.h>             //amaroK::OverrideCursor
#include <kedittoolbar.h>        //slotConfigToolbars()
#include <kglobalaccel.h>        //initGlobalShortcuts()
#include <kglobalsettings.h>     //applyColorScheme()
#include <kkeydialog.h>          //slotConfigShortcuts()
#include <klocale.h>
#include <kmessagebox.h>         //applySettings(), genericEventHandler()
#include <kstandarddirs.h>
#include <kurldrag.h>            //genericEventHandler()

#include <qevent.h>              //genericEventHandler()
#include <qeventloop.h>          //applySettings()
#include <qfile.h>
#include <qobjectlist.h>         //applyColorScheme()
#include <qpalette.h>            //applyColorScheme()
#include <qpixmap.h>             //QPixmap::setDefaultOptimization()
#include <qpopupmenu.h>          //genericEventHandler
#include <qtooltip.h>            //default tooltip for trayicon


App::App()
        : KApplication()
        , m_pPlayerWindow( 0 ) //will be created in applySettings()
{
    DEBUG_BLOCK

    const KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();
    bool restoreSession = args->count() == 0 || args->isSet( "append" ) || args->isSet( "enqueue" );

    QPixmap::setDefaultOptimization( QPixmap::MemoryOptim );

    //needs to be created before the wizard
    m_pDcopPlayerHandler   = new amaroK::DcopPlayerHandler(); // Must be created first
    m_pDcopPlaylistHandler = new amaroK::DcopPlaylistHandler();

    // Remember old folder setup, so we can detect changes after the wizard was used
    const QStringList oldCollectionFolders = AmarokConfig::collectionFolders();

    if ( amaroK::config()->readBoolEntry( "First Run", true ) || args->isSet( "wizard" ) ) {
        std::cout << "STARTUP\n" << std::flush; //hide the splashscreen
        firstRunWizard();
        amaroK::config()->writeEntry( "First Run", false );
        amaroK::config()->sync();
    }

    m_pGlobalAccel    = new KGlobalAccel( this );
    m_pPlaylistWindow = new PlaylistWindow();
    m_pTray           = new amaroK::TrayIcon( m_pPlaylistWindow );

    m_pPlaylistWindow->init(); //creates the playlist, browsers, etc.
    initGlobalShortcuts();

    //load previous playlist in separate thread
    if ( restoreSession && AmarokConfig::savePlaylist() )
        Playlist::instance()->restoreSession();

    if( args->isSet( "engine" ) )
      AmarokConfig::setSoundSystem( args->getOption( "engine" ) );

    //create engine, show PlayerWindow, show TrayIcon etc.
    applySettings( true );

    // Start ScriptManager. Must be created _after_ PlaylistWindow.
    if ( amaroK::config( "ScriptManager" )->readBoolEntry( "Auto Run" ) )
        ScriptManager::instance();

    //notify loader application that we have started
    std::cout << "STARTUP\n" << std::flush;

    //after this point only analyzer and temporary pixmaps will be created
    QPixmap::setDefaultOptimization( QPixmap::BestOptim );

    //do after applySettings(), or the OSD will flicker and other wierdness!
    //do before restoreSession()!
    EngineController::instance()->attach( this );

    //set a default interface
    engineStateChanged( Engine::Empty );

    if ( AmarokConfig::resumePlayback() && restoreSession && !args->isSet( "stop" ) ) {
        //restore session as long as the user didn't specify media to play etc.
        //do this after applySettings() so OSD displays correctly
        EngineController::instance()->restoreSession();
    }

    // Remove amazon cover images older than 90 days, to comply with licensing terms
    #ifdef AMAZON_SUPPORT
    pruneCoverImages();
    #endif

    // Trigger collection scan if folder setup was changed by wizard
    if ( oldCollectionFolders != AmarokConfig::collectionFolders() )
        CollectionDB::instance()->startScan();
    // If database version is updated, the collection needs to be rescanned.
    // Works also if the collection is empty for some other reason
    // (e.g. deleted collection.db)
    else if ( CollectionDB::instance()->isEmpty() )
    {
        CollectionDB::instance()->startScan();
    }

    handleCliArgs();
}

App::~App()
{
    DEBUG_BLOCK

    // Hiding the OSD before exit prevents crash
    amaroK::OSD::instance()->hide();

    EngineBase* const engine = EngineController::engine();

    if ( AmarokConfig::resumePlayback() ) {
        if ( engine->state() != Engine::Empty ) {
            AmarokConfig::setResumeTrack( EngineController::instance()->playingURL().prettyURL() );
            AmarokConfig::setResumeTime( engine->position() );
        }
        else AmarokConfig::setResumeTrack( QString::null ); //otherwise it'll play previous resume next time!
    }

    EngineController::instance()->endSession(); //records final statistics
    EngineController::instance()->detach( this );

    // do even if trayicon is not shown, it is safe
    amaroK::config()->writeEntry( "HiddenOnExit", mainWindow()->isHidden() );

    delete m_pPlayerWindow;   //sets some XT keys
    delete m_pPlaylistWindow; //sets some XT keys

    // this must be deleted before the connection to the Xserver is
    // severed, or we risk a crash when the QApplication is exited,
    // I asked Trolltech! *smug*
    delete amaroK::OSD::instance();

    AmarokConfig::setVersion( APP_VERSION );
    AmarokConfig::writeConfig();

    //need to unload the engine before the kapplication is destroyed
    PluginManager::unload( engine );
}


void App::handleCliArgs() //static
{
    KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    if ( args->count() > 0 )
    {
        KURL::List list;
        for( int i = 0; i < args->count(); i++ )
            list << args->url( i );

        int options;
        if( args->isSet( "queue" ) )
           options = Playlist::Queue;
        else if( args->isSet( "append" ) || args->isSet( "enqueue" ) ) {
           options = Playlist::Append;
           if( args->isSet( "play" ) )
              options |= Playlist::DirectPlay;
        }
        else
           options = Playlist::Replace | Playlist::DirectPlay;

        Playlist::instance()->insertMedia( list, options );
    }

    //we shouldn't let the user specify two of these since it is pointless!
    //so we prioritise, pause > stop > play > next > prev
    //thus pause is the least destructive, followed by stop as brakes are the most important bit of a car(!)
    //then the others seemed sensible. Feel free to modify this order, but please leave justification in the cvs log
    //I considered doing some sanity checks (eg only stop if paused or playing), but decided it wasn't worth it
    else if ( args->isSet( "pause" ) )
        EngineController::instance()->pause();
    else if ( args->isSet( "stop" ) )
        EngineController::instance()->stop();
    else if ( args->isSet( "play-pause" ) )
        EngineController::instance()->playPause();
    else if ( args->isSet( "play" ) ) //will restart if we are playing
        EngineController::instance()->play();
    else if ( args->isSet( "next" ) )
        EngineController::instance()->next();
    else if ( args->isSet( "previous" ) )
        EngineController::instance()->previous();

    if ( args->isSet( "toggle-playlist-window" ) )
        pApp->m_pPlaylistWindow->showHide();

    args->clear();    //free up memory
}


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void App::initCliArgs( int argc, char *argv[] ) //static
{
    extern class KAboutData aboutData; //defined in amarokcore/main.cpp

    static KCmdLineOptions options[] =
        {
            { "+[URL(s)]", I18N_NOOP( "Files/URLs to Open" ), 0 },
            { "r", 0, 0 },
            { "previous", I18N_NOOP( "Skip backwards in playlist" ), 0 },
            { "p", 0, 0 },
            { "play", I18N_NOOP( "Start playing current playlist" ), 0 },
            { "t", 0, 0 },
            { "play-pause", I18N_NOOP( "Play if stopped, pause if playing" ), 0 },
            { "pause", I18N_NOOP( "Pause playback" ), 0 },
            { "s", 0, 0 },
            { "stop", I18N_NOOP( "Stop playback" ), 0 },
            { "f", 0, 0 },
            { "next", I18N_NOOP( "Skip forwards in playlist" ), 0 },
            { ":", I18N_NOOP("Additional options:"), 0 },
            { "a", 0, 0 },
            { "append", I18N_NOOP( "Append files/URLs to playlist" ), 0 },
            { "e", 0, 0 },
            { "enqueue", I18N_NOOP("See append, available for backwards compatability"), 0 },
            { "queue", I18N_NOOP("Queue URLs after the currently playing track"), 0 },
            { "m", 0, 0 },
            { "toggle-playlist-window", I18N_NOOP("Toggle the Playlist-window"), 0 },
            { "wizard", I18N_NOOP( "Run First-run Wizard" ), 0 },
            { "engine <name>", I18N_NOOP( "Use the <name> engine" ), 0 },
            { 0, 0, 0 }
        };

    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &aboutData ); //calls KApplication::addCmdLineOptions()
    KCmdLineArgs::addCmdLineOptions( options );   //add our own options
}


#include <kaction.h>
#include <kshortcutlist.h>
void App::initGlobalShortcuts()
{
    EngineController* const ec = EngineController::instance();


    m_pGlobalAccel->insert( "play", i18n( "Play" ), 0, KKey("WIN+x"), 0,
                            ec, SLOT( play() ), true, true );
    m_pGlobalAccel->insert( "pause", i18n( "Pause" ), 0, KKey("WIN+c"), 0,
                            ec, SLOT( pause() ), true, true );
    m_pGlobalAccel->insert( "play_pause", i18n( "Play/Pause" ), 0, 0, 0,
                            ec, SLOT( playPause() ), true, true );
    m_pGlobalAccel->insert( "stop", i18n( "Stop" ), 0, KKey("WIN+v"), 0,
                            ec, SLOT( stop() ), true, true );
    m_pGlobalAccel->insert( "next", i18n( "Next Track" ), 0, KKey("WIN+b"), 0,
                            ec, SLOT( next() ), true, true );
    m_pGlobalAccel->insert( "prev", i18n( "Previous Track" ), 0, KKey("WIN+z"), 0,
                            ec, SLOT( previous() ), true, true );
    m_pGlobalAccel->insert( "volup", i18n( "Increase Volume" ), 0, KKey("WIN+KP_Add"), 0,
                            ec, SLOT( increaseVolume() ), true, true );
    m_pGlobalAccel->insert( "voldn", i18n( "Decrease Volume" ), 0, KKey("WIN+KP_Subtract"), 0,
                            ec, SLOT( decreaseVolume() ), true, true );
    m_pGlobalAccel->insert( "playlist_add", i18n( "Add Media..." ), 0, KKey("WIN+a"), 0,
                            m_pPlaylistWindow, SLOT( slotAddLocation() ), true, true );
    m_pGlobalAccel->insert( "show", i18n( "Toggle Playlist Window" ), 0, KKey("WIN+p"), 0,
                            m_pPlaylistWindow, SLOT( showHide() ), true, true );
    m_pGlobalAccel->insert( "osd", i18n( "Show OSD" ), 0, KKey("WIN+o"), 0,
                            amaroK::OSD::instance(), SLOT( forceToggleOSD() ), true, true );

    m_pGlobalAccel->setConfigGroup( "Shortcuts" );
    m_pGlobalAccel->readSettings( kapp->config() );
    m_pGlobalAccel->updateConnections();

    //TODO fix kde accel system so that kactions find appropriate global shortcuts
    //     and there is only one configure shortcuts dialog

    KActionCollection* const ac = amaroK::actionCollection();
    KAccelShortcutList list( m_pGlobalAccel );

    for( uint i = 0; i < list.count(); ++i )
    {
        KAction *action = ac->action( list.name( i ).latin1() );

        if( action )
        {
            //this is a hack really, also it means there may be two calls to the slot for the shortcut
            action->setShortcutConfigurable( false );
            action->setShortcut( list.shortcut( i ) );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// METHODS
/////////////////////////////////////////////////////////////////////////////////////

#include <taglib/id3v1tag.h>
#include <taglib/tbytevector.h>
#include <qtextcodec.h>

//this class is only used in this module, so I figured I may as well define it
//here and save creating another header/source file combination

class ID3v1StringHandler : public TagLib::ID3v1::StringHandler {
    QTextCodec *m_codec;
public:
    ID3v1StringHandler( int codecIndex )
            : m_codec( QTextCodec::codecForIndex( codecIndex ) )
    {}
    virtual TagLib::String parse( const TagLib::ByteVector &data ) const
    {
        return QStringToTString( m_codec->toUnicode( data.data(), data.size() ) );
    }
    virtual TagLib::ByteVector render( const TagLib::String &ts ) const
    {
        const QCString qcs = m_codec->fromUnicode( TStringToQString( ts ) );
        return TagLib::ByteVector( qcs, qcs.length() );
    }
};

//SLOT
void App::applySettings( bool firstTime )
{
    ///Called when the configDialog is closed with OK or Apply

    DEBUG_BLOCK

    //determine and apply colors first
    applyColorScheme();

    if( AmarokConfig::showPlayerWindow() )
    {
        if( !m_pPlayerWindow )
        {
            //the player Window becomes the main Window
            //it is the focus for hideWithMainWindow behaviour etc.
            //it gets the majestic "amaroK" caption
            m_pPlaylistWindow->setCaption( kapp->makeStdCaption( i18n("Playlist") ) );

            m_pPlayerWindow = new PlayerWidget( m_pPlaylistWindow, "PlayerWindow", firstTime && AmarokConfig::playlistWindowEnabled() );

            //don't show PlayerWindow on firstTime, that is done below
            //we need to explicately set the PL button if it's the first time
            if( !firstTime ) m_pPlayerWindow->show();

            connect( m_pPlayerWindow, SIGNAL(effectsWindowToggled( bool )), SLOT(slotConfigEffects( bool )) );
            connect( m_pPlayerWindow, SIGNAL(playlistToggled( bool )), m_pPlaylistWindow, SLOT(showHide()) );

            //TODO get this to work!
            //may work if you set no parent for the systray?
            //KWin::setSystemTrayWindowFor( m_pTray->winId(), m_pPlayerWindow->winId() );

            delete m_pTray; m_pTray = new amaroK::TrayIcon( m_pPlayerWindow );
        }
        else
            //this is called in the PlayerWindow ctor, hence the else
            m_pPlayerWindow->applySettings();

    } else if( m_pPlayerWindow ) {

        delete m_pTray; m_pTray = new amaroK::TrayIcon( m_pPlaylistWindow );
        delete m_pPlayerWindow; m_pPlayerWindow = 0;

        m_pPlaylistWindow->setCaption( "amaroK" );
        //m_pPlaylistWindow->show(); //must be shown //we do below now

        //ensure that at least one Menu is plugged into an accessible UI element
        if( !AmarokConfig::showMenuBar() && !amaroK::actionCollection()->action( "amarok_menu" )->isPlugged() )
           playlistWindow()->createGUI();
    }


    playlistWindow()->applySettings();
    Scrobbler::instance()->applySettings();
    amaroK::OSD::instance()->applySettings();
    CollectionDB::instance()->applySettings();
    amaroK::StatusBar::instance()->setShown( AmarokConfig::showStatusBar() );
    m_pTray->setShown( AmarokConfig::showTrayIcon() );

    if ( AmarokConfig::recodeID3v1Tags() )
        TagLib::ID3v1::Tag::setStringHandler( new ID3v1StringHandler( AmarokConfig::recodeEncoding() ) );


    //on startup we need to show the window, but only if it wasn't hidden on exit
    //and always if the trayicon isn't showing
    if( firstTime && !amaroK::config()->readBoolEntry( "HiddenOnExit", false ) || !AmarokConfig::showTrayIcon() )
    {
        mainWindow()->show();

        //takes longer but feels shorter. Crazy eh? :)
        kapp->eventLoop()->processEvents( QEventLoop::ExcludeUserInput );
    }


    { //<Engine>
        EngineBase *engine = EngineController::engine();

        if( firstTime || AmarokConfig::soundSystem() !=
                         PluginManager::getService( engine )->property( "X-KDE-amaroK-name" ).toString() )
        {
            //will unload engine for us first if necessary
            engine = EngineController::loadEngine();
        }

        AmarokConfig::setHardwareMixer( engine->setHardwareMixer( AmarokConfig::hardwareMixer() ) );

        engine->setXfadeLength( AmarokConfig::crossfade() ? AmarokConfig::crossfadeLength() : 0 );
        engine->setVolume( AmarokConfig::masterVolume() );

        engine->setEqualizerEnabled( AmarokConfig::equalizerEnabled() );
        if ( AmarokConfig::equalizerEnabled() )
            engine->setEqualizerParameters( AmarokConfig::equalizerPreamp(), AmarokConfig::equalizerGains() );
    } //</Engine>

    { //<Collection>
        CollectionView::instance()->renderView();
    } //</Collection>

    {   // delete unneeded cover images from cache
        const QString size = QString::number( AmarokConfig::coverPreviewSize() ) + '@';
        const QDir cacheDir = amaroK::saveLocation( "albumcovers/cache/" );
        const QStringList obsoleteCovers = cacheDir.entryList( "*" );
        foreach( obsoleteCovers )
            if ( !(*it).startsWith( size  ) && !(*it).startsWith( "50@" ) )
                QFile( cacheDir.filePath( *it ) ).remove();
    }

    //if ( !firstTime )
        // Bizarrely and ironically calling this causes crashes for
        // some people! FIXME
        //AmarokConfig::writeConfig();
}

void
App::applyColorScheme()
{
    DEBUG_BLOCK

    QColorGroup group;
    using amaroK::ColorScheme::AltBase;
    int h, s, v;
    QWidget* const browserBar = (QWidget*)playlistWindow()->child( "BrowserBar" );

    if( AmarokConfig::schemeKDE() )
    {
        AltBase = KGlobalSettings::alternateBackgroundColor();

        playlistWindow()->unsetPalette();
        browserBar->unsetPalette();

        PlayerWidget::determineAmarokColors();
    }

    else if( AmarokConfig::schemeAmarok() )
    {
        group = QApplication::palette().active();
        const QColor bg( amaroK::blue );
        AltBase.setRgb( 57, 64, 98 );

        group.setColor( QColorGroup::Text, Qt::white );
        group.setColor( QColorGroup::Base, bg );
        group.setColor( QColorGroup::Foreground, 0xd7d7ef );
        group.setColor( QColorGroup::Background, AltBase );

        group.setColor( QColorGroup::Button, AltBase );
        group.setColor( QColorGroup::ButtonText, 0xd7d7ef );

//         group.setColor( QColorGroup::Light,    Qt::cyan   /*lighter than Button color*/ );
//         group.setColor( QColorGroup::Midlight, Qt::blue   /*between Button and Light*/ );
//         group.setColor( QColorGroup::Dark,     Qt::green  /*darker than Button*/ );
//         group.setColor( QColorGroup::Mid,      Qt::red    /*between Button and Dark*/ );
//         group.setColor( QColorGroup::Shadow,   Qt::yellow /*a very dark color. By default, the shadow color is Qt::black*/ );

        group.setColor( QColorGroup::Highlight, Qt::white );
        group.setColor( QColorGroup::HighlightedText, bg );
        //group.setColor( QColorGroup::BrightText, QColor( 0xff, 0x40, 0x40 ) ); //GlowColor

        AltBase.getHsv( &h, &s, &v );
        group.setColor( QColorGroup::Midlight, QColor( h, s/3, (int)(v * 1.2), QColor::Hsv ) ); //column separator in playlist

        //TODO set all colours, even button colours, that way we can change the dark,
        //light, etc. colours and amaroK scheme will look much better

        using namespace amaroK::ColorScheme;
        Base       = amaroK::blue;
        Text       = Qt::white;
        Background = 0x002090;
        Foreground = 0x80A0FF;

        //all children() derive their palette from this
        playlistWindow()->setPalette( QPalette( group, group, group ) );
        browserBar->unsetPalette();
    }

    else if( AmarokConfig::schemeCustom() )
    {
        // we try to be smart: this code figures out contrasting colors for
        // selection and alternate background rows
        group = QApplication::palette().active();
        const QColor fg( AmarokConfig::playlistWindowFgColor() );
        const QColor bg( AmarokConfig::playlistWindowBgColor() );

        //TODO use the ensureContrast function you devised in BlockAnalyzer

        bg.hsv( &h, &s, &v );
        v += (v < 128) ? +50 : -50;
        v &= 255; //ensures 0 <= v < 256
        AltBase.setHsv( h, s, v );

        fg.hsv( &h, &s, &v );
        v += (v < 128) ? +150 : -150;
        v &= 255; //ensures 0 <= v < 256
        QColor highlight( h, s, v, QColor::Hsv );

        group.setColor( QColorGroup::Base, bg );
        group.setColor( QColorGroup::Background, bg.dark( 115 ) );
        group.setColor( QColorGroup::Text, fg );
        group.setColor( QColorGroup::Highlight, highlight );
        group.setColor( QColorGroup::HighlightedText, Qt::white );
        group.setColor( QColorGroup::Dark, Qt::darkGray );

        PlayerWidget::determineAmarokColors();

        // we only colour the middle section since we only
        // allow the user to choose two colours
        browserBar->setPalette( QPalette( group, group, group ) );
        playlistWindow()->unsetPalette();
    }

    // set the KListView alternate colours
    QObjectList* const list = playlistWindow()->queryList( "KListView" );
    for( QObject *o = list->first(); o; o = list->next() )
        static_cast<KListView*>(o)->setAlternateBackground( AltBase );
    delete list; //heap allocated!
}


bool amaroK::genericEventHandler( QWidget *recipient, QEvent *e )
{
    //this is used as a generic event handler for widgets that want to handle
    //typical events in an amaroK fashion

    //to use it just pass the event eg:
    //
    // void Foo::barEvent( QBarEvent *e )
    // {
    //     amaroK::genericEventHandler( this, e );
    // }

    switch( e->type() )
    {
    case QEvent::DragEnter:
        #define e static_cast<QDropEvent*>(e)
        e->accept( KURLDrag::canDecode( e ) );
        break;

    case QEvent::Drop:
        if( KURLDrag::canDecode( e ) )
        {
            QPopupMenu popup;
            //FIXME this isn't a good way to determine if there is a currentTrack, need playlist() function
            const bool b = EngineController::engine()->loaded();

            popup.insertItem( i18n( "&Append to Playlist" ), Playlist::Append );
            popup.insertItem( i18n( "Append && &Play" ), Playlist::DirectPlay | Playlist::Append );
            if( b ) popup.insertItem( i18n( "&Queue After Current Track" ), Playlist::Queue );
            popup.insertSeparator();
            popup.insertItem( i18n( "&Cancel" ), 0 );

            const int id = popup.exec( recipient->mapToGlobal( e->pos() ) );
            KURL::List list;
            KURLDrag::decode( e, list );

            if ( id > 0 )
                Playlist::instance()->insertMedia( list, id );
        }
        else return false;
        #undef e

        break;

    //this like every entry in the generic event handler is used by more than one widget
    //please don't remove!
    case QEvent::Wheel:
    {
        #define e static_cast<QWheelEvent*>(e)

        //this behaviour happens for the systray and the player window
        //to override one, override it in that class

        switch( e->state() )
        {
        case Qt::ControlButton:
        {
            const bool up = e->delta() > 0;

            //if this seems strange to you, please bring it up on #amarok
            //for discussion as we can't decide which way is best!
            if( up ) EngineController::instance()->previous();
            else     EngineController::instance()->next();
            break;
        }
        case Qt::ShiftButton:
        {
            int seek = EngineController::engine()->position() + ( e->delta() / 120 ) * 10000; // 10 seconds
            EngineController::engine()->seek( seek < 0 ? 0 : seek );
            break;
        }
        default:
            EngineController::instance()->increaseVolume( e->delta() / 18 );
        }

        e->accept();
        #undef e

        break;
    }

    case QEvent::Close:

        //KDE policy states we should hide to tray and not quit() when the
        //close window button is pushed for the main widget

        static_cast<QCloseEvent*>(e)->accept(); //if we don't do this the info box appears on quit()!

        if( AmarokConfig::showTrayIcon() && !e->spontaneous() && !kapp->sessionSaving() )
        {
            KMessageBox::information( recipient,
                i18n( "<qt>Closing the main-window will keep amaroK running in the System Tray. "
                      "Use <B>Quit</B> from the menu, or the amaroK tray-icon to exit the application.</qt>" ),
                i18n( "Docking in System Tray" ), "hideOnCloseInfo" );
        }
        else kapp->quit();

        break;

    default:
        return false;
    }

    return true;
}


void App::engineStateChanged( Engine::State state )
{
    switch( state )
    {
    case Engine::Empty:
        QToolTip::add( m_pTray, i18n( "amaroK - Audio Player" ) );
        break;

    default:
        ;
    }
}


void App::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    amaroK::OSD::instance()->show( bundle );

    TrackToolTip::add( m_pTray, bundle );
}


void App::engineVolumeChanged( int newVolume )
{
    amaroK::OSD::instance()->OSDWidget::show( i18n("Volume: %1%").arg( newVolume ) );
}

void App::slotConfigEffects( bool show ) //SLOT
{
    if( show )
    {
        if ( !EngineController::engine()->hasEffects() )
            KMessageBox::sorry( 0, i18n( "Effects are not available with this engine." ) );

        else if( !EffectWidget::self )
        {
            //safe even if m_pPlayerWindow is 0
            connect( EffectWidget::self, SIGNAL(destroyed()), m_pPlayerWindow, SLOT(setEffectsWindowShown()) );
            EffectWidget::self = new EffectWidget( m_pPlaylistWindow );
        }
        else EffectWidget::self->raise();
    }
    else
        delete EffectWidget::self; //will set self = 0 in its dtor

    if( m_pPlayerWindow ) m_pPlayerWindow->setEffectsWindowShown( show );
}


void App::slotConfigEqualizer() //SLOT
{
    EqualizerSetup::instance()->raise();
}


void App::slotConfigAmarok( const QCString& page )
{
    DEBUG_FUNC_INFO

    AmarokConfigDialog* dialog = (AmarokConfigDialog*) KConfigDialog::exists( "settings" );

    if( !dialog )
    {
        //KConfigDialog didn't find an instance of this dialog, so lets create it :
        dialog = new AmarokConfigDialog( m_pPlaylistWindow, "settings", AmarokConfig::self() );

        connect( dialog, SIGNAL(settingsChanged()), SLOT(applySettings()) );
    }

    //FIXME it seems that if the dialog is on a different desktop it gets lost
    //      what do to? detect and move it?

    dialog->show();
    dialog->raise();
    dialog->setActiveWindow();

    //so that if the engine page is needed to be shown it works
    kapp->processEvents();

    if ( !page.isNull() ) dialog->showPage( page );
}

void App::slotConfigShortcuts()
{
    KKeyDialog::configure( amaroK::actionCollection(), m_pPlaylistWindow );
}

void App::slotConfigGlobalShortcuts()
{
    KKeyDialog::configure( m_pGlobalAccel, true, m_pPlaylistWindow, true );
}

void App::slotConfigToolBars()
{
    PlaylistWindow* const pw = playlistWindow();
    KEditToolbar dialog( pw->actionCollection(), pw->xmlFile(), true, pw );

    dialog.showButtonApply( false );

    if( dialog.exec() )
    {
        playlistWindow()->reloadXML();
        playlistWindow()->createGUI();
    }
}


void App::firstRunWizard()
{
    ///show firstRunWizard

    FirstRunWizard wizard;
    setTopWidget( &wizard );
    wizard.setCaption( makeStdCaption( i18n( "First-Run Wizard" ) ) );

    if( wizard.exec() != QDialog::Rejected )
    {
        switch( wizard.interface() )
        {
        case FirstRunWizard::XMMS:
            amaroK::config()->writeEntry( "XMLFile", "amarokui_xmms.rc" );
            AmarokConfig::setShowPlayerWindow( true );
            //FIXME the statusbar is now quite essential and also without it
            // the popup messages break. Fix in 1.2.1
            AmarokConfig::setShowStatusBar( /*false*/ true );
            break;

        case FirstRunWizard::Compact:
            amaroK::config()->writeEntry( "XMLFile", "amarokui.rc" );
            AmarokConfig::setShowPlayerWindow( false );
            AmarokConfig::setShowStatusBar( true );
            break;
        }

        const QStringList oldCollectionFolders = AmarokConfig::collectionFolders();
        wizard.writeCollectionConfig();

        // If wizard is invoked at runtime, rescan collection if folder setup has changed
        if ( !amaroK::config()->readBoolEntry( "First Run", true ) &&
             oldCollectionFolders != AmarokConfig::collectionFolders() )
            CollectionDB::instance()->startScan();
    }
}


void App::pruneCoverImages()
{
#ifdef AMAZON_SUPPORT
    // TODO Offer the option to refresh the images, instead of deleting

    const int MAX_DAYS = 90;

    QDir covers( amaroK::saveLocation( "albumcovers/large/" ) );
    QDir coverCache( amaroK::saveLocation( "albumcovers/cache/" ) );

    QFileInfoList list( *covers.entryInfoList( QDir::Files ) );
    QFileInfoList listCache( *coverCache.entryInfoList( QDir::Files ) );

    // Merge both lists
    for ( uint i = 0; i < listCache.count(); i++ )
        list.append( listCache.at( i ) );

    // Prune files
    uint count = 0;
    uint pruneCount = 0;
    const QDate currentDate = QDate::currentDate();
    for ( uint i = 0; i < list.count(); ++i, ++count )
        if ( list.at( i )->created().date().daysTo( currentDate ) > MAX_DAYS ) {
            pruneCount++;
            QFile::remove( list.at( i )->absFilePath() );
        }

    if ( pruneCount )
        amaroK::StatusBar::instance()->longMessage( i18n(
                "amaroK has had to delete some of the images that you downloaded from Amazon. "
                "Amazon's licensing policy require us to remove content within 90 days of its download. "
                "You can read more about Amazon's policies "
                "<a href='http://www.amazon.com/webservices'>here</a>."
                "Please note that this only affects covers downloaded using the Cover Manager's fetch covers feature and the covers downloaded using the context menu in the Context browser, not manually added covers." ) );
#endif
}

QWidget *App::mainWindow() const
{
   return AmarokConfig::showPlayerWindow() ? (QWidget*)m_pPlayerWindow : (QWidget*)m_pPlaylistWindow;
}

namespace amaroK
{
    /// @see amarok.h

    QWidget *mainWindow()
    {
        return pApp->playlistWindow();
    }

    KActionCollection *actionCollection()
    {
        return pApp->playlistWindow()->actionCollection();
    }

    KConfig *config( const QString &group )
    {
        //Slightly more useful config() that allows setting the group simultaneously

        kapp->config()->setGroup( group );
        return kapp->config();
    }

    namespace ColorScheme
    {
        QColor Base;
        QColor Text;
        QColor Background;
        QColor Foreground;
        QColor AltBase;
    }

    OverrideCursor::OverrideCursor( Qt::CursorShape cursor )
    {
        QApplication::setOverrideCursor( cursor == Qt::WaitCursor ? KCursor::waitCursor() : KCursor::workingCursor() );
    }

    OverrideCursor::~OverrideCursor()
    {
        QApplication::restoreOverrideCursor();
    }

    QString saveLocation( const QString &directory )
    {
        return KGlobal::dirs()->saveLocation( "data", QString("amarok/") + directory, true );
    }
}

namespace Debug
{
    /// @see debug.h

    QCString indent; //declared in debug.h
}

#include "app.moc"
