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

#define DEBUG_PREFIX "ActionClasses"

#include "ActionClasses.h"


#include "App.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "amarokconfig.h"
#include <config.h>
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "widgets/Osd.h"
#include "KNotificationBackend.h"

#include <KAuthorized>
#include <KHelpMenu>
#include <KLocale>
#include <KToolBar>

extern OcsData ocsData;

namespace Amarok
{
    bool favorNone()      { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::Off; }
    bool favorScores()    { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::HigherScores; }
    bool favorRatings()   { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::HigherRatings; }
    bool favorLastPlay()  { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::LessRecentlyPlayed; }


    bool entireAlbums()   { return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatAlbum ||
                                   AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomAlbum; }

    bool repeatEnabled()  { return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatTrack ||
                                   AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatAlbum  ||
                                   AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist; }

    bool randomEnabled()  { return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomTrack ||
                                   AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomAlbum; }
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
        s_helpMenu = new KHelpMenu( parent, KGlobal::mainComponent().aboutData(), Amarok::actionCollection() );

    KMenu* menu = s_helpMenu->menu();

    // "What's This" isn't currently defined for anything in Amarok, so let's remove it
    s_helpMenu->action( KHelpMenu::menuWhatsThis )->setVisible( false );

    // Hide the default "About App" dialog, as we replace it with a custom one
    s_helpMenu->action( KHelpMenu::menuAboutApp )->setVisible( false );

    return menu;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PlayPauseAction
//////////////////////////////////////////////////////////////////////////////////////////

PlayPauseAction::PlayPauseAction( KActionCollection *ac, QObject *parent )
        : KToggleAction( parent )
{

    ac->addAction( "play_pause", this );
    setText( i18n( "Play/Pause" ) );
    setShortcut( Qt::Key_Space );
    setGlobalShortcut( KShortcut() );

    EngineController *engine = The::engineController();

    if( engine->isPaused() )
        paused();
    else if( engine->isPlaying() )
        playing();
    else
        stopped();

    connect( this, SIGNAL(triggered()),
             engine, SLOT(playPause()) );

    connect( engine, SIGNAL(stopped(qint64,qint64)),
             this, SLOT(stopped()) );
    connect( engine, SIGNAL(paused()),
             this, SLOT(paused()) );
    connect( engine, SIGNAL(trackPlaying(Meta::TrackPtr)),
             this, SLOT(playing()) );
}

void
PlayPauseAction::stopped()
{
    setChecked( false );
    setIcon( KIcon("media-playback-start-amarok") );
}

void
PlayPauseAction::paused()
{
    setChecked( true );
    setIcon( KIcon("media-playback-start-amarok") );
}

void
PlayPauseAction::playing()
{
    setChecked( false );
    setIcon( KIcon("media-playback-pause-amarok") );
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


void
RandomAction::setCurrentItem( int n )
{
    // Porting
    //if( KAction *a = parentCollection()->action( "favor_tracks" ) )
    //    a->setEnabled( n );
    SelectAction::setCurrentItem( n );
}

//////////////////////////////////////////////////////////////////////////////////////////
// ReplayGainModeAction
//////////////////////////////////////////////////////////////////////////////////////////
ReplayGainModeAction::ReplayGainModeAction( KActionCollection *ac, QObject *parent ) :
    SelectAction( i18n( "&Replay Gain Mode" ), &AmarokConfig::setReplayGainMode, ac, "replay_gain_mode", parent )
{
    setItems( QStringList() << i18nc( "Replay Gain state, as in, disabled", "&Off" )
                            << i18nc( "Item, as in, music", "&Track" )
                            << i18n( "&Album" ) );
    EngineController *engine = EngineController::instance();
    Q_ASSERT( engine );
    if( engine->supportsGainAdjustments() )
        setCurrentItem( AmarokConfig::replayGainMode() );
    else
    {
        // Note: it would be nice to set a tooltip that would explain why this is disabled
        // to users, but tooltips aren't shown in meny anyway :-(
        actions().at( 1 )->setEnabled( false );
        actions().at( 2 )->setEnabled( false );
    }
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
        connect( bar, SIGNAL(destroyed()), SLOT(slotDestroyed()) );

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

    addAction( i18n("Current Playlist"), this, SLOT(slotBurnCurrentPlaylist()) );
    addAction( i18n("Selected Tracks"), this, SLOT(slotBurnSelectedTracks()) );
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
{
    ac->addAction( "stop", this );
    setText( i18n( "Stop" ) );
    setIcon( KIcon("media-playback-stop-amarok") );
    setGlobalShortcut( KShortcut() );
    connect( this, SIGNAL(triggered()), this, SLOT(stop()) );

    EngineController *engine = The::engineController();

    if( engine->isStopped() )
        stopped();
    else
        playing();

    connect( engine, SIGNAL(stopped(qint64,qint64)),
             this, SLOT(stopped()) );
    connect( engine, SIGNAL(trackPlaying(Meta::TrackPtr)),
             this, SLOT(playing()) );

}

void
StopAction::stopped()
{
    setEnabled( false );
}

void
StopAction::playing()
{
    setEnabled( true );
}

void
StopAction::stop()
{
    The::engineController()->stop();
}


//////////////////////////////////////////////////////////////////////////////////////////
// StopPlayingAfterCurrentTrackAction
//////////////////////////////////////////////////////////////////////////////////////////

StopPlayingAfterCurrentTrackAction::StopPlayingAfterCurrentTrackAction( KActionCollection *ac, QObject *parent )
: KAction( parent )
{
    ac->addAction( "stop_after_current", this );
    setText( i18n( "Stop after current Track" ) );
    setIcon( KIcon("media-playback-stop-amarok") );
    setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_V ) );
    connect( this, SIGNAL(triggered()), SLOT(stopPlayingAfterCurrentTrack()) );
}

void
StopPlayingAfterCurrentTrackAction::stopPlayingAfterCurrentTrack()
{
    QString text;

    quint64 activeTrack = Playlist::ModelStack::instance()->bottom()->activeId();
    if( activeTrack )
    {
        if( The::playlistActions()->willStopAfterTrack( activeTrack ) )
        {
            The::playlistActions()->stopAfterPlayingTrack( 0 );
            text = i18n( "Stop after current track: Off" );
        }
        else
        {
            The::playlistActions()->stopAfterPlayingTrack( activeTrack );
            text = i18n( "Stop after current track: On" );
        }
    }
    else
        text = i18n( "No track playing" );

    Amarok::OSD::instance()->OSDWidget::show( text );
    if( Amarok::KNotificationBackend::instance()->isEnabled() )
        Amarok::KNotificationBackend::instance()->show( i18n( "Amarok" ), text );
}
