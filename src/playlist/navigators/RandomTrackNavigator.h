/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_RANDOMTRACKNAVIGATOR_H
#define AMAROK_RANDOMTRACKNAVIGATOR_H

#include "TrackNavigator.h"

namespace Playlist
{
    /**
     * Plays a random track in the playlist
     */
    class RandomTrackNavigator : public TrackNavigator
    {
        Q_OBJECT

    public:
        RandomTrackNavigator();
        quint64 requestNextTrack();
        quint64 requestUserNextTrack() { return requestNextTrack(); }
        quint64 requestLastTrack();

        void reset();

    private slots:
        void recvInsertedIds( const QList<quint64>& );
        void recvRemovedIds( const QList<quint64>& );
        void recvActiveTrackChanged( const quint64 );

        void modelLayoutChanged();

    private:
        QList<quint64> m_playedRows;
        QList<quint64> m_unplayedRows;

    };
}

#endif
