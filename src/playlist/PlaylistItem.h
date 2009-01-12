/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *                        (C) 2008 Soren Harward <stharward@gmail.com>
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


#ifndef AMAROKPLAYLISTITEM_H
#define AMAROKPLAYLISTITEM_H

#include "meta/Meta.h"

namespace Playlist
{
    class Item
    {
        public:
            enum State
            {
                Invalid,
                NewlyAdded,
                Unplayed,
                Played,
                Queued
            };

            Item() : m_track ( 0 ), m_state ( Invalid ), m_id ( 0 ) { }
            Item ( Meta::TrackPtr track );
            ~Item();
            Meta::TrackPtr track() const { return m_track; }

            State state() const { return m_state; }
            void setState ( State s ) { m_state = s; }

            /**
             * @return a random number which uniquely identifies this particular
             * instance of a track in the playlist
             */
            quint64 id() const { return m_id; }

            //sorting functions galore...
            static bool lessThanAlbum ( Item* left, Item* right );
            static bool lessThanAlbumArtist ( Item* left, Item* right );
            static bool lessThanArtist ( Item* left, Item* right );
            static bool lessThanBitrate ( Item* left, Item* right );
            static bool lessThanBpm ( Item* left, Item* right );
            static bool lessThanComment ( Item* left, Item* right );
            static bool lessThanComposer ( Item* left, Item* right );
            static bool lessThanDirectory ( Item* left, Item* right );
            static bool lessThanDiscNumber ( Item* left, Item* right );
            static bool lessThanFilename ( Item* left, Item* right );
            static bool lessThanFilesize ( Item* left, Item* right );
            static bool lessThanGenre ( Item* left, Item* right );
            static bool lessThanLastPlayed ( Item* left, Item* right );
            static bool lessThanLength ( Item* left, Item* right );
            static bool lessThanMood ( Item* left, Item* right );
            static bool lessThanPlayCount ( Item* left, Item* right );
            static bool lessThanRating ( Item* left, Item* right );
            static bool lessThanSampleRate ( Item* left, Item* right );
            static bool lessThanScore ( Item* left, Item* right );
            static bool lessThanSource ( Item* left, Item* right );
            static bool lessThanTitle ( Item* left, Item* right );
            static bool lessThanTrackNumber ( Item* left, Item* right );
            static bool lessThanType ( Item* left, Item* right );
            static bool lessThanYear ( Item* left, Item* right );

        private:
            Meta::TrackPtr m_track;
            State          m_state;
            quint64        m_id;
    };
}

Q_DECLARE_METATYPE ( Playlist::Item* )

#endif
