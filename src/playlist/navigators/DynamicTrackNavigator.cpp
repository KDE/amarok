/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "Debug.h"
#include "DynamicModel.h"
#include "DynamicPlaylist.h"
#include "core/meta/Meta.h"
#include "amarokconfig.h"
#include "playlist/PlaylistModelStack.h"

#include <QMutexLocker>


Playlist::DynamicTrackNavigator::DynamicTrackNavigator( Dynamic::DynamicPlaylistPtr p )
        : m_playlist( p )
{
    DEBUG_BLOCK

    connect( m_playlist.data(), SIGNAL( tracksReady( Meta::TrackList ) ), SLOT( receiveTracks( Meta::TrackList ) ) );
    connect( m_model->qaim(), SIGNAL( activeTrackChanged( quint64 ) ), SLOT( trackChanged() ) );
    connect( m_model->qaim(), SIGNAL( modelReset() ), SLOT( repopulate() ) );
    connect( PlaylistBrowserNS::DynamicModel::instance(), SIGNAL( activeChanged() ), SLOT( activePlaylistChanged() ) );
}

Playlist::DynamicTrackNavigator::~DynamicTrackNavigator()
{
    if( !m_playlist.isNull() )
        m_playlist->requestAbort();
}

void
Playlist::DynamicTrackNavigator::receiveTracks( Meta::TrackList tracks )
{
    DEBUG_BLOCK

    The::playlistController()->insertOptioned( tracks, Append );
}

void
Playlist::DynamicTrackNavigator::appendUpcoming()
{
    DEBUG_BLOCK

    int updateRow = m_model->activeRow() + 1;
    int rowCount = m_model->rowCount();
    int upcomingCountLag = AmarokConfig::upcomingTracks() - ( rowCount - updateRow );

    if ( upcomingCountLag > 0 && !m_playlist.isNull() )
        m_playlist->requestTracks( upcomingCountLag );
}

void
Playlist::DynamicTrackNavigator::removePlayed()
{
    int activeRow = m_model->activeRow();
    if ( activeRow > AmarokConfig::previousTracks() )
    {
        The::playlistController()->removeRows( 0, activeRow - AmarokConfig::previousTracks() );
    }
}

void
Playlist::DynamicTrackNavigator::activePlaylistChanged()
{
    DEBUG_BLOCK

    Dynamic::DynamicPlaylistPtr newPlaylist =
        PlaylistBrowserNS::DynamicModel::instance()->activePlaylist();

    if ( newPlaylist == m_playlist )
        return;

    if( m_playlist )
        m_playlist->requestAbort();

    QMutexLocker locker( &m_mutex );

    m_playlist = newPlaylist;

    connect( m_playlist.data(), SIGNAL( tracksReady( Meta::TrackList ) ), SLOT( receiveTracks( Meta::TrackList ) ) );
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
    DEBUG_BLOCK
    if ( !m_mutex.tryLock() )
        return;

    int row = m_model->activeRow() + 1;
    if ( row < 0 )
        row = 0;

    // Don't remove queued tracks
    QList<int> rows;

    do {
        if( !(m_model->stateOfRow( row ) & Item::Queued) )
            rows << row;
        row++;
    }
    while( row < m_model->rowCount() );

    if( !rows.isEmpty() )
        The::playlistController()->removeRows( rows );

    if( !m_playlist.isNull() )
        m_playlist->recalculate();
    appendUpcoming();

    m_mutex.unlock();
}
