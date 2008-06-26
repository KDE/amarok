/***************************************************************************
 * copyright            : (C) 2008 Seb Ruiz <ruiz@kde.org>    
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "RandomTrackNavigator.h"

#include "Debug.h"
#include "PlaylistItem.h"
#include "PlaylistModel.h"

#include <KRandom>

using namespace Playlist;


int
RandomTrackNavigator::nextRow()
{
    DEBUG_BLOCK

    // TODO: is any of this necessary? Will RowList take care of it?
    // later we should try commenting this out and seeing what happens
    if( playlistChanged() )
    {
        debug() << "Playlist has changed, regenerating unplayed tracks";
        generateUnplayedRows();
        TrackNavigator::playlistChangeHandled();
    }

    int lastRow = m_playlistModel->activeRow();
    m_playedRows.append( lastRow );
    m_unplayedRows.removeAll( lastRow );

    if( !m_unplayedRows.isEmpty() && m_playlistModel->stopAfterMode() != StopAfterCurrent )
    {
        return m_unplayedRows.at( KRandom::random() % m_unplayedRows.count() );
    }

    m_playedRows.clear();
    TrackNavigator::setPlaylistChanged(); // will cause generateUnplayedTracks() to be called
    debug() << "There are no more tracks to play, starting over";

    // out of tracks to play or stopAfterMode == Current.
    return -1;
}

int
RandomTrackNavigator::lastRow()
{
    if( m_playedRows.isEmpty() ) return -1;
    else return m_playedRows.takeAt( m_playedRows.count() - 1 );
}

void
RandomTrackNavigator::generateUnplayedRows()
{
    m_unplayedRows.clear();
    int row;
    for( row = 0; row < m_playlistModel->rowCount(); ++row )
    {
        if( !m_playedRows.contains( row ) ) m_unplayedRows.append( row );
    }
}

