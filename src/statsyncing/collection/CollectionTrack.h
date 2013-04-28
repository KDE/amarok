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

#ifndef STATSYNCING_COLLECTIONTRACK_H
#define STATSYNCING_COLLECTIONTRACK_H

#include "core/meta/forward_declarations.h"
#include "statsyncing/Track.h"

namespace StatSyncing
{

    class CollectionTrack : public Track
    {
        public:
            explicit CollectionTrack( Meta::TrackPtr track );
            virtual ~CollectionTrack();

            virtual QString name() const;
            virtual QString album() const;
            virtual QString artist() const;
            virtual QString composer() const;
            virtual int year() const;
            virtual int trackNumber() const;
            virtual int discNumber() const;

            virtual int rating() const;
            virtual void setRating( int rating );
            virtual QDateTime firstPlayed() const;
            virtual void setFirstPlayed( const QDateTime &firstPlayed );
            virtual QDateTime lastPlayed() const;
            virtual void setLastPlayed( const QDateTime &lastPlayed );
            virtual int playCount() const;
            virtual int recentPlayCount() const;
            virtual void setPlayCount( int playCount );
            virtual QSet<QString> labels() const;
            virtual void setLabels( const QSet<QString> &labels );

            virtual Meta::TrackPtr metaTrack() const;
            virtual void commit();

        private:
            Q_DISABLE_COPY( CollectionTrack )

            /**
             * Calls m_trackStats->beginUpdate() if it hasn't been already called
             */
            void beginUpdate();

            Meta::TrackPtr m_track;
            Meta::StatisticsPtr m_trackStats;
            bool m_beginUpdateAlreadyCalled;
    };

} // namespace StatSyncing

#endif // STATSYNCING_COLLECTIONTRACK_H
