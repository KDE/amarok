// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution

#include "amarokconfig.h"
#include "amarokmenu.h"
#include "playerapp.h"
#include "enginecontroller.h"

#include <kaction.h>
#include <khelpmenu.h>
#include <klocale.h>

//static member
KHelpMenu *amaroK::Menu::HelpMenu = 0;


static bool plug( KActionCollection *ac, const char *name, QWidget *w )
{
    if( ac )
    {
        KAction *a = ac->action( name );

        if( a )
        {
            a->plug( w );
            return true;
        }
    }

    return false;
}


amaroK::Menu::Menu( QWidget *parent, KActionCollection *ac )
  : QPopupMenu( parent )
{
    setCheckable( true );

    insertItem( i18n( "Repeat &Track" ),    ID_REPEAT_TRACK );
    insertItem( i18n( "Repeat &Playlist" ), ID_REPEAT_PLAYLIST );
    insertItem( i18n( "Random &Mode" ),     ID_RANDOM_MODE );

    insertSeparator();

    insertItem( i18n( "Configure &Effects..." ), pApp, SLOT( showEffectWidget() ) );
    insertItem( i18n( "Configure &Decoder..." ), ID_CONF_DECODER );

    insertSeparator();

    plug( ac, "options_configure_toolbars", this );
    plug( ac, "options_configure_keybinding", this );
    plug( ac, "options_configure_globals", this );
    plug( ac, "options_configure", this );

    insertSeparator();

    insertItem( i18n( "&Help" ), helpMenu( parent ) );

    insertSeparator();

    plug( ac, "file_quit", this );

    connect( this, SIGNAL( aboutToShow() ), SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );
}

KPopupMenu*
amaroK::Menu::helpMenu( QWidget *parent ) //STATIC
{
    if( HelpMenu == 0 )
        HelpMenu = new KHelpMenu( parent, KGlobal::instance()->aboutData(), pApp->actionCollection() );

    return HelpMenu->menu();
}


void amaroK::Menu::slotAboutToShow()
{
    setItemChecked( ID_REPEAT_TRACK,    AmarokConfig::repeatTrack() );
    setItemChecked( ID_REPEAT_PLAYLIST, AmarokConfig::repeatPlaylist() );
    setItemChecked( ID_RANDOM_MODE,     AmarokConfig::randomMode() );
    setItemEnabled( ID_CONF_DECODER,    EngineController::instance()->engine()->decoderConfigurable() );
}


void amaroK::Menu::slotActivated( int index )
{
    switch( index ) {
    case ID_REPEAT_TRACK:
        AmarokConfig::setRepeatTrack( !isItemChecked(ID_REPEAT_TRACK) );
        break;
    case ID_REPEAT_PLAYLIST:
        AmarokConfig::setRepeatPlaylist( !isItemChecked(ID_REPEAT_PLAYLIST) );
        break;
    case ID_RANDOM_MODE:
        AmarokConfig::setRandomMode( !isItemChecked(ID_RANDOM_MODE) );
        break;
    case ID_CONF_DECODER:
        EngineController::instance()->engine()->configureDecoder();
        break;
    }
}


#include <ktoolbar.h>
#include <ktoolbarbutton.h>

amaroK::MenuAction::MenuAction( KActionCollection *ac )
  : KAction( i18n( "amaroK Menu" ), 0, ac, "amarok_menu" )
{}

int
amaroK::MenuAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( !bar || !kapp->authorizeKAction(name()) ) return -1;

    const int id = KAction::getToolButtonID();

    //TODO create menu on demand
    //TODO create menu above and aligned within window
    //TODO make the arrow point upwards!
    bar->insertButton( QString::null, id, true, i18n( "Menu" ), index );
    bar->alignItemRight( id );

    KToolBarButton *button = bar->getButton( id );
    button->setPopup( new amaroK::Menu( bar, parentCollection() ) );
    button->setName( "toolbutton_amarok_menu" );

    addContainer( bar, id );
    connect( bar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

    return containerCount() - 1;
}


#include "enginecontroller.h"
#include <kdebug.h>

amaroK::PlayPauseAction::PlayPauseAction( KActionCollection *ac )
  : KAction( i18n( "Play/Pause" ), 0, ac, "play_pause" )
{
    EngineController* const ec = EngineController::instance();

    ec->attach( this );
    connect( this, SIGNAL( activated() ), ec, SLOT( pause() ) );
}

void
amaroK::PlayPauseAction::engineStateChanged( EngineBase::EngineState state )
{
    switch( state )
    {
    case EngineBase::Playing:
        kdDebug() << "played\n";
        setIcon( "player_pause" );
        break;
    default:
        kdDebug() << "paused\n";
        setIcon( "player_play" );
        break;
    }
}

#include "amarokmenu.moc"
