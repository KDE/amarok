/***************************************************************************
 * copyright       : (C) 2008 Mark Kretschmann <kretschmann@kde.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 ***************************************************************************/

#include "RepeatAlbumNavigator.h"

#include "Debug.h"
#include "EngineController.h"
#include "PlaylistModel.h"

using namespace Playlist;

int
RepeatAlbumNavigator::nextRow()
{
    DEBUG_BLOCK

    int currentRow = m_playlistModel->activeRow();
    QString currentAlbum = m_playlistModel->trackForRow( currentRow )->album()->name(); 
    QString newAlbum;

    // Iterate over all tracks in the playlist, starting with the next row and rolling over at the end,
    // and stop when track from the same album is found 
    do
    {
        currentRow++;
        if( !m_playlistModel->rowExists( currentRow ) )
            currentRow = 0;

        newAlbum = m_playlistModel->trackForRow( currentRow )->album()->name();
    }
    while( newAlbum != currentAlbum );


    return currentRow;
}



