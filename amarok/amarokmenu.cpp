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
KHelpMenu *AmarokMenu::HelpMenu = 0;


AmarokMenu::AmarokMenu( QWidget *parent )
  : QPopupMenu( parent )
{
    KActionCollection *ac = pApp->actionCollection();

    setCheckable( true );

    insertItem( i18n( "Repeat &Track" ),    ID_REPEAT_TRACK );
    insertItem( i18n( "Repeat &Playlist" ), ID_REPEAT_PLAYLIST );
    insertItem( i18n( "Random &Mode" ),     ID_RANDOM_MODE );

    insertSeparator();

    insertItem( i18n( "Configure &Effects..." ), pApp, SLOT( showEffectWidget() ) );
    insertItem( i18n( "Configure &Decoder..." ), pApp, SIGNAL( configureDecoder() ), 0, ID_CONF_DECODER );

    insertSeparator();

    ac->action( "options_configure_keybinding" )->plug( this );
    ac->action( "options_configure_global_keybinding" )->plug( this );
    ac->action( "options_configure" )->plug( this );

    insertSeparator();

    insertItem( i18n( "&Help" ), helpMenu( parent ) );

    insertSeparator();

    ac->action( "file_quit" )->plug( this );
}

KPopupMenu*
AmarokMenu::helpMenu( QWidget *parent ) //STATIC
{
    if( HelpMenu == 0 )
        HelpMenu = new KHelpMenu( parent, KGlobal::instance()->aboutData(), pApp->actionCollection() );

    return HelpMenu->menu();
}

int
AmarokMenu::exec( const QPoint &p, int indexAtPoint ) //NOTE non virtual! :(
{
    setItemChecked( ID_REPEAT_TRACK,    AmarokConfig::repeatTrack() );
    setItemChecked( ID_REPEAT_PLAYLIST, AmarokConfig::repeatPlaylist() );
    setItemChecked( ID_RANDOM_MODE,     AmarokConfig::randomMode() );
    setItemEnabled( ID_CONF_DECODER,    EngineController::instance()->engine()->decoderConfigurable() );

    int index = QPopupMenu::exec( p, indexAtPoint );

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
    }

    return index;
}
