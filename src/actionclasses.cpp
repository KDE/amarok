// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution

#include "config.h"             //HAVE_XMMS definition

#include "actionclasses.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "debug.h"
#include "collectiondb.h"
#include "covermanager.h"
#include "enginecontroller.h"
#include "k3bexporter.h"
#include "playlistwindow.h"
#include "playlist.h"
#include "socketserver.h"       //Vis::Selector::showInstance()

#include <qpixmap.h>
#include <qtooltip.h>

#include <kaction.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kurl.h>

using namespace amaroK;

KHelpMenu *Menu::s_helpMenu = 0;


static void
safePlug( KActionCollection *ac, const char *name, QWidget *w )
{
    if( ac )
    {
        KAction *a = ac->action( name );
        if( a ) a->plug( w );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// MenuAction && Menu
// KActionMenu doesn't work very well, so we derived our own
//////////////////////////////////////////////////////////////////////////////////////////

MenuAction::MenuAction( KActionCollection *ac )
  : KAction( i18n( "amaroK Menu" ), 0, ac, "amarok_menu" )
{
    setShortcutConfigurable ( false ); //FIXME disabled as it doesn't work, should use QCursor::pos()
}

int
MenuAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( bar, id );
        connect( bar, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        //TODO create menu on demand
        //TODO create menu above and aligned within window
        //TODO make the arrow point upwards!
        bar->insertButton( QString::null, id, true, i18n( "Menu" ), index );
        bar->alignItemRight( id );

        KToolBarButton* button = bar->getButton( id );
        button->setPopup( amaroK::Menu::instance() );
        button->setName( "toolbutton_amarok_menu" );
        button->setIcon( "amarok" );

        return containerCount() - 1;
    }
    else return -1;
}

Menu::Menu()
{
    KActionCollection *ac = amaroK::actionCollection();

    setCheckable( true );

    safePlug( ac, "repeat_track", this );
    safePlug( ac, "repeat_playlist", this );
    safePlug( ac, "random_mode", this );
    safePlug( ac, "dynamic_mode", this );

    insertSeparator();

    insertItem( SmallIconSet( "covermanager" ), i18n( "C&over Manager" ), ID_SHOW_COVER_MANAGER );
    safePlug( ac, "queue_manager", this );
    insertItem( SmallIconSet( "visualizations"), i18n( "&Visualizations" ), ID_SHOW_VIS_SELECTOR );
    insertItem( SmallIconSet( "equalizer"), i18n( "E&qualizer" ), kapp, SLOT( slotConfigEqualizer() ), 0, ID_CONFIGURE_EQUALIZER );
    safePlug( ac, "script_manager", this );

    insertSeparator();

    insertItem( SmallIconSet( "wizard" ), i18n( "First-Run &Wizard" ), ID_SHOW_WIZARD );
    insertItem( i18n("&Rescan Collection"), ID_RESCAN_COLLECTION );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::ShowMenubar), this );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::ConfigureToolbars), this );
    safePlug( ac, KStdAction::name(KStdAction::KeyBindings), this );
    safePlug( ac, "options_configure_globals", this ); //we created this one
    safePlug( ac, KStdAction::name(KStdAction::Preferences), this );

    insertSeparator();

    insertItem( SmallIconSet("help"), i18n( "&Help" ), helpMenu( this ) );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::Quit), this );

    connect( this, SIGNAL( aboutToShow() ),  SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );

    setItemEnabled( ID_SHOW_VIS_SELECTOR, false );
    #ifdef HAVE_XMMS
    setItemEnabled( ID_SHOW_VIS_SELECTOR, true );
    #endif
    #ifdef HAVE_LIBVISUAL
    setItemEnabled( ID_SHOW_VIS_SELECTOR, true );
    #endif
}

Menu*
Menu::instance()
{
    static Menu menu;
    return &menu;
}

KPopupMenu*
Menu::helpMenu( QWidget *parent ) //STATIC
{
    extern KAboutData aboutData;

    if ( s_helpMenu == 0 )
        s_helpMenu = new KHelpMenu( parent, &aboutData, amaroK::actionCollection() );
    return s_helpMenu->menu();

    return (new KHelpMenu( parent, &aboutData, amaroK::actionCollection() ))->menu();
}

void
Menu::slotAboutToShow()
{
    setItemEnabled( ID_CONFIGURE_EQUALIZER, EngineController::hasEngineProperty( "HasEqualizer" ) );
    setItemEnabled( ID_CONF_DECODER, EngineController::hasEngineProperty( "HasConfigure" ) );
}

void
Menu::slotActivated( int index )
{
    switch( index )
    {
    case ID_SHOW_COVER_MANAGER:
        CoverManager::showOnce();
        break;
    case ID_SHOW_WIZARD:
        pApp->firstRunWizard();
        pApp->playlistWindow()->recreateGUI();
        pApp->applySettings();
        break;
    case ID_SHOW_VIS_SELECTOR:
        Vis::Selector::instance()->show(); //doing it here means we delay creation of the widget
        break;
    case ID_RESCAN_COLLECTION:
        CollectionDB::instance()->startScan();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PlayPauseAction
//////////////////////////////////////////////////////////////////////////////////////////

PlayPauseAction::PlayPauseAction( KActionCollection *ac )
        : KToggleAction( i18n( "Play/Pause" ), 0, ac, "play_pause" )
        , EngineObserver( EngineController::instance() )
{
    engineStateChanged( EngineController::engine()->state() );

    connect( this, SIGNAL(activated()), EngineController::instance(), SLOT(playPause()) );
}

void
PlayPauseAction::engineStateChanged( Engine::State state,  Engine::State /*oldState*/ )
{
    QString text;

    switch( state ) {
    case Engine::Playing:
        setChecked( false );
        setIcon( "player_pause" );
        text = i18n( "Pause" );
        break;
    case Engine::Paused:
        setChecked( true );
        setIcon( "player_pause" );
        text = i18n( "Pause" );
        break;
    case Engine::Empty:
        setChecked( false );
        setIcon( "player_play" );
        text = i18n( "Play" );
        break;
    case Engine::Idle:
        return;
    }

    //update menu texts for this special action
    for( int x = 0; x < containerCount(); ++x ) {
        QWidget *w = container( x );
        if( w->inherits( "QPopupMenu" ) )
            static_cast<QPopupMenu*>(w)->changeItem( itemId( x ), text );
        //TODO KToolBar sucks so much
//         else if( w->inherits( "KToolBar" ) )
//             static_cast<KToolBar*>(w)->getButton( itemId( x ) )->setText( text );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// AnalyzerAction
//////////////////////////////////////////////////////////////////////////////////////////
#include "analyzerbase.h"

AnalyzerAction::AnalyzerAction( KActionCollection *ac )
        : KAction( i18n( "Analyzer" ), 0, ac, "toolbar_analyzer" )
{
    setShortcutConfigurable( false );
}

int
AnalyzerAction::plug( QWidget *w, int index )
{
    //NOTE the analyzer will be deleted when the toolbar is deleted or cleared()
    //we are not designed for unplugging() yet so there would be a leak if that happens
    //but it's a rare event and unplugging is complicated.

    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( w, id );
        connect( w, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );
        QWidget *container = new AnalyzerContainer( w );
        bar->insertWidget( id, 0, container, index );
        bar->setItemAutoSized( id, true );

        return containerCount() - 1;
    }
    else return -1;
}


AnalyzerContainer::AnalyzerContainer( QWidget *parent )
        : QWidget( parent, "AnalyzerContainer" )
        , m_child( 0 )
{
    QToolTip::add( this, i18n( "Click for more analyzers" ) );
    changeAnalyzer();
}

void
AnalyzerContainer::resizeEvent( QResizeEvent *)
{
    m_child->resize( size() );
}

void AnalyzerContainer::changeAnalyzer()
{
    delete m_child;
    m_child = Analyzer::Factory::createPlaylistAnalyzer( this );
    m_child->setName( "ToolBarAnalyzer" );
    m_child->resize( size() );
    m_child->show();
}

void
AnalyzerContainer::mousePressEvent( QMouseEvent *e)
{
    if( e->button() == Qt::LeftButton ) {
        AmarokConfig::setCurrentPlaylistAnalyzer( AmarokConfig::currentPlaylistAnalyzer() + 1 );
        changeAnalyzer();
    }
    else if( e->button() == Qt::RightButton ) {
        #if defined HAVE_XMMS || defined HAVE_LIBVISUAL
        KPopupMenu menu;
        menu.insertItem( SmallIconSet( "visualizations" ), i18n("&Visualizations"), Menu::ID_SHOW_VIS_SELECTOR );

        if( menu.exec( mapToGlobal( e->pos() ) ) == Menu::ID_SHOW_VIS_SELECTOR )
            Menu::instance()->slotActivated( Menu::ID_SHOW_VIS_SELECTOR );
        #endif
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// VolumeAction
//////////////////////////////////////////////////////////////////////////////////////////

VolumeAction::VolumeAction( KActionCollection *ac )
        : KAction( i18n( "Volume" ), 0, ac, "toolbar_volume" )
        , EngineObserver( EngineController::instance() )
        , m_slider( 0 ) //is QGuardedPtr
{}

int
VolumeAction::plug( QWidget *w, int index )
{
    //NOTE we only support one plugging currently

    delete (amaroK::VolumeSlider*) m_slider; //just in case, remember, we only support one plugging!

    m_slider = new amaroK::VolumeSlider( w, amaroK::VOLUME_MAX );
    m_slider->setName( "ToolBarVolume" );
    m_slider->setValue( AmarokConfig::masterVolume() );
    m_slider->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Ignored );

    QToolTip::add( m_slider, i18n( "Volume control" ) );

    EngineController* const ec = EngineController::instance();
    connect( m_slider, SIGNAL(sliderMoved( int )), ec, SLOT(setVolume( int )) );
    connect( m_slider, SIGNAL(sliderReleased( int )), ec, SLOT(setVolume( int )) );

    static_cast<KToolBar*>(w)->insertWidget( KAction::getToolButtonID(), 0, m_slider, index );

    return 0;
}

void
VolumeAction::engineVolumeChanged( int value )
{
    if( m_slider ) m_slider->setValue( value );
}



//////////////////////////////////////////////////////////////////////////////////////////
// RandomAction
//////////////////////////////////////////////////////////////////////////////////////////
RandomAction::RandomAction( KActionCollection *ac ) :
    ToggleAction( i18n( "Random &Mode" ), &AmarokConfig::setRandomMode, ac, "random_mode" )
{
    KToggleAction::setChecked( AmarokConfig::randomMode() );
    setIcon( "random" );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatTrackAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatTrackAction::RepeatTrackAction( KActionCollection *ac ) :
    ToggleAction( i18n( "Repeat &Track" ), &AmarokConfig::setRepeatTrack, ac, "repeat_track" )
{
    KToggleAction::setChecked( AmarokConfig::repeatTrack() );
    setIcon( "repeat_track" );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatPlaylistAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatPlaylistAction::RepeatPlaylistAction( KActionCollection *ac ) :
    ToggleAction( i18n( "R&epeat Playlist" ), &AmarokConfig::setRepeatPlaylist, ac, "repeat_playlist" )
{
    KToggleAction::setChecked( AmarokConfig::repeatPlaylist() );
    setIcon( "repeat_playlist" );
}

//////////////////////////////////////////////////////////////////////////////////////////
// PartyAction
//////////////////////////////////////////////////////////////////////////////////////////
DynamicAction::DynamicAction( KActionCollection *ac ) :
    ToggleAction( i18n( "&Dynamic Mode" ), &AmarokConfig::setDynamicMode, ac, "dynamic_mode" )
{
    KToggleAction::setChecked( AmarokConfig::dynamicMode() );
    setIcon( "dynamic" );
}

//////////////////////////////////////////////////////////////////////////////////////////
// BurnMenuAction
//////////////////////////////////////////////////////////////////////////////////////////
BurnMenuAction::BurnMenuAction( KActionCollection *ac )
  : KAction( i18n( "Burn" ), 0, ac, "burn_menu" )
{
}

int
BurnMenuAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( bar, id );
        connect( bar, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        bar->insertButton( QString::null, id, true, i18n( "Burn" ), index );

        KToolBarButton* button = bar->getButton( id );
        button->setPopup( amaroK::BurnMenu::instance() );
        button->setName( "toolbutton_burn_menu" );
        button->setIcon( "k3b" );

        return containerCount() - 1;
    }
    else return -1;
}

BurnMenu::BurnMenu()
{
    insertItem( i18n("Current Playlist"), CURRENT_PLAYLIST );
    insertItem( i18n("Selected Tracks"), SELECTED_TRACKS );
    //TODO add "album" and "all tracks by artist"

    connect( this, SIGNAL( aboutToShow() ),  SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );
}

KPopupMenu*
BurnMenu::instance()
{
    static BurnMenu menu;
    return &menu;
}

void
BurnMenu::slotAboutToShow()
{

}

void
BurnMenu::slotActivated( int index )
{
    switch( index )
    {
    case CURRENT_PLAYLIST:
        K3bExporter::instance()->exportCurrentPlaylist();
        break;

    case SELECTED_TRACKS:
        K3bExporter::instance()->exportSelectedTracks();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// StopMenuAction
//////////////////////////////////////////////////////////////////////////////////////////

StopAction::StopAction( KActionCollection *ac )
  : KAction( i18n( "Stop" ), "player_stop", 0, EngineController::instance(), SLOT( stop() ), ac, "stop" )
{}

int
StopAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( bar, id );
        connect( bar, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        bar->insertButton( QString::null, id, SIGNAL( clicked() ), EngineController::instance(), SLOT( stop() ),
                           true, i18n( "Stop" ), index );

        KToolBarButton* button = bar->getButton( id );
        button->setDelayedPopup( amaroK::StopMenu::instance() );
        button->setName( "toolbutton_stop_menu" );
        button->setIcon( "player_stop" );
        button->setEnabled( false ); // Required, otherwise the button is always enabled at startup

        return containerCount() - 1;
    }
    else return KAction::plug( w, index );
}

StopMenu::StopMenu()
{
    insertTitle( i18n( "Stop" ) );
    insertItem( i18n("Now"), NOW );
    insertItem( i18n("After Current Track"), AFTER_TRACK );
    insertItem( i18n("After Queue"), AFTER_QUEUE );

    connect( this, SIGNAL( aboutToShow() ),  SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );
}

KPopupMenu*
StopMenu::instance()
{
    static StopMenu menu;
    return &menu;
}

void
StopMenu::slotAboutToShow()
{
    Playlist *pl = Playlist::instance();

    setItemEnabled( NOW,         amaroK::actionCollection()->action( "stop" )->isEnabled() );

    setItemEnabled( AFTER_TRACK, pl->currentTrackIndex() >= 0 );
    setItemChecked( AFTER_TRACK, pl->stopAfterMode() == Playlist::StopAfterCurrent );

    setItemEnabled( AFTER_QUEUE, pl->nextTracksCount() );
    setItemChecked( AFTER_QUEUE, pl->stopAfterMode() == Playlist::StopAfterQueue );
}

void
StopMenu::slotActivated( int index )
{
    Playlist* pl = Playlist::instance();
    const int mode = pl->stopAfterMode();

    switch( index )
    {
        case NOW:
            amaroK::actionCollection()->action( "stop" )->activate();
            if( mode == Playlist::StopAfterCurrent || mode == Playlist::StopAfterQueue )
                pl->setStopAfterMode( Playlist::DoNotStop );
            break;
        case AFTER_TRACK:
            pl->setStopAfterMode( mode == Playlist::StopAfterCurrent
                                ? Playlist::DoNotStop
                                : Playlist::StopAfterCurrent );
            break;
        case AFTER_QUEUE:
            pl->setStopAfterMode( mode == Playlist::StopAfterQueue
                                ? Playlist::DoNotStop
                                : Playlist::StopAfterQueue );
            break;
    }
}


#include "actionclasses.moc"
