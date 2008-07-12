/***************************************************************************
 * copyright   : (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
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
 
#include "RandomAlbumNavigator.h"

#include "Debug.h"
#include "PlaylistItem.h"
#include "PlaylistModel.h"

#include <KRandom>

using namespace Playlist;

int RandomAlbumNavigator::nextRow()
{

    if( playlistChanged() )
    {
        generateAlbumHeaders();
        TrackNavigator::playlistChangeHandled();
    }

    int currentRow = m_playlistModel->activeRow();
    QString currentAlbum = m_playlistModel->trackForRow( currentRow )->album()->name();
    QString newAlbum;

    m_playedRows.append( currentRow );

    //is the next row in the same album
    if ( ( currentRow + 1 ) < m_playlistModel->rowCount() && currentAlbum == m_playlistModel->trackForRow( currentRow + 1  )->album()->name() ) {
        //same album, just keep playing
        return currentRow + 1;
    } else {
        //jump to a random, unplayed album header
        generateUnplayedAlbums();
        if ( !m_unplayedAlbums.isEmpty() )
            return m_unplayedAlbums[ KRandom::random() % m_unplayedAlbums.count() ];
    }
    return -1;

}

int RandomAlbumNavigator::lastRow()
{
    if( m_playedRows.isEmpty() ) return -1;
    else return m_playedRows.takeAt( m_playedRows.count() - 1 );
}

void RandomAlbumNavigator::generateAlbumHeaders()
{
    //DEBUG_BLOCK
    m_albumHeaders.clear();
    int row;
    QString currentAlbum = QString();
    for( row = 0; row < m_playlistModel->rowCount(); ++row )
    {
        if( currentAlbum != m_playlistModel->trackForRow( row )->album()->name() ) {
            m_albumHeaders.append( row );
            currentAlbum = m_playlistModel->trackForRow( row )->album()->name();
            //debug() << "Album starting at: " << row;
        }
    }
}

void RandomAlbumNavigator::generateUnplayedAlbums()
{
    //DEBUG_BLOCK
    m_unplayedAlbums.clear();
    //int albumHeadRow;
    for( int row = 0; row < m_albumHeaders.count(); ++row ) {
        //debug() << "Album starting at: " << m_albumHeaders[row];
        if ( !m_playedRows.contains( m_albumHeaders[row] ) ) {
            m_unplayedAlbums.append( m_albumHeaders[row] );
            //debug() << "Unplayed album starting at: " << m_albumHeaders[row];
        }
    }
}


