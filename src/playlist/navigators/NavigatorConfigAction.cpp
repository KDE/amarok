/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "NavigatorConfigAction.h"

#include "amarokconfig.h"
#include "Debug.h"
#include "playlist/PlaylistActions.h"

#include <KMenu>
#include <KLocale>
#include <KStandardDirs>

NavigatorConfigAction::NavigatorConfigAction( QWidget * parent )
    : KAction( parent )
{

    KMenu * navigatorMenu = new KMenu( parent );
    setMenu( navigatorMenu );
    setText( i18n( "Track Progression" ) );

    QActionGroup * navigatorActions = new QActionGroup( navigatorMenu );
    navigatorActions->setExclusive( true );

    m_standardNavigatorAction = navigatorActions->addAction( i18n( "Standard" ) );
    m_standardNavigatorAction->setIcon( KIcon( "media-playlist-repeat-off-amarok" ) );
    m_standardNavigatorAction->setCheckable( true );
    //action->setIcon( true );

    QAction * action = new QAction( parent );
    action->setSeparator( true );
    navigatorActions->addAction( action );
    
    m_repeatTrackNavigatorAction = navigatorActions->addAction( i18n( "Repeat Track" ) );
    m_repeatTrackNavigatorAction->setIcon( KIcon( "media-track-repeat-amarok" ) );
    m_repeatTrackNavigatorAction->setCheckable( true );
        
    m_repeatAlbumNavigatorAction = navigatorActions->addAction( i18n( "Repeat Album" ) );
    m_repeatAlbumNavigatorAction->setIcon( KIcon( "media-album-repeat-amarok" ) );
    m_repeatAlbumNavigatorAction->setCheckable( true );
        
    m_repeatPlaylistNavigatorAction = navigatorActions->addAction( i18n( "Repeat Playlist" ) );
    m_repeatPlaylistNavigatorAction->setIcon( KIcon( "media-playlist-repeat-amarok" ) );
    m_repeatPlaylistNavigatorAction->setCheckable( true );
        
    action = new QAction( parent );
    action->setSeparator( true );
    navigatorActions->addAction( action );
    
    m_randomTrackNavigatorAction = navigatorActions->addAction( i18n( "Random Tracks" ) );
    m_randomTrackNavigatorAction->setIcon( KIcon( "amarok_track" ) );
    m_randomTrackNavigatorAction->setCheckable( true );
        
    m_randomAlbumNavigatorAction = navigatorActions->addAction( i18n( "Random Albums" ) );
    m_randomAlbumNavigatorAction->setIcon( KIcon( "media-album-shuffle-amarok" ) );
    m_randomAlbumNavigatorAction->setCheckable( true );

    navigatorMenu->addActions( navigatorActions->actions() );
        
    QMenu * favorMenu = navigatorMenu->addMenu( i18n( "Favor" ) );
    QActionGroup * favorActions = new QActionGroup( favorMenu );

    action = favorActions->addAction( i18n( "None" ) );
    action->setCheckable( true );
    
    action = favorActions->addAction( i18n( "Higher Scores" ) );
    action->setCheckable( true );
    
    action = favorActions->addAction( i18n( "Higher Ratings" ) );
    action->setCheckable( true );
    
    action = favorActions->addAction( i18n( "Not Recently Played" ) );
    action->setCheckable( true );

    favorMenu->addActions( favorActions->actions() );

    //make sure the correct entry is selected from start:
    switch( AmarokConfig::trackProgression() )
    {
        case AmarokConfig::EnumTrackProgression::RepeatTrack:
            m_repeatTrackNavigatorAction->setChecked( true );
            setIcon( KIcon( "media-track-repeat-amarok" ) );
            break;

        case AmarokConfig::EnumTrackProgression::RepeatAlbum:
            m_repeatAlbumNavigatorAction->setChecked( true );
            setIcon( KIcon( "media-album-repeat-amarok" ) );
            break;

        case AmarokConfig::EnumTrackProgression::RepeatPlaylist:
            m_repeatPlaylistNavigatorAction->setChecked( true );
            setIcon( KIcon( "media-playlist-repeat-amarok" ) );
            break;

        case AmarokConfig::EnumTrackProgression::RandomTrack:
            m_randomTrackNavigatorAction->setChecked( true );
            setIcon( KIcon( "amarok_track" ) );
            break;

        case AmarokConfig::EnumTrackProgression::RandomAlbum:
            m_randomAlbumNavigatorAction->setChecked( true );
            setIcon( KIcon( "media-album-shuffle-amarok" ) );
            break;

        case AmarokConfig::EnumTrackProgression::Normal:
        default:
            m_standardNavigatorAction->setChecked( true );
            setIcon( KIcon( "media-playlist-repeat-amarok" ) );
            break;
    }

     connect( navigatorMenu, SIGNAL( triggered( QAction* ) ), this, SLOT( setActiveNavigator( QAction* ) ) );
}

NavigatorConfigAction::~NavigatorConfigAction()
{
}

void NavigatorConfigAction::setActiveNavigator( QAction *navigatorAction )
{
    DEBUG_BLOCK
    if( navigatorAction == m_standardNavigatorAction )
    {
        AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::Normal );
        setIcon( KIcon( "media-playlist-repeat-amarok" ) );
    }
    else if ( navigatorAction == m_repeatTrackNavigatorAction )
    {
        AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RepeatTrack );
        setIcon( KIcon( "media-track-repeat-amarok" ) );
    }
    else if ( navigatorAction == m_repeatAlbumNavigatorAction )
    {
        AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RepeatAlbum );
        setIcon( KIcon( "media-album-repeat-amarok" ) );
    }
    else if ( navigatorAction == m_repeatPlaylistNavigatorAction )
    {
        AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RepeatPlaylist );
        setIcon( KIcon( "media-playlist-repeat-amarok" ) );
    }
    else if ( navigatorAction == m_randomTrackNavigatorAction )
    {
        AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RandomTrack );
        setIcon( KIcon( "amarok_track" ) );
    }
    else if ( navigatorAction == m_randomAlbumNavigatorAction )
    {
        AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RandomAlbum );
        setIcon( KIcon( "media-album-shuffle-amarok" ) );
    }

    The::playlistActions()->playlistModeChanged();
}

#include "NavigatorConfigAction.moc"