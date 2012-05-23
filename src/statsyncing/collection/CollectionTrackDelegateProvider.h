/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef COLLECTIONTRACKDELEGATEPROVIDER_H
#define COLLECTIONTRACKDELEGATEPROVIDER_H

#include "statsyncing/TrackDelegateProvider.h"
#include <core/collections/Collection.h>
#include <core/meta/Meta.h>
#include <QSemaphore>

class QEventLoop;

namespace StatSyncing
{
    /**
     * Track delegate provider that has Collections::Colections as a back-end.
     */
    class CollectionTrackDelegateProvider : public QObject, public TrackDelegateProvider
    {
        Q_OBJECT

        public:
            /**
             * Construct provider that has @param collection as a back-end.
             */
            CollectionTrackDelegateProvider( Collections::Collection *collection );
            virtual ~CollectionTrackDelegateProvider();

            virtual QString id() const;
            virtual QString prettyName() const;
            virtual KIcon icon() const;
            virtual qint64 reliableTrackMetaData() const;
            virtual QSet<QString> artists();
            virtual TrackDelegateList artistTracks( const QString &artistName );

        signals:
            /// hacks to create and start QueryMaker in main eventloop
            void startArtistSearch();
            void startTrackSearch( QString artistName );

        private slots:
            /// @see startArtistSearch
            void slotStartArtistSearch();
            void slotStartTrackSearch( QString artistName );

            void slotNewResultReady( Meta::ArtistList list );
            void slotNewResultReady( Meta::TrackList list );
            void slotQueryDone();

        private:
            Q_DISABLE_COPY(CollectionTrackDelegateProvider)

            /// collection can disappear at any time, use weak pointer to notice it
            QWeakPointer<Collections::Collection> m_coll;
            QSet<QString> m_foundArtists;
            TrackDelegateList m_foundTracks;
            QSemaphore m_queryMakerSemaphore;
    };

} // namespace StatSyncing

#endif // COLLECTIONTRACKDELEGATEPROVIDER_H
