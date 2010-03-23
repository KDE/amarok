/****************************************************************************************
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef METADYNAMICPLAYLIST_H
#define METADYNAMICPLAYLIST_H

#include <Playlist.h>

namespace Meta {

/**
 * Base Class for all dynamic playlists.
 *  @author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT DynamicPlaylist : public Playlist
{
    public:
        DynamicPlaylist( PlaylistPtr playlist );

        ~DynamicPlaylist();

        /* Meta::Playlist virtuals */
        virtual QString name() const { return prettyName(); };
        virtual QString prettyName() const;

        TrackList tracks() { return m_tracks; };

        void recalculate();

    protected:
        TrackList m_tracks;
        TrackPtr m_newestTrack;
};

}

#endif
