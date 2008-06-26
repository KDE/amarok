/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "RepeatTrackNavigator.h"


#include "EngineController.h"
#include "PlaylistModel.h"

using namespace Playlist;

int
RepeatTrackNavigator::nextRow()
{
    // we need to repeat
    if( m_previousTrack.empty() || 
            (m_previousTrack[0] != m_playlistModel->activeRow() ) )
    {
        m_previousTrack.append( m_playlistModel->activeRow() );
        return m_playlistModel->activeRow();
    }
    else
    {
        // We already repeated, so advance.
        if( m_previousTrack[0] == m_playlistModel->activeRow() )
        {
            int updateRow = m_playlistModel->activeRow() + 1;
            if( m_playlistModel->rowExists( updateRow ) ) return updateRow;
        }

    }

    return -1;
}

int
RepeatTrackNavigator::userNextRow()
{
    int updateRow = m_playlistModel->activeRow() + 1;
    if( m_playlistModel->rowExists( updateRow ) ) setCurrentTrack( updateRow );
    return -1;
}


