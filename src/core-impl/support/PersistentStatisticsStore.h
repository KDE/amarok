/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2012 Matěj Lait <matej@laitl.cz>                                       *
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

#ifndef PERSISTENTSTATISTICSSTORE_H
#define PERSISTENTSTATISTICSSTORE_H

#include "amarok_export.h"
#include "core/meta/Observer.h"
#include "core/meta/Statistics.h"

#include <QDateTime>
#include <QReadWriteLock>

/**
 * Base class for all permanent statistics storage providers. Use one of the sublassed if
 * your collection cannot store statistics (rating, play count..) natively, but you still
 * want to provide the functionality.
 *
 * All subclasses automatically call notifyObservers() on your track when the statistics
 * change. PersistentStatisticsStore uses some trickery not to hold reference to your
 * track to avoid circular reference counting. PersistentStatisticsStore can even deal
 * with your track being destroyed and is implemented in thread-safe way. You should
 * store is as StatisticsPtr (a AmarokSharedPointer) in your Track class.
 */
class AMAROK_EXPORT PersistentStatisticsStore : public Meta::Statistics, private Meta::Observer
{
    public:
        /**
         * Create persistent statistics store of @param track statistics. @p track may
         * not be null.
         *
         * This methods takes plain pointer so that you can call it in the Track
         * constructor without AmarokSharedPointer deleting it right away.
         */
        explicit PersistentStatisticsStore( Meta::Track *track );
        virtual ~PersistentStatisticsStore();

        // Meta::Statistics methods
        virtual double score() const;
        virtual void setScore( double newScore );

        virtual int rating() const;
        virtual void setRating( int newRating );

        virtual QDateTime lastPlayed() const;
        virtual void setLastPlayed( const QDateTime &dt );

        virtual QDateTime firstPlayed() const;
        virtual void setFirstPlayed( const QDateTime &dt );

        virtual int playCount() const;
        virtual void setPlayCount( int playCount );

        virtual void beginUpdate();
        virtual void endUpdate();

        // Meta::Observer methods

        /**
         * Notice that the linked track was destroyed.
         */
        virtual void entityDestroyed();

    protected:
        virtual void save() = 0; // called with m_lock locked for writing!

        static const QString s_sqlDateFormat;

        Meta::Track *m_track; // plain pointer not to hold reference
        QDateTime m_lastPlayed;
        QDateTime m_firstPlayed;
        double m_score;
        int m_rating;
        int m_playCount;
        mutable QReadWriteLock m_lock; // lock procecting access to fields.

    private:
        void commitIfInNonBatchUpdate(); // must be called with the m_lock locked for writing

        /**
         * Number of current batch operations started by @see beginUpdate() and not
         * yet ended by @see endUpdate(). Must only be accessed with m_track held.
         */
        int m_batch;
};

#endif // PERSISTENTSTATISTICSSTORE_H
