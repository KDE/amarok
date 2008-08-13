/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
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

#include "UndoCommands.h"


using namespace Playlist;
using namespace Qt;

AddTracksCmd::AddTracksCmd( QUndoCommand* parent, int row, Meta::TrackList tracks )
    : QUndoCommand( i18n("Tracks Added"), parent )
    , m_tracks( tracks )
    , m_row( row )
{  }

void
AddTracksCmd::redo()
{
    The::playlistModel()->insertTracksCommand( m_row, m_tracks );
}

void
AddTracksCmd::undo()
{
    The::playlistModel()->removeTracksCommand( m_row, m_tracks.size() );
}

RemoveTracksCmd::RemoveTracksCmd( QUndoCommand* parent, int position, int numOfRows )
    : QUndoCommand( i18n("Tracks Removed"), parent )
    , m_numOfRows( numOfRows )
    , m_position( position )
{  }

void
RemoveTracksCmd::redo()
{
    m_tracks = The::playlistModel()->removeTracksCommand( m_position, m_numOfRows );
}

void
RemoveTracksCmd::undo()
{
    The::playlistModel()->insertTracksCommand( m_position, m_tracks );
}

AddPlaylistsCmd::AddPlaylistsCmd( QUndoCommand * parent, int row, Meta::PlaylistList playlists )
    : QUndoCommand( i18n("Playlists Added"), parent )
        , m_playlists( playlists )
        , m_row( row )
{
}

void
AddPlaylistsCmd::redo()
{
    /* A playlist isn't actually added, only it's tracks are.
    * But the Playlist is observed for changes to it's trackList.
    */
    foreach( Meta::PlaylistPtr playlist, m_playlists )
    {
        The::playlistModel()->registerPlaylist( playlist );
        The::playlistModel()->insertTracksCommand( m_row, playlist->tracks() );
    }
}

void
AddPlaylistsCmd::undo()
{
    int startRow = m_row;
    foreach( Meta::PlaylistPtr playlist, m_playlists )
    {
        The::playlistModel()->unRegisterPlaylist( playlist );
        The::playlistModel()->removeTracksCommand( startRow, playlist->tracks().size() );
    }
}



Playlist::MoveTracksCmd::MoveTracksCmd( QUndoCommand * parent, int from, int to )
    : QUndoCommand( i18n("Track moved"), parent )
    , m_from( from )
    , m_to( to )
{

}

void Playlist::MoveTracksCmd::redo()
{
    DEBUG_BLOCK
    The::playlistModel()->moveRowCommand( m_from, m_to );
}

void Playlist::MoveTracksCmd::undo()
{
    The::playlistModel()->moveRowCommand( m_to, m_from );
}
