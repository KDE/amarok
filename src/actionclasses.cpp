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

    insertSeparator();

    safePlug( ac, "playlist_playmedia", this );
    safePlug( ac, "play_audiocd", this );
    safePlug( ac, "lastfm_play", this );

    insertSeparator();

    insertItem( KIcon( Amarok::icon( "covermanager" ) ), i18n( "C&over Manager" ), ID_SHOW_COVER_MANAGER );
    safePlug( ac, "queue_manager", this );
    insertItem( KIcon( Amarok::icon( "visualizations" ) ), i18n( "&Visualizations" ), ID_SHOW_VIS_SELECTOR );
    insertItem( KIcon( Amarok::icon( "equalizer" ) ), i18n( "E&qualizer" ), kapp, SLOT( slotConfigEqualizer() ), 0, ID_CONFIGURE_EQUALIZER );
    safePlug( ac, "script_manager", this );
    safePlug( ac, "statistics", this );

    insertSeparator();


    safePlug( ac, "update_collection", this );
    insertItem( KIcon( Amarok::icon( "rescan" ) ), i18n("&Rescan Collection"), ID_RESCAN_COLLECTION );
    setItemEnabled( ID_RESCAN_COLLECTION, !ThreadManager::instance()->isJobPending( "CollectionScanner" ) );

#ifndef Q_WS_MAC
    insertSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::ShowMenubar), this );
#endif

    insertSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::ConfigureToolbars), this );
    safePlug( ac, KStandardAction::name(KStandardAction::KeyBindings), this );
//    safePlug( ac, "options_configure_globals", this ); //we created this one
    safePlug( ac, KStandardAction::name(KStandardAction::Preferences), this );

    insertSeparator();

    insertItem( KIcon("help"), i18n( "&Help" ), helpMenu( this ) );

    insertSeparator();

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
// AnalyzerAction
//////////////////////////////////////////////////////////////////////////////////////////
#include "analyzerbase.h"

AnalyzerAction::AnalyzerAction( KActionCollection *ac )
        : KAction(0 )
{
    setText(i18n( "Analyzer" ));
    ac->addAction("toolbar_analyzer", this);
    setShortcutConfigurable( false );
}

QWidget*
AnalyzerAction::createWidget( QWidget *w )
{
	QWidget* container = new AnalyzerContainer( w );
    container->setFixedWidth( 300 );

    return container;
}


AnalyzerContainer::AnalyzerContainer( QWidget *parent )
        : QWidget( parent )
        , m_child( 0 )
{
    setObjectName(  "AnalyzerContainer" );
    this->setToolTip( i18n( "Click for more analyzers" ) );
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
    m_child->setObjectName( "ToolBarAnalyzer" );
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
}

void
AnalyzerContainer::contextMenuEvent( QContextMenuEvent *e)
{
#if defined HAVE_LIBVISUAL
    KMenu menu;
    menu.addItem( KIcon( Amarok::icon( "visualizations" ) ), i18n("&Visualizations"), Menu::ID_SHOW_VIS_SELECTOR );

    if( menu.exec( mapToGlobal( e->pos() ) ) == Menu::ID_SHOW_VIS_SELECTOR )
        Menu::instance()->slotActivated( Menu::ID_SHOW_VIS_SELECTOR );
#else
    Q_UNUSED(e);
#endif
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
// VolumeAction
//////////////////////////////////////////////////////////////////////////////////////////

VolumeAction::VolumeAction( KActionCollection *ac )
        : KAction( 0 )
        , EngineObserver( EngineController::instance() )
        , m_slider( 0 ) //is QPointer
{
    setText(i18n( "Volume" ));
    ac->addAction("toolbar_volume", this);
}

QWidget*
VolumeAction::createWidget( QWidget *w )
{
    //NOTE we only support one plugging currently

    delete static_cast<Amarok::VolumeSlider*>( m_slider ); //just in case, remember, we only support one plugging!

    m_slider = new Amarok::VolumeSlider( w, Amarok::VOLUME_MAX );
    m_slider->setObjectName( "ToolBarVolume" );
    m_slider->setValue( AmarokConfig::masterVolume() );
    m_slider->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Ignored );

    m_slider->setToolTip( i18n( "Volume control" ) );

    EngineController* const ec = EngineController::instance();
    connect( m_slider, SIGNAL(sliderMoved( int )), ec, SLOT(setVolume( int )) );
    connect( m_slider, SIGNAL(sliderReleased( int )), ec, SLOT(setVolume( int )) );

    return m_slider;
}

void
VolumeAction::engineVolumeChanged( int value )
{
    if( m_slider ) m_slider->setValue( value );
}



//////////////////////////////////////////////////////////////////////////////////////////
// SearchAction
//////////////////////////////////////////////////////////////////////////////////////////

SearchAction::SearchAction( KActionCollection *ac, QWidget *caller )
    : KAction( 0 ),
    m_searchWidget( 0 )
{
    setText( i18n( "Search Bar" ) );
    m_caller = caller;
    ac->addAction( "search_bar", this );
}


QWidget *SearchAction::createWidget( QWidget *w )
{
    KHBox *searchBox = new KHBox( w );
    searchBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );

    m_searchWidget = new KLineEdit( searchBox );
    m_searchWidget->setClickMessage( i18n( "Enter search terms here" ) );
    m_searchWidget->setClearButtonShown( true );
    m_searchWidget->setFrame( QFrame::Sunken );
    m_searchWidget->setToolTip( i18n(
        "Enter space-separated terms to search in the playlist." ) );

    KPushButton *filterButton = new KPushButton( "...", searchBox );
    filterButton->setFlat( true ); //TODO: maybe?
    filterButton->setObjectName( "filter" );
    filterButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    filterButton->setToolTip( i18n( "Click to edit playlist filter" ) );

    connect( filterButton, SIGNAL( clicked() ), m_caller,
            SLOT(slotEditFilter() ) );
    connect( m_searchWidget, SIGNAL( textChanged( const QString & ) ), m_caller,
            SLOT( slotSetFilterTimeout() ) );

    return searchBox;
}

//////////////////////////////////////////////////////////////////////////////////////////
// SliderAction
//////////////////////////////////////////////////////////////////////////////////////////
SliderAction *SliderAction::s_instance = 0;
SliderAction::SliderAction( KActionCollection *ac )
    : KAction( 0 ),
      EngineObserver( EngineController::instance() ),
      m_positionBox( 0 )
{
    s_instance = this;
    setText( i18n( "Progress Slider" ) );
    ac->addAction( "progress_bar", this );
}

QWidget *SliderAction::createWidget( QWidget *w )
{
    m_positionBox = new QWidget( w );
    m_positionBox->setObjectName( "positionBox" );
    QHBoxLayout *box = new QHBoxLayout( m_positionBox );
    m_positionBox->setLayout( box );
    box->setMargin( 1 );
    box->setSpacing( 3 );

    m_slider = new Amarok::PrettySlider(
            Qt::Horizontal, Amarok::PrettySlider::Normal, m_positionBox, 1000 );

    // the two time labels. m_timeLabel is the left one,
    // m_timeLabel2 the right one.
    m_timeLabel = new TimeLabel( m_positionBox );
    m_timeLabel->setToolTip( i18n( "The amount of time elapsed in current song" ) );
    m_slider->setMinimumWidth( m_timeLabel->width() );

    m_timeLabel2 = new TimeLabel( m_positionBox );
    m_timeLabel->setToolTip( i18n( "The amount of time remaining in current song" ) );
    m_slider->setMinimumWidth( m_timeLabel2->width() );

    box->addSpacing( 3 );
    box->addWidget( m_timeLabel );
    box->addWidget( m_slider );
    box->addWidget( m_timeLabel2 );
#ifdef Q_WS_MAC
    // don't overlap the resize handle with the time display
    box->addSpacing( 12 );
#endif

    if( !AmarokConfig::leftTimeDisplayEnabled() )
        m_timeLabel->hide();

    engineStateChanged( Engine::Empty );

    connect( m_slider, SIGNAL(sliderReleased( int )), EngineController::instance(), SLOT(seek( int )) );
    connect( m_slider, SIGNAL(valueChanged( int )), SLOT(drawTimeDisplay( int )) );

    return m_positionBox;
}

void SliderAction::drawTimeDisplay( int ms )  //SLOT
{
    int seconds = ms / 1000;
    int seconds2 = seconds; // for the second label.
    const uint trackLength = EngineController::instance()->bundle().length();

    if( AmarokConfig::leftTimeDisplayEnabled() )
        m_timeLabel->show();
    else
        m_timeLabel->hide();

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = seconds;
        seconds = trackLength - seconds;
    // when the left label shows the remaining time and it's a stream
    } else if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = seconds;
        seconds = 0; // for streams
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = trackLength - seconds;
    // when the right label shows the remaining time and it's a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = 0;
    }

    QString s1 = MetaBundle::prettyTime( seconds );
    QString s2 = MetaBundle::prettyTime( seconds2 );

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 ) {
        s1.prepend( '-' );
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        s2.prepend( '-' );
    }

    while( (int)s1.length() < m_timeLength )
        s1.prepend( ' ' );

    while( (int)s2.length() < m_timeLength )
        s2.prepend( ' ' );

    s1 += ' ';
    s2 += ' ';

    m_timeLabel->setText( s1 );
    m_timeLabel2->setText( s2 );

    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabel->setEnabled( false );
        m_timeLabel2->setEnabled( true );
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( false );
    } else
    {
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( true );
    }
}

void SliderAction::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
    m_slider->setValue( position );

    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}

void
SliderAction::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{

    switch ( state ) {
    case Engine::Empty:
        m_slider->setEnabled( false );
        m_slider->setMinimum( 0 ); //needed because setMaximum() calls with bogus values can change minValue
        m_slider->setMaximum( 0 );
    m_slider->newBundle( MetaBundle() ); // Set an empty bundle
        m_timeLabel->setEnabled( false ); //must be done after the setValue() above, due to a signal connection
        m_timeLabel2->setEnabled( false );
        break;

    case Engine::Playing:
        DEBUG_LINE_INFO
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( true );
        break;

    case Engine::Idle:
        ; //just do nothing, idle is temporary and a limbo state
    }
}
void SliderAction::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    m_slider->newBundle( bundle );
    engineTrackLengthChanged( bundle.length() );
}

void SliderAction::engineTrackLengthChanged( long length )
{
    m_slider->setMinimum( 0 );
    m_slider->setMaximum( length * 1000 );
    m_slider->setEnabled( length > 0 );
    m_timeLength = MetaBundle::prettyTime( length ).length()+1; // account for - in remaining time
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
