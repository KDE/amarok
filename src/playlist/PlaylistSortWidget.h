/***************************************************************************
*   Copyright © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
***************************************************************************/

#ifndef AMAROK_PLAYLISTSORTWIDGET_H
#define AMAROK_PLAYLISTSORTWIDGET_H

#include <KHBox>

namespace Playlist
{
/**
 * A ribbon interface that allows the user to define multiple sorting levels for the playlist.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class SortWidget : public KHBox
{
    Q_OBJECT
    public:
        SortWidget( QWidget* parent = 0 );

    private slots:
        void applySortingScheme();
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSORTWIDGET_H