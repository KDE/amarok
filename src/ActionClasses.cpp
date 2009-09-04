/****************************************************************************************
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ActionClasses.h"

#include <config-amarok.h>

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "statusbar/StatusBar.h"
#include "amarokconfig.h"
#include "covermanager/CoverManager.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"

#include <KAuthorized>
#include <KHelpMenu>
#include <KLocale>
#include <KToolBar>
#include <Osd.h>


extern KAboutData aboutData;
extern OcsData ocsData;

namespace Amarok
{
    bool repeatNone()     { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Off; }
    bool repeatTrack()    { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Track; }
    bool repeatAlbum()    { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Album; }
    bool repeatPlaylist() { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Playlist; }
    bool randomOff()      { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Off; }
    bool randomTracks()   { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Tracks; }
    bool randomAlbums()   { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Albums; }
    bool favorNone()      { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::Off; }
    bool favorScores()    { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::HigherScores; }
    bool favorRatings()   { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::HigherRatings; }
    bool favorLastPlay()  { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::LessRecentlyPlayed; }

    bool entireAlbums()   { return repeatAlbum()  || randomAlbums(); }
    bool repeatEnabled()  { return repeatTrack()  || repeatAlbum() || repeatPlaylist(); }
    bool randomEnabled()  { return randomTracks() || randomAlbums(); }
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

MenuAction::MenuAction( KActionCollection *ac, QObject *parent )
  : KAction( parent )
{
    setText(i18n( "Amarok Menu" ));
    ac->addAction("amarok_menu", this);
    setShortcutConfigurable ( false ); //FIXME disabled as it doesn't work, should use QCursor::pos()
}


Menu* Menu::s_instance = 0;

Menu::Menu( QWidget* parent )
    : KMenu( parent )
{
    s_instance = this;

    KActionCollection *ac = Amarok::actionCollection();

    safePlug( ac, "repeat", this );
    safePlug( ac, "random_mode", this );

    addSeparator();

    safePlug( ac, "playlist_playmedia", this );

    addSeparator();

    safePlug( ac, "cover_manager", this );
    safePlug( ac, "queue_manager", this );
    safePlug( ac, "script_manager", this );

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
}

Menu*
Menu::instance()
{
    return s_instance ? s_instance : new Menu( The::mainWindow() );
}

KMenu*
Menu::helpMenu( QWidget *parent ) //STATIC
{
    if ( s_helpMenu == 0 )
        s_helpMenu = new KHelpMenu( parent, &aboutData, Amarok::actionCollection() );

    KMenu* menu = s_helpMenu->menu();


    // NOTE: We hide the "Amarok Handbook" entry until the handbook actually exists (WIP)
    s_helpMenu->action( KHelpMenu::menuHelpContents )->setVisible( false );

    // NOTE: "What's This" isn't currently defined for anything in Amarok, so let's remove that too
    s_helpMenu->action( KHelpMenu::menuWhatsThis )->setVisible( false );

    s_helpMenu->action( KHelpMenu::menuAboutApp )->setVisible( false );

    QAction *extendedAboutAction = new QAction( KIcon( "amarok" ), i18n( "&About Amarok" ), menu ); //translateme
    menu->insertAction( s_helpMenu->action(KHelpMenu::menuAboutKDE ),extendedAboutAction );
    connect( extendedAboutAction, SIGNAL(triggered()), The::mainWindow(), SLOT(showAbout()) );
    return menu;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PlayPauseAction
//////////////////////////////////////////////////////////////////////////////////////////

PlayPauseAction::PlayPauseAction( KActionCollection *ac, QObject *parent )
        : KToggleAction( parent )
        , EngineObserver( The::engineController() )
{
    ac->addAction( "play_pause", this );
    setText( i18n( "Play/Pause" ) );
    setShortcut( Qt::Key_Space );
    setGlobalShortcut( KShortcut( Qt::META + Qt::Key_C ) );
    PERF_LOG( "PlayPauseAction: before engineStateChanged" )
    engineStateChanged( The::engineController()->state() );
    PERF_LOG( "PlayPauseAction: after engineStateChanged" )

    connect( this, SIGNAL(triggered()), The::engineController(), SLOT(playPause()) );
}

void
PlayPauseAction::engineStateChanged( Phonon::State state,  Phonon::State oldState )
{
    DEBUG_BLOCK
    debug() << "NEWSTATE: " << state << "OLDSTATE: " << oldState;
    switch( state )
    {
    case Phonon::PlayingState:
        setChecked( false );
        setIcon( KIcon("media-playback-pause-amarok") );
        break;
    case Phonon::PausedState:
        setChecked( true );
        setIcon( KIcon("media-playback-start-amarok") );
        break;
    case Phonon::StoppedState:
    case Phonon::LoadingState:
        setChecked( false );
        setIcon( KIcon("media-playback-start-amarok") );
        break;
    case Phonon::ErrorState:
    case Phonon::BufferingState:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// ToggleAction
//////////////////////////////////////////////////////////////////////////////////////////

ToggleAction::ToggleAction( const QString &text, void ( *f ) ( bool ), KActionCollection* const ac, const char *name, QObject *parent )
        : KToggleAction( parent )
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

    KToggleAction::setEnabled( b );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit QAction::triggered( b );
}

//////////////////////////////////////////////////////////////////////////////////////////
// SelectAction
//////////////////////////////////////////////////////////////////////////////////////////

SelectAction::SelectAction( const QString &text, void ( *f ) ( int ), KActionCollection* const ac, const char *name, QObject *parent )
        : KSelectAction( parent )
        , m_function( f )
{
    PERF_LOG( "In SelectAction" );
    setText(text);
    ac->addAction(name, this);
}

void SelectAction::setCurrentItem( int n )
{
    const bool announce = n != currentItem();

    debug() << "setCurrentItem: " << n;

    m_function( n );
    KSelectAction::setCurrentItem( n );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit triggered( n );
}

void SelectAction::actionTriggered( QAction *a )
{
    m_function( currentItem() );
    AmarokConfig::self()->writeConfig();
    KSelectAction::actionTriggered( a );
}

void SelectAction::setEnabled( bool b )
{
    const bool announce = b != isEnabled();

    KSelectAction::setEnabled( b );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit QAction::triggered( b );
}

void SelectAction::setIcons( QStringList icons )
{
    m_icons = icons;
    foreach( QAction *a, selectableActionGroup()->actions() )
    {
        a->setIcon( KIcon(icons.takeFirst()) );
    }
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
RandomAction::RandomAction( KActionCollection *ac, QObject *parent ) :
    SelectAction( i18n( "Ra&ndom" ), &AmarokConfig::setRandomMode, ac, "random_mode", parent )
{
    QStringList items;
    items << i18nc( "State, as in disabled", "&Off" )
          << i18nc( "Items, as in music"   , "&Tracks" )
          << i18n( "&Albums" );
    setItems( items );

    setCurrentItem( AmarokConfig::randomMode() );

    QStringList icons;
    icons << "media-playlist-shuffle-off-amarok"
          << "media-playlist-shuffle-amarok"
          << "media-album-shuffle-amarok";
    setIcons( icons );

    connect( this, SIGNAL( triggered( int ) ), The::playlistActions(), SLOT( playlistModeChanged() ) );
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
FavorAction::FavorAction( KActionCollection *ac, QObject *parent ) :
    SelectAction( i18n( "&Favor" ), &AmarokConfig::setFavorTracks, ac, "favor_tracks", parent )
{
    setItems( QStringList() << i18nc( "State, as in disabled", "Off" )
                            << i18n( "Higher &Scores" )
                            << i18n( "Higher &Ratings" )
                            << i18n( "Not Recently &Played" ) );

    setCurrentItem( AmarokConfig::favorTracks() );
    setEnabled( true );
    connect( this, SIGNAL( triggered( int ) ), The::playlistActions(), SLOT( playlistModeChanged() ) );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatAction::RepeatAction( KActionCollection *ac, QObject *parent ) :
    SelectAction( i18n( "&Repeat" ), &AmarokConfig::setRepeat, ac, "repeat", parent )
{
    setItems( QStringList() << i18nc( "State, as in, disabled", "&Off" ) << i18nc( "Item, as in, music", "&Track" )
                            << i18n( "&Album" ) << i18n( "&Playlist" ) );
    setIcons( QStringList() << "media-playlist-repeat-off-amarok" << "media-track-repeat-amarok" << "media-album-repeat-amarok" << "media-playlist-repeat-amarok" );
    setCurrentItem( AmarokConfig::repeat() );

    connect( this, SIGNAL( triggered( int ) ), The::playlistActions(), SLOT( playlistModeChanged() ) );
}

//////////////////////////////////////////////////////////////////////////////////////////
// ReplayGainModeAction
//////////////////////////////////////////////////////////////////////////////////////////
ReplayGainModeAction::ReplayGainModeAction( KActionCollection *ac, QObject *parent ) :
    SelectAction( i18n( "&Replay Gain Mode" ), &AmarokConfig::setReplayGainMode, ac, "replay_gain_mode", parent )
{
    setItems( QStringList() << i18nc( "State, as in, disabled", "&Off" ) << i18nc( "Item, as in, music", "&Track" )
                            << i18n( "&Album" ) );
    //setIcons( QStringList() << "media-playlist-replaygain-off-amarok" << "media-track-replaygain-amarok" << "media-album-replaygain-amarok" );
    setCurrentItem( AmarokConfig::replayGainMode() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// EqualizerAction
//////////////////////////////////////////////////////////////////////////////////////////
EqualizerAction::EqualizerAction( KActionCollection *ac, QObject *parent ) :
    SelectAction( i18n( "&Equalizer" ), &AmarokConfig::setEqualizerMode, ac, "equalizer_mode", parent )
{
    // build a new preset list in menu
    newList();
    // set selected preset from config
    updateContent();
    connect( this, SIGNAL( triggered( int ) ), this, SLOT( actTrigg( int ) ) );
}

void
EqualizerAction::updateContent() //SLOT
{
    // this slot update the content of equalizer main window menu
    // according to config blocking is neccessary to prevent
    // circluar loop between menu and config dialog
    blockSignals( true ); 
    setCurrentItem( AmarokConfig::equalizerMode() );
    blockSignals( false );
}

void
EqualizerAction::newList() //SLOT
{
    // this slot build a new list of presets in equalizer menu
    // or disable this menu if equalizer is not supported
    if( !The::engineController()->isEqSupported() )
    {
        setEnabled( false );
        setToolTip( i18n("Your current setup does not support the equalizer feature") );
        return;
    }
    setEnabled( true );
    setToolTip( QString() );
    setItems( QStringList() << i18nc( "State, as in, disabled", "&Off" ) << eqGlobalList() );
}

void
EqualizerAction::actTrigg( int index ) //SLOT
{
    if( The::engineController()->isEqSupported() )
    {
        AmarokConfig::setEqualizerGains( eqCfgGetPresetVal( index - 1 ) );
        The::engineController()->eqUpdate();
    }
}

QStringList
EqualizerAction::eqGlobalList()
{
    // Prepare a global list with duplicates removed
    QStringList mGlobalList;
    mGlobalList += AmarokConfig::defEqualizerPresetsNames();
    foreach( QString mUsrName, AmarokConfig::equalizerPresetsNames() )
    {
        if( mGlobalList.indexOf( mUsrName ) < 0 )
            mGlobalList.append( mUsrName );
    }
    return mGlobalList;
}

QList<int>
EqualizerAction::eqCfgGetPresetVal( int mPresetNo )
{
    QList<int> mPresetVal;
    if( mPresetNo > eqGlobalList().count() ||  mPresetNo < 0 )
        return mPresetVal;
    QString mPresetName = eqGlobalList().at(mPresetNo);
    int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( mPresetName );
    int idDef = AmarokConfig::defEqualizerPresetsNames().indexOf( mPresetName );

    if( idUsr >= 0 )
        mPresetVal = AmarokConfig::equalizerPresestValues().mid( idUsr*11,11 );
    else if( idDef >= 0)
        mPresetVal = AmarokConfig::defEqualizerPresestValues().mid( idDef*11,11 );

    return mPresetVal;
}

//////////////////////////////////////////////////////////////////////////////////////////
// BurnMenuAction
//////////////////////////////////////////////////////////////////////////////////////////
BurnMenuAction::BurnMenuAction( KActionCollection *ac, QObject *parent )
  : KAction( parent )
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


BurnMenu* BurnMenu::s_instance = 0;

BurnMenu::BurnMenu( QWidget* parent )
    : KMenu( parent )
{
    s_instance = this;

    addAction( i18n("Current Playlist"), this, SLOT( slotBurnCurrentPlaylist() ) );
    addAction( i18n("Selected Tracks"), this, SLOT( slotBurnSelectedTracks() ) );
    //TODO add "album" and "all tracks by artist"
}

KMenu*
BurnMenu::instance()
{
    return s_instance ? s_instance : new BurnMenu( The::mainWindow() );
}

void
BurnMenu::slotBurnCurrentPlaylist() //SLOT
{
    //K3bExporter::instance()->exportCurrentPlaylist();
}

void
BurnMenu::slotBurnSelectedTracks() //SLOT
{
    //K3bExporter::instance()->exportSelectedTracks();
}


//////////////////////////////////////////////////////////////////////////////////////////
// StopAction
//////////////////////////////////////////////////////////////////////////////////////////

StopAction::StopAction( KActionCollection *ac, QObject *parent )
  : KAction( parent )
  , EngineObserver( The::engineController() )
{
    ac->addAction( "stop", this );
    setText( i18n( "Stop" ) );
    setIcon( KIcon("media-playback-stop-amarok") );
    setGlobalShortcut( KShortcut( Qt::META + Qt::Key_V ) );
    connect( this, SIGNAL( triggered() ), The::engineController(), SLOT( stop() ) );
    setEnabled( false );  // Disable action at startup
}

void
StopAction::engineStateChanged( Phonon::State state,  Phonon::State /*oldState*/ )
{
    switch( state ) {
    case Phonon::PlayingState:
    case Phonon::PausedState:
        setEnabled( true );
        break;
    case Phonon::StoppedState:
    case Phonon::LoadingState:
        setDisabled( true );
        break;
    case Phonon::ErrorState:
    case Phonon::BufferingState:
        break;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// StopPlayingAfterCurrentTrackAction
//////////////////////////////////////////////////////////////////////////////////////////

StopPlayingAfterCurrentTrackAction::StopPlayingAfterCurrentTrackAction( KActionCollection *ac, QObject *parent )
: KAction( parent )
, EngineObserver( The::engineController() )
{
    ac->addAction( "stop_after_current", this );
    setText( i18n( "Stop after current Track" ) );
    setIcon( KIcon("media-playback-stop-amarok") );
    setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_V ) );
    connect( this, SIGNAL( triggered() ), SLOT( stopPlayingAfterCurrentTrack() ) );
    setEnabled( false );  // Disable action at startup
}

void
StopPlayingAfterCurrentTrackAction::stopPlayingAfterCurrentTrack()
{
    if ( !The::playlistActions()->willStopAfterTrack( Playlist::ModelStack::instance()->source()->activeId() ) )
    {
        The::playlistActions()->setStopAfterMode( Playlist::StopAfterCurrent );
        The::playlistActions()->setTrackToBeLast( Playlist::ModelStack::instance()->source()->activeId() );
        Amarok::OSD::instance()->setImage( QImage( KIconLoader::global()->iconPath( "amarok", -KIconLoader::SizeHuge ) ) );
        Amarok::OSD::instance()->OSDWidget::show( i18n( "Stop after current track: On" ) );       
    } else {
        The::playlistActions()->setStopAfterMode( Playlist::StopNever );
        The::playlistActions()->setTrackToBeLast( 0 );
        Amarok::OSD::instance()->setImage( QImage( KIconLoader::global()->iconPath( "amarok", -KIconLoader::SizeHuge ) ) );
        Amarok::OSD::instance()->OSDWidget::show( i18n( "Stop after current track: Off" ) );
    }
}

void
StopPlayingAfterCurrentTrackAction::engineStateChanged( Phonon::State state,  Phonon::State /*oldState*/ )
{
    switch( state ) {
        case Phonon::PlayingState:
        case Phonon::PausedState:
        case Phonon::StoppedState:
            setEnabled( true );
            break;
        case Phonon::LoadingState:
            setDisabled( true );
            break;
        case Phonon::ErrorState:
        case Phonon::BufferingState:
            break;
    }
}

#include "ActionClasses.moc"

