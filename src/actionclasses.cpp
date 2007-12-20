// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#include "actionclasses.h"

#include "config-amarok.h"             //HAVE_LIBVISUAL definition

#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "debug.h"
#include "collectiondb.h"
#include "CoverManager.h"
#include "enginecontroller.h"
#include "k3bexporter.h"
#include "MainWindow.h"
//#include "mediumpluginmanager.h"

#include "socketserver.h"       //Vis::Selector::showInstance()
#include "ContextStatusBar.h"
#include "threadmanager.h"
#include "timeLabel.h"

#include <KHBox>
#include <KHelpMenu>
#include <KIconLoader>
#include <KLineEdit>
#include <KLocale>
#include <KPushButton>
#include <KStandardDirs>
#include <KToolBar>
#include <KUrl>
#include <KAuthorized>

#include <QPixmap>
#include <QToolTip>
#include <Q3PopupMenu>


namespace Amarok
{
    bool repeatNone() { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Off; }
    bool repeatTrack() { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Track; }
    bool repeatAlbum() { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Album; }
    bool repeatPlaylist() { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Playlist; }
    bool randomOff() { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Off; }
    bool randomTracks() { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Tracks; }
    bool randomAlbums() { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Albums; }
    bool favorNone() { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::Off; }
    bool favorScores() { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::HigherScores; }
    bool favorRatings() { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::HigherRatings; }
    bool favorLastPlay() { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::LessRecentlyPlayed; }
    bool entireAlbums() { return repeatAlbum() || randomAlbums(); }
}

using namespace Amarok;

KHelpMenu *Menu::s_helpMenu = 0;

static void
safePlug( KActionCollection *ac, const char *name, QWidget *w )
{
    if( ac )
    {
        KAction *a = (KAction*) ac->action( name );
        if( a && w ) w->addAction( a );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// MenuAction && Menu
// KActionMenu doesn't work very well, so we derived our own
//////////////////////////////////////////////////////////////////////////////////////////

MenuAction::MenuAction( KActionCollection *ac )
  : KAction( 0 )
{
    setText(i18n( "Amarok Menu" ));
    ac->addAction("amarok_menu", this);
    setShortcutConfigurable ( false ); //FIXME disabled as it doesn't work, should use QCursor::pos()
}

int
MenuAction::plug( QWidget *w, int )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && KAuthorized::authorizeKAction( objectName() ) )
    {
        //const int id = KAction::getToolButtonID();

        //addContainer( bar, id );
        w->addAction( this );
        connect( bar, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        //TODO create menu on demand
        //TODO create menu above and aligned within window
        //TODO make the arrow point upwards!
        //bar->insertButton( QString::null, id, true, i18n( "Menu" ), index );
        //bar->alignItemRight( id );

        //KToolBarButton* button = bar->getButton( id );
        //button->setPopup( Amarok::Menu::instance() );
        //button->setObjectName( "toolbutton_amarok_menu" );
        //button->setIcon( "amarok" );

        return associatedWidgets().count() - 1;
    }
    else return -1;
}

Menu::Menu()
{
    KActionCollection *ac = Amarok::actionCollection();

    safePlug( ac, "repeat", this );
    safePlug( ac, "random_mode", this );

    addSeparator();

    safePlug( ac, "playlist_playmedia", this );
    safePlug( ac, "play_audiocd", this );
    safePlug( ac, "lastfm_play", this );

    addSeparator();

    safePlug( ac, "cover_manager", this );
    safePlug( ac, "queue_manager", this );
    safePlug( ac, "visualizations", this );
    safePlug( ac, "equalizer", this );
    safePlug( ac, "script_manager", this );
    safePlug( ac, "statistics", this );

    addSeparator();

    safePlug( ac, "update_collection", this );
    safePlug( ac, "rescan_collection", this );

#ifndef Q_WS_MAC
    addSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::ShowMenubar), this );
#endif

    addSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::ConfigureToolbars), this );
    safePlug( ac, KStandardAction::name(KStandardAction::KeyBindings), this );
//    safePlug( ac, "options_configure_globals", this ); //we created this one
    safePlug( ac, KStandardAction::name(KStandardAction::Preferences), this );

    addSeparator();

    addMenu( helpMenu( this ) );

    addSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::Quit), this );

    #ifdef HAVE_LIBVISUAL
    Amarok::actionCollection()->action( "visualizations" )->setEnabled( false );
    #endif
}

Menu*
Menu::instance()
{
    static Menu menu;
    return &menu;
}

KMenu*
Menu::helpMenu( QWidget *parent ) //STATIC
{
    extern KAboutData aboutData;

    if ( s_helpMenu == 0 )
        s_helpMenu = new KHelpMenu( parent, &aboutData, Amarok::actionCollection() );
        
    return s_helpMenu->menu();
}

//////////////////////////////////////////////////////////////////////////////////////////
// PlayPauseAction
//////////////////////////////////////////////////////////////////////////////////////////

PlayPauseAction::PlayPauseAction( KActionCollection *ac )
        : KToggleAction( 0 )
        , EngineObserver( EngineController::instance() )
{
    setText(i18n( "Play/Pause" ));
    ac->addAction("play_pause", this);

    engineStateChanged( EngineController::engine()->state() );

    connect( this, SIGNAL(triggered()), EngineController::instance(), SLOT(playPause()) );
}

void
PlayPauseAction::engineStateChanged( Engine::State state,  Engine::State /*oldState*/ )
{
    switch( state ) {
    case Engine::Playing:
        setChecked( false );
        setIcon( KIcon(Amarok::icon( "pause" )) );
        setText( i18n( "Pause" ) );
        break;
    case Engine::Paused:
        setChecked( true );
        setIcon( KIcon(Amarok::icon( "pause" )) );
        setText( i18n( "Pause" ) );
        break;
    case Engine::Empty:
        setChecked( false );
        setIcon( KIcon(Amarok::icon( "play" )) );
        setText( i18n( "Play" ) );
        break;
    case Engine::Idle:
        return;
    }
}
//////////////////////////////////////////////////////////////////////////////////////////
// ToggleAction
//////////////////////////////////////////////////////////////////////////////////////////

ToggleAction::ToggleAction( const QString &text, void ( *f ) ( bool ), KActionCollection* const ac, const char *name )
        : KToggleAction( 0 )
        , m_function( f )
{
    setText(text);
    ac->addAction(name, this);
}

void ToggleAction::setChecked( bool b )
{
    const bool announce = b != isChecked();

    m_function( b );
    KToggleAction::setChecked( b );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit toggled( b ); //KToggleAction doesn't do this for us. How gay!
}

void ToggleAction::setEnabled( bool b )
{
    const bool announce = b != isEnabled();

    if( !b )
        setChecked( false );
    KToggleAction::setEnabled( b );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit QAction::triggered( b );
}

//////////////////////////////////////////////////////////////////////////////////////////
// SelectAction
//////////////////////////////////////////////////////////////////////////////////////////

SelectAction::SelectAction( const QString &text, void ( *f ) ( int ), KActionCollection* const ac, const char *name )
        : KSelectAction( 0 )
        , m_function( f )
{
    setText(text);
    ac->addAction(name, this);
}

void SelectAction::setCurrentItem( int n )
{
    const bool announce = n != currentItem();

    m_function( n );
    KSelectAction::setCurrentItem( n );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit triggered( n );
}

void SelectAction::setEnabled( bool b )
{
    const bool announce = b != isEnabled();

    if( !b )
        setCurrentItem( 0 );
    KSelectAction::setEnabled( b );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit QAction::triggered( b );
}

void SelectAction::setIcons( QStringList icons )
{
    m_icons = icons;
    for( int i = 0, n = items().count(); i < n; ++i )
        menu()->changeItem( i, KIconLoader::global()->loadIconSet( icons.at( i ), KIconLoader::Small ), menu()->text( i ) );
}

QStringList SelectAction::icons() const { return m_icons; }

QString SelectAction::currentIcon() const
{
    if( m_icons.count() )
        return m_icons.at( currentItem() );
    return QString();
}

QString SelectAction::currentText() const {
    return KSelectAction::currentText() + "<br /><br />" + i18n("Click to change");
}

//////////////////////////////////////////////////////////////////////////////////////////
// RandomAction
//////////////////////////////////////////////////////////////////////////////////////////
RandomAction::RandomAction( KActionCollection *ac ) :
    SelectAction( i18n( "Ra&ndom" ), &AmarokConfig::setRandomMode, ac, "random_mode" )
{
    setItems( QStringList() << i18n( "&Off" ) << i18n( "&Tracks" ) << i18n( "&Albums" ) );
    setCurrentItem( AmarokConfig::randomMode() );
    setIcons( QStringList() << Amarok::icon( "random_no" ) << Amarok::icon( "random_track" ) << Amarok::icon( "random_album" ) );
}

void
RandomAction::setCurrentItem( int n )
{
    // Porting
    //if( KAction *a = parentCollection()->action( "favor_tracks" ) )
    //    a->setEnabled( n );
    SelectAction::setCurrentItem( n );
}


//////////////////////////////////////////////////////////////////////////////////////////
// FavorAction
//////////////////////////////////////////////////////////////////////////////////////////
FavorAction::FavorAction( KActionCollection *ac ) :
    SelectAction( i18n( "&Favor" ), &AmarokConfig::setFavorTracks, ac, "favor_tracks" )
{
    setItems( QStringList() << i18n( "Off" )
                            << i18n( "Higher &Scores" )
                            << i18n( "Higher &Ratings" )
                            << i18n( "Not Recently &Played" ) );

    setCurrentItem( AmarokConfig::favorTracks() );
    setEnabled( AmarokConfig::randomMode() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatAction::RepeatAction( KActionCollection *ac ) :
    SelectAction( i18n( "&Repeat" ), &AmarokConfig::setRepeat, ac, "repeat" )
{
    setItems( QStringList() << i18n( "&Off" ) << i18n( "&Track" )
                            << i18n( "&Album" ) << i18n( "&Playlist" ) );
    setIcons( QStringList() << Amarok::icon( "repeat_no" ) << Amarok::icon( "repeat_track" ) << Amarok::icon( "repeat_album" ) << Amarok::icon( "repeat_playlist" ) );
    setCurrentItem( AmarokConfig::repeat() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// BurnMenuAction
//////////////////////////////////////////////////////////////////////////////////////////
BurnMenuAction::BurnMenuAction( KActionCollection *ac )
  : KAction( 0 )
{
    setText(i18n( "Burn" ));
    ac->addAction("burn_menu", this);
}

QWidget*
BurnMenuAction::createWidget( QWidget *w )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && KAuthorized::authorizeKAction( objectName() ) )
    {
        //const int id = KAction::getToolButtonID();

        //addContainer( bar, id );
        w->addAction( this );
        connect( bar, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        //bar->insertButton( QString::null, id, true, i18n( "Burn" ), index );

        //KToolBarButton* button = bar->getButton( id );
        //button->setPopup( Amarok::BurnMenu::instance() );
        //button->setObjectName( "toolbutton_burn_menu" );
        //button->setIcon( "k3b" );

        //return associatedWidgets().count() - 1;
        return 0;
    }
    //else return -1;
    else return 0;
}

BurnMenu::BurnMenu()
{
    addAction( i18n("Current Playlist"), this, SLOT( slotBurnCurrentPlaylist() ) );
    addAction( i18n("Selected Tracks"), this, SLOT( slotBurnSelectedTracks() ) );
    //TODO add "album" and "all tracks by artist"
}

KMenu*
BurnMenu::instance()
{
    static BurnMenu menu;
    return &menu;
}

void
BurnMenu::slotBurnCurrentPlaylist() //SLOT
{
    K3bExporter::instance()->exportCurrentPlaylist();
}

void
BurnMenu::slotBurnSelectedTracks() //SLOT
{
    K3bExporter::instance()->exportSelectedTracks();
}


//////////////////////////////////////////////////////////////////////////////////////////
// StopMenuAction
//////////////////////////////////////////////////////////////////////////////////////////

StopAction::StopAction( KActionCollection *ac )
  : KAction( 0 )
{
    setText( i18n( "Stop" ) );
    setIcon( KIcon(Amarok::icon( "stop" )) );
    connect( this, SIGNAL( triggered() ), EngineController::instance(), SLOT( stop() ) );
    ac->addAction( "stop", this );
}

int
StopAction::plug( QWidget *w, int )
{
    //KToolBar *bar = dynamic_cast<KToolBar*>(w);
    w->addAction( this );
    /*
    if( bar && KAuthorized::authorizeKAction( name() ) )
    {
        //const int id = KAction::getToolButtonID();

        //addContainer( bar, id );

        w->addAction( this );
        connect( bar, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        //bar->insertButton( QString::null, id, SIGNAL( clicked() ), EngineController::instance(), SLOT( stop() ),
        //                   true, i18n( "Stop" ), index );

        //KToolBarButton* button = bar->getButton( id );
        //button->setDelayedPopup( Amarok::StopMenu::instance() );
        //button->setObjectName( "toolbutton_stop_menu" );
        //button->setIcon( Amarok::icon( "stop" ) );
        //button->setEnabled( EngineController::instance()->engine()->loaded() );  // Disable button at startup

        return associatedWidgets().count() - 1;
    }
    else return QAction::plug( w, index );
    */
    return 1;
}

StopMenu::StopMenu()
{
    addTitle( i18n( "Stop" ) );

    m_stopNow        = addAction( i18n( "Now" ), this, SLOT( slotStopNow() ) );
    m_stopAfterTrack = addAction( i18n( "After Current Track" ), this, SLOT( slotStopAfterTrack() ) );
    m_stopAfterQueue = addAction( i18n( "After Queue" ), this, SLOT( slotStopAfterQueue() ) );

    connect( this, SIGNAL( aboutToShow() ),  SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );
}

KMenu*
StopMenu::instance()
{
    static StopMenu menu;
    return &menu;
}

void
StopMenu::slotAboutToShow()
{
    //PORT 2.0
//     Playlist *pl = Playlist::instance();

    m_stopNow->setEnabled( Amarok::actionCollection()->action( "stop" )->isEnabled() );

    m_stopAfterTrack->setEnabled( EngineController::engine()->loaded() );
//     m_stopAfterTrack->setChecked( pl->stopAfterMode() == Playlist::StopAfterCurrent );

//     m_stopAfterQueue->setEnabled( pl->nextTracks().count() );
//     m_stopAfterQueue->setChecked( pl->stopAfterMode() == Playlist::StopAfterQueue );
}

void
StopMenu::slotStopNow() //SLOT
{
    //PORT 2.0
//     Playlist* pl = Playlist::instance();
//     const int mode = pl->stopAfterMode();

//     Amarok::actionCollection()->action( "stop" )->trigger();
//     if( mode == Playlist::StopAfterCurrent || mode == Playlist::StopAfterQueue )
//         pl->setStopAfterMode( Playlist::DoNotStop );
}

void
StopMenu::slotStopAfterTrack() //SLOT
{
    //PORT 2.0
//     Playlist* pl = Playlist::instance();
//     const int mode = pl->stopAfterMode();

//     pl->setStopAfterMode( mode == Playlist::StopAfterCurrent
//                                 ? Playlist::DoNotStop
//                                 : Playlist::StopAfterCurrent );
}

void
StopMenu::slotStopAfterQueue() //SLOT
{
    //PORT 2.0
//     Playlist* pl = Playlist::instance();
//     const int mode = pl->stopAfterMode();

//     pl->setStopAfterMode( mode == Playlist::StopAfterQueue
//                                 ? Playlist::DoNotStop
//                                 : Playlist::StopAfterQueue );
}

#include "actionclasses.moc"
