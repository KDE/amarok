/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/


#include "core/playlists/PlaylistProvider.h"
#include "PlaylistProvider.moc"

using namespace Playlists;

PlaylistPtr
PlaylistProvider::addPlaylist( Playlists::PlaylistPtr playlist )
{
    Q_UNUSED( playlist );
    return PlaylistPtr();
}

Meta::TrackPtr
PlaylistProvider::addTrack( Meta::TrackPtr track )
{
    Q_UNUSED( track );
    return Meta::TrackPtr();
}
