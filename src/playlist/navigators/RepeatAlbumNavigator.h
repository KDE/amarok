/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef REPEATALBUMNAVIGATOR_H
#define REPEATALBUMNAVIGATOR_H

#include "TrackNavigator.h"

#include "Meta.h"

#include <QHash>
#include <QList>

namespace Playlist
{
    /**
        Navigator which repeats one album over and over

        @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
        @author Soren Harward <stharward@gmail.com>
    */

    class RepeatAlbumNavigator : public TrackNavigator
    {
        Q_OBJECT

    public:
        RepeatAlbumNavigator();

        quint64 requestNextTrack();
        quint64 requestUserNextTrack() { return requestNextTrack(); }
        quint64 requestLastTrack();

        void reset() {};

    private slots:
        void recvInsertedIds( const QList<quint64>& );
        void recvRemovedIds( const QList<quint64>& );
        void recvActiveTrackChanged( const quint64 );

    private:
        static bool idLessThan( const quint64 left, const quint64 right );
        void sortTheseAlbums( const Meta::AlbumList );

        void dump();

        QHash<Meta::AlbumPtr, ItemList> m_albumGroups;

        Meta::AlbumPtr m_currentAlbum;
        quint64 m_currentTrack;
    };
}
#endif
