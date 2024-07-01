/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
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

#ifndef AMAROKPLAYLISTITEM_H
#define AMAROKPLAYLISTITEM_H

#include "core/meta/forward_declarations.h"

namespace Playlist
{
    /** Playlist::Item is used in the playlist model to reference an entry in the playlist.
        It combines a track with a unique id and a played state.
    */
    class Item
    {
        public:
            enum State
            {
                Invalid = 0,
                NewlyAdded,
                Unplayed,
                Played
            };

            static void listRemove( QList<quint64> &target, QSet<quint64> &removeSet );

            Item();
            explicit Item( const Meta::TrackPtr &track );
            ~Item();

            const Meta::TrackPtr& track() const;

            State state() const { return m_state; }
            void setState ( State s ) { m_state = s; }

            /**
             * @return a random number which uniquely identifies this particular
             * instance of a track in the playlist
             */
            quint64 id() const { return m_id; }

        private:
            Meta::TrackPtr m_track;
            State          m_state;
            quint64        m_id;
    };
}

Q_DECLARE_METATYPE( Playlist::Item* )

#endif
