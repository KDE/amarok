// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution

#include "config.h"             //HAVE_LIBVISUAL definition

#include "actionclasses.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "debug.h"
#include "collectiondb.h"
#include "covermanager.h"
#include "enginecontroller.h"
#include "k3bexporter.h"
#include "mediumpluginmanager.h"
#include "playlistwindow.h"
#include "playlist.h"
#include "socketserver.h"       //Vis::Selector::showInstance()
#include "statusbar.h"
#include "threadmanager.h"
#include "timeLabel.h"

#include <khbox.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <kauthorized.h>

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
MenuAction::plug( QWidget *w, int index )
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

    setCheckable( true );

    safePlug( ac, "repeat", this );
    safePlug( ac, "random_mode", this );

    addSeparator();

    safePlug( ac, "playlist_playmedia", this );
    safePlug( ac, "play_audiocd", this );
    safePlug( ac, "lastfm_play", this );

    addSeparator();

    insertItem( KIcon( Amarok::icon( "covermanager" ) ), i18n( "C&over Manager" ), ID_SHOW_COVER_MANAGER );
    safePlug( ac, "queue_manager", this );
    insertItem( KIcon( Amarok::icon( "visualizations" ) ), i18n( "&Visualizations" ), ID_SHOW_VIS_SELECTOR );
    insertItem( KIcon( Amarok::icon( "equalizer" ) ), i18n( "E&qualizer" ), kapp, SLOT( slotConfigEqualizer() ), 0, ID_CONFIGURE_EQUALIZER );
    safePlug( ac, "script_manager", this );
    safePlug( ac, "statistics", this );

    addSeparator();


    safePlug( ac, "update_collection", this );
    insertItem( KIcon( Amarok::icon( "rescan" ) ), i18n("&Rescan Collection"), ID_RESCAN_COLLECTION );
    setItemEnabled( ID_RESCAN_COLLECTION, !ThreadManager::instance()->isJobPending( "CollectionScanner" ) );

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

    insertItem( KIcon("help"), i18n( "&Help" ), helpMenu( this ) );

    addSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::Quit), this );

    connect( this, SIGNAL( aboutToShow() ),  SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );

    setItemEnabled( ID_SHOW_VIS_SELECTOR, false );
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

KMenu*
Menu::helpMenu( QWidget *parent ) //STATIC
{
    extern KAboutData aboutData;

    if ( s_helpMenu == 0 )
        s_helpMenu = new KHelpMenu( parent, &aboutData, Amarok::actionCollection() );
    return s_helpMenu->menu();

    return (new KHelpMenu( parent, &aboutData, Amarok::actionCollection() ))->menu();
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
/*
    //update menu texts for this special action
    for( int x = 0; x < associatedWidgets().count(); ++x ) {
        QWidget *w = associatedWidgets().value( x );
        if( w->inherits( "QPopupMenu" ) )
            static_cast<Q3PopupMenu*>(w)->changeItem( *this, text );
        //TODO KToolBar sucks so much
//         else if( w->inherits( "KToolBar" ) )
//             static_cast<KToolBar*>(w)->getButton( itemId( x ) )->setText( text );
    }*/
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
    AmarokConfig::writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit toggled( b ); //KToggleAction doesn't do this for us. How gay!
}

void ToggleAction::setEnabled( bool b )
{
    const bool announce = b != isEnabled();

    if( !b )
        setChecked( false );
    KToggleAction::setEnabled( b );
    AmarokConfig::writeConfig(); //So we don't lose the setting when crashing
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
    AmarokConfig::writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit triggered( n );
}

void SelectAction::setEnabled( bool b )
{
    const bool announce = b != isEnabled();

    if( !b )
        setCurrentItem( 0 );
    KSelectAction::setEnabled( b );
    AmarokConfig::writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit QAction::triggered( b );
}

void SelectAction::setIcons( QStringList icons )
{
    m_icons = icons;
    for( int i = 0, n = items().count(); i < n; ++i )
        menu()->changeItem( i, KIconLoader::global()->loadIconSet( icons.at( i ), K3Icon::Small ), menu()->text( i ) );
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

    if( bar && KAuthorized::authorizeKAction( name() ) )
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
    insertItem( i18n("Current Playlist"), CURRENT_PLAYLIST );
    insertItem( i18n("Selected Tracks"), SELECTED_TRACKS );
    //TODO add "album" and "all tracks by artist"

    connect( this, SIGNAL( aboutToShow() ),  SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );
}

KMenu*
BurnMenu::instance()
{
    static BurnMenu menu;
    return &menu;
}

void
BurnMenu::slotAboutToShow()
{}

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
  : KAction( 0 )
{
    setText(i18n( "Stop" ));
    setIcon(KIcon(Amarok::icon( "stop" )));
    connect( this, SIGNAL( triggered() ), EngineController::instance(), SLOT( stop() ) );
    ac->addAction("stop", this);
}

int
StopAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);
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
    insertItem( i18n("Now"), NOW );
    insertItem( i18n("After Current Track"), AFTER_TRACK );
    insertItem( i18n("After Queue"), AFTER_QUEUE );

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
    Playlist *pl = Playlist::instance();

    setItemEnabled( NOW,         Amarok::actionCollection()->action( "stop" )->isEnabled() );

    setItemEnabled( AFTER_TRACK, EngineController::engine()->loaded() );
    setItemChecked( AFTER_TRACK, pl->stopAfterMode() == Playlist::StopAfterCurrent );

    setItemEnabled( AFTER_QUEUE, pl->nextTracks().count() );
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
            Amarok::actionCollection()->action( "stop" )->trigger();
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
