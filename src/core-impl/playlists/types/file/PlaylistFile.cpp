/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistFile.h"

#include "playlistmanager/file/PlaylistFileProvider.h"

using namespace Playlists;

void
PlaylistFile::saveLater()
{
    if( !m_provider )
        return;

    PlaylistFileProvider *fileProvider = qobject_cast<PlaylistFileProvider *>( m_provider );

    if( !fileProvider )
        return;

    fileProvider->saveLater( PlaylistFilePtr( this ) );
}
