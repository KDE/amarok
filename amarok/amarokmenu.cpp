// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution

#include "amarokconfig.h"
#include "amarokmenu.h"
#include "playerapp.h"
#include "enginecontroller.h"

#include <kaction.h>
#include <kapplication.h>
#include <khelpmenu.h>
#include <klocale.h>


//TODO get kde-admin to rename this file amarokactions.cpp!


using namespace amaroK;


//static member
KHelpMenu *Menu::HelpMenu = 0;


static void
safePlug( KActionCollection *ac, const char *name, QWidget *w )
{
    if( ac )
    {
        KAction *a = ac->action( name );
        if( a ) a->plug( w );
    }
}


Menu::Menu( QWidget *parent )
  : KPopupMenu( parent )
{
    KActionCollection *ac = pApp->actionCollection();

    setCheckable( true );

    safePlug( ac, "repeat_track", this );
    safePlug( ac, "repeat_playlist", this );
    safePlug( ac, "random_mode", this );

    insertSeparator();
    
    insertItem( i18n( "&Visualizations" ), pApp, SIGNAL( showVisSelector() ) );

    insertSeparator();

    insertItem( i18n( "Configure &Effects..." ), pApp, SLOT( showEffectWidget() ) );
    insertItem( i18n( "Configure &Decoder..." ), ID_CONF_DECODER );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::ConfigureToolbars), this );
    safePlug( ac, KStdAction::name(KStdAction::KeyBindings), this );
    safePlug( ac, "options_configure_globals", this ); //we created this one
    safePlug( ac, KStdAction::name(KStdAction::Preferences), this );

    insertSeparator();
       
    insertItem( i18n( "&Help" ), helpMenu( parent ) );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::Quit), this );

    connect( this, SIGNAL( aboutToShow() ), SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );
}

KPopupMenu*
Menu::helpMenu( QWidget *parent ) //STATIC
{
    if( HelpMenu == 0 )
        HelpMenu = new KHelpMenu( parent, KGlobal::instance()->aboutData(), pApp->actionCollection() );

    return HelpMenu->menu();
}

void
Menu::slotAboutToShow()
{
    setItemEnabled( ID_CONF_DECODER,    EngineController::instance()->engine()->decoderConfigurable() );
}

void
Menu::slotActivated( int index )
{
    if( index == ID_CONF_DECODER )
    {
        EngineController::engine()->configureDecoder();
    }
}



#include <ktoolbar.h>
#include <ktoolbarbutton.h>

//there is KActionMenu, but it doesn't work very well, hence we made our own

MenuAction::MenuAction( KActionCollection *ac )
  : KAction( i18n( "amaroK Menu" ), 0, ac, "amarok_menu" )
{}

int
MenuAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( w, id );
        connect( w, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        //TODO create menu on demand
        //TODO create menu above and aligned within window
        //TODO make the arrow point upwards!
        bar->insertButton( QString::null, id, true, i18n( "Menu" ), index );
        bar->alignItemRight( id );

        KToolBarButton* button = bar->getButton( id );
        button->setPopup( new amaroK::Menu( 0 ) );
        button->setName( "toolbutton_amarok_menu" );

        return containerCount() - 1;
    }
    else return -1;
}



PlayPauseAction::PlayPauseAction( KActionCollection *ac )
  : KAction( i18n( "Play/Pause" ), 0, ac, "play_pause" )
{
    EngineController* const ec = EngineController::instance();

    engineStateChanged( ec->engine() ? ec->engine()->state() : EngineBase::Empty );

    ec->attach( this );
    connect( this, SIGNAL( activated() ), ec, SLOT( playPause() ) );
}

PlayPauseAction::~PlayPauseAction()
{
    EngineController::instance()->detach( this );
}

void
PlayPauseAction::engineStateChanged( EngineBase::EngineState state )
{
    switch( state )
    {
    case EngineBase::Playing:
        setIcon( "player_pause" );
        break;
    default:
        setIcon( "player_play" );
        break;
    }
}



#include "blockanalyzer.h"

AnalyzerAction::AnalyzerAction( KActionCollection *ac )
  : KAction( i18n( "Analyzer" ), 0, ac, "toolbar_analyzer" )
{}

int
AnalyzerAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( w, id );
        connect( w, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        QWidget *block = new BlockAnalyzer( w );
        //block->setBackgroundColor( w->backgroundColor().dark( 110 ) );

        bar->insertWidget( id, 0, block, index );
        bar->setItemAutoSized( id, true );

        return containerCount() - 1;
    }
    else return -1;
}



RandomAction::RandomAction( KActionCollection *ac ) :
    KToggleAction( i18n( "Random &Mode" ), 0, ac, "random_mode" )
{
    KToggleAction::setChecked( AmarokConfig::randomMode() );
}

void RandomAction::setChecked( bool on )
{
    KToggleAction::setChecked( on );
    AmarokConfig::setRandomMode( on );
}


RepeatTrackAction::RepeatTrackAction( KActionCollection *ac ) :
    KToggleAction( i18n( "Repeat &Track" ), 0, ac, "repeat_track" )
{
    KToggleAction::setChecked( AmarokConfig::repeatTrack() );
}

void RepeatTrackAction::setChecked( bool on )
{
    KToggleAction::setChecked( on );
    AmarokConfig::setRepeatTrack( on );
}


RepeatPlaylistAction::RepeatPlaylistAction( KActionCollection *ac ) :
    KToggleAction( i18n( "Repeat &Playlist" ), 0, ac, "repeat_playlist" )
{
    KToggleAction::setChecked( AmarokConfig::repeatPlaylist() );
}

void RepeatPlaylistAction::setChecked( bool on )
{
    KToggleAction::setChecked( on );
    AmarokConfig::setRepeatPlaylist( on );
}




#include "amarokmenu.moc"
