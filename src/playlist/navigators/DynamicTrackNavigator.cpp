/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicTrackNavigator.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "dynamic/DynamicPlaylist.h"
#include "dynamic/DynamicModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistController.h"

Playlist::DynamicTrackNavigator::DynamicTrackNavigator()
    : m_playlist( nullptr )
{
    connect( Playlist::ModelStack::instance()->bottom(), &Playlist::Model::activeTrackChanged,
             this, &DynamicTrackNavigator::trackChanged );
    connect( m_model->qaim(), &QAbstractItemModel::modelReset,
             this, &DynamicTrackNavigator::repopulate );

    connect( Dynamic::DynamicModel::instance(), &Dynamic::DynamicModel::activeChanged,
             this, &DynamicTrackNavigator::activePlaylistChanged );
    activePlaylistChanged();
}

Playlist::DynamicTrackNavigator::~DynamicTrackNavigator()
{
    if( m_playlist )
        m_playlist->requestAbort();
}

void
Playlist::DynamicTrackNavigator::receiveTracks( const Meta::TrackList &tracks )
{
    The::playlistController()->insertOptioned( tracks );
}

void
Playlist::DynamicTrackNavigator::appendUpcoming()
{
    // a little bit stupid. the playlist jumps to the newly inserted tracks

    int updateRow = m_model->activeRow() + 1;
    int rowCount = m_model->qaim()->rowCount();
    int upcomingCountLag = AmarokConfig::upcomingTracks() - ( rowCount - updateRow );

    if( upcomingCountLag > 0 && m_playlist )
        m_playlist->requestTracks( upcomingCountLag );
}

void
Playlist::DynamicTrackNavigator::removePlayed()
{
    int activeRow = m_model->activeRow();
    if( activeRow > AmarokConfig::previousTracks() )
        The::playlistController()->removeRows( 0, activeRow - AmarokConfig::previousTracks() );
}

void
Playlist::DynamicTrackNavigator::activePlaylistChanged()
{
    DEBUG_BLOCK

    Dynamic::DynamicPlaylist *newPlaylist =
        Dynamic::DynamicModel::instance()->activePlaylist();

    if( newPlaylist == m_playlist )
        return;

    if( m_playlist )
    {
        disconnect( m_playlist, &Dynamic::DynamicPlaylist::tracksReady,
                    this, &DynamicTrackNavigator::receiveTracks );
        m_playlist->requestAbort();
    }

    m_playlist = newPlaylist;
    if( !m_playlist )
    {
        warning() << "No dynamic playlist current loaded! Creating dynamic track navigator with null playlist!";
    }
    else
    {
        connect( m_playlist, &Dynamic::DynamicPlaylist::tracksReady,
                 this, &DynamicTrackNavigator::receiveTracks );
    }
}

void
Playlist::DynamicTrackNavigator::trackChanged()
{
    appendUpcoming();
    removePlayed();
}

void
Playlist::DynamicTrackNavigator::repopulate()
{
    // remove all future tracks
    int activeRow = m_model->activeRow();
    int rowCount = m_model->qaim()->rowCount();
    if( activeRow < rowCount )
        The::playlistController()->removeRows( activeRow + 1, rowCount - activeRow - 1);

    appendUpcoming();
}
