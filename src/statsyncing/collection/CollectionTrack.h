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

            QString name() const override;
            QString album() const override;
            QString artist() const override;
            QString composer() const override;
            int year() const override;
            int trackNumber() const override;
            int discNumber() const override;

            int rating() const override;
            void setRating( int rating ) override;
            QDateTime firstPlayed() const override;
            void setFirstPlayed( const QDateTime &firstPlayed ) override;
            QDateTime lastPlayed() const override;
            void setLastPlayed( const QDateTime &lastPlayed ) override;
            int playCount() const override;
            int recentPlayCount() const override;
            void setPlayCount( int playCount ) override;
            QSet<QString> labels() const override;
            void setLabels( const QSet<QString> &labels ) override;

            Meta::TrackPtr metaTrack() const override;
            void commit() override;

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
