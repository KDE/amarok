/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef DYNAMICTRACKNAVIGATOR_H
#define DYNAMICTRACKNAVIGATOR_H

#include "StandardTrackNavigator.h"

namespace Dynamic
{
    class DynamicPlaylist;
}

#include <QPointer>

namespace Playlist
{

    /** A navigator that implements 'dynamic mode', which is a never-ending queue of tracks.
        The navigator will use the current dynamic playlist and try to get new
        tracks before advancing
    */
    class DynamicTrackNavigator : public StandardTrackNavigator
    {
        Q_OBJECT

        public:
            DynamicTrackNavigator() ;
            ~DynamicTrackNavigator();

            void appendUpcoming();

        public Q_SLOTS:
            void repopulate();

        private Q_SLOTS:
            void activePlaylistChanged();
            void receiveTracks( Meta::TrackList );
            void trackChanged();

        private:
            void removePlayed();

            QPointer<Dynamic::DynamicPlaylist> m_playlist;
    };
}

#endif
