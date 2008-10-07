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

#include "playlist/PlaylistModel.h"

int
Playlist::RepeatTrackNavigator::nextRow()
{
    return Model::instance()->activeRow();
}

int
Playlist::RepeatTrackNavigator::userNextRow()
{
    int updateRow = The::playlistModel()->activeRow() + 1;
    if( Model::instance()->rowExists( updateRow ) ) setCurrentTrack( updateRow );
    return -1;
}


