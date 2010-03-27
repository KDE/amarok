/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef STATISTICS_STATISTICSPROVIDER_H
#define STATISTICS_STATISTICSPROVIDER_H

#include "shared/amarok_export.h"
#include "core/meta/Meta.h"

#include <QDateTime>

namespace Statistics
{
    class AMAROK_EXPORT StatisticsProvider
    {                
        public:
            StatisticsProvider();
            virtual ~StatisticsProvider();

             /** Returns the score of this track */
            double score() const;
            void setScore( double newScore );
            /** Returns the rating of this track */
            int rating() const;
            void setRating( int newRating );
            /** Returns the time the song was last played, or 0 if it has not been played yet */
            QDateTime lastPlayed() const;
            void setLastPlayed( const QDateTime &dt );
            /** Returns the time the song was first played, or 0 if it has not been played yet */
            QDateTime firstPlayed() const;
            void setFirstPlayed( const QDateTime &dt );

            /** Returns the number of times the track was played (what about unknown?)*/
            int playCount() const;
            void setPlayCount( int playCount );
            /** indicate to the statistics provider that a song was played */
            void played( double playedFraction, Meta::TrackPtr track );

        protected:
            virtual void save() = 0;

            QDateTime m_lastPlayed;
            QDateTime m_firstPlayed;
            double m_score;
            int m_rating;
            int m_playCount;
    };
}

#endif
