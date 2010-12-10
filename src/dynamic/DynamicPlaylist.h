/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
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

#ifndef AMAROK_DYNAMICPLAYLIST_H
#define AMAROK_DYNAMICPLAYLIST_H

#include "core/meta/Meta.h"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace Dynamic {

/** Provides a basis for dynamic playlists.
    The DynamicPlaylist is used by the DynamicTrackNavigator.
    Currently the only implementation of this abstract class is the BiasedPlaylist.
*/
class DynamicPlaylist : public QObject
{
    Q_OBJECT

    public:
        DynamicPlaylist( QObject *parent = 0 );
        DynamicPlaylist( QXmlStreamReader *reader, QObject *parent = 0 );
        virtual ~DynamicPlaylist();

        void toXml( QXmlStreamWriter *writer ) const;

        virtual void requestTracks(int) = 0;

        QString title() const;
        void setTitle( QString );

        virtual void requestAbort() {}

    signals:
        void tracksReady( Meta::TrackList );

    public slots:
        virtual void recalculate();
        virtual void invalidate();

    protected:
        Collections::Collection* m_collection;
        QString m_title;
};

}

#endif

