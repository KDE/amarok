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

#include "core/meta/forward_declarations.h"
#include "core/collections/QueryMaker.h"

#include "amarok_export.h" // we are exporting it for the tests

namespace Collections {
    class Collection;
}
class QXmlStreamReader;
class QXmlStreamWriter;

namespace Dynamic {

/** Provides a basis for dynamic playlists.
    The DynamicPlaylist is used by the DynamicTrackNavigator.
    Currently the only implementation of this abstract class is the BiasedPlaylist.
*/
class AMAROK_EXPORT DynamicPlaylist : public QObject
{
    Q_OBJECT

    public:
        explicit DynamicPlaylist( QObject *parent = nullptr );
        explicit DynamicPlaylist( QXmlStreamReader *reader, QObject *parent = nullptr );
        virtual ~DynamicPlaylist();

        virtual void toXml( QXmlStreamWriter *writer ) const = 0;

        virtual void requestTracks(int) = 0;

        QString title() const;
        void setTitle( QString );

    Q_SIGNALS:
        void tracksReady( Meta::TrackList );

        /** Emitted when this playlist has been modified in some way.
            The DynamicModel will listen to it to detect if it needs to save it.
        */
        void changed( Dynamic::DynamicPlaylist* playlist );


    public Q_SLOTS:
        /** Start recalculating all tracks after the currently played track */
        // virtual void repopulate();

        /** Aborts the current playlist generation operation */
        virtual void requestAbort()
        {}

    protected:
        Collections::Collection* m_collection;
        QString m_title;
};

}

#endif

