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

#ifndef META_STATISTICS_H
#define META_STATISTICS_H

#include "core/amarokcore_export.h"

#include "AmarokSharedPointer.h"

class QDateTime;

namespace Meta
{
    /**
     * Interface that can be provided by tracks that can support play-related statistics:
     * rating, score, first/last played, play count.
     *
     * This class is memory-managed exclusively using AmarokSharedPointers: always use
     * StatisticsPtr to store or pass pointer to this class. This class must be
     * implemented in a reentrant manner. Additionally, underlying Meta::Track must be
     * thread-safe -- if you return same instance of Statistics every time then it means
     * that even the instance must be thread-safe.
     */
    class AMAROKCORE_EXPORT Statistics : public virtual QSharedData // virtual inheritance
    // so that Track implementations can inherit both Meta::Track and Meta::Statistics
    {
        public:
            virtual ~Statistics();

            /**
             * Return the score of this track in range 0 .. 100. Default implementation
             * returns 0.
             */
            virtual double score() const;

            /**
             * Set score of this track. If you touch more fields, consider using
             * @see beginUpdate(), @see endUpdate()
             */
            virtual void setScore( double newScore );

            /**
             * Return the rating of this track as a count of half-stars in range 0 .. 10.
             * Default implementation returns 0.
             */
            virtual int rating() const;

            /**
             * Set rating of this track. If you touch more fields, consider using
             * @see beginUpdate(), @see endUpdate()
             */
            virtual void setRating( int newRating );

            /**
             * Return the time the song was last played, or an invalid QDateTime if it
             * has not been played yet (done by the default implementation).
             */
            virtual QDateTime lastPlayed() const;

            /**
             * Set the last played time. @param date may be an invalid date to reset
             * the date to "never played". If you touch more fields, consider using
             * @see beginUpdate(), @see endUpdate()
             */
            virtual void setLastPlayed( const QDateTime &date );

            /**
             * Return the time the song was first played, or an invalid QDateTime if it
             * has not been played yet (done by the default implementation).
             */
            virtual QDateTime firstPlayed() const;

            /**
             * Set the first played time. @param date may be an invalid date to reset
             * the date to "never played". If you touch more fields, consider using
             * @see beginUpdate(), @see endUpdate()
             */
            virtual void setFirstPlayed( const QDateTime &date );

            /**
             * Returns the number of times the track was played, 0 id it is unknown.
             * Default implementation returns 0.
             */
            virtual int playCount() const;

            /**
             * Return play count on device since it has been last connected to a computer.
             * This number is _already_ _included_ in playCount()! Subclasses returning
             * nonzero must also implement setPlayCount() and setting play count must
             * reset recent playcount to 0. Default implementation returns 0.
             */
            virtual int recentPlayCount() const;

            /**
             * Set play count. If you touch more fields, consider using
             * @see beginUpdate(), @see endUpdate()
             */
            virtual void setPlayCount( int newPlayCount );

            /**
             * If you call multiple set*() methods, enclose the calls in beginUpdate() ...
             * endUpdate(); to allow more efficient processing.
             */
            virtual void beginUpdate();

            /**
             * If you call multiple set*() methods, enclose the calls in beginUpdate() ...
             * endUpdate(); to allow more efficient processing.
             */
            virtual void endUpdate();
    };

    typedef AmarokSharedPointer<Statistics> StatisticsPtr;
    typedef AmarokSharedPointer<const Statistics> ConstStatisticsPtr;
}

#endif // META_STATISTICS_H
