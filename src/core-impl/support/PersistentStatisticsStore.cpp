/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "PersistentStatisticsStore.h"

#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

const QString PersistentStatisticsStore::s_sqlDateFormat( QStringLiteral("yyyy-MM-dd hh:mm:ss") );

PersistentStatisticsStore::PersistentStatisticsStore( Meta::Track *track )
    : m_track( track )
    , m_score( 0.0 )
    , m_rating( 0 )
    , m_playCount( 0 )
    , m_batch( 0 )
{
    subscribeTo( track ); // notice the track being deleted
}

PersistentStatisticsStore::~PersistentStatisticsStore()
{
}

double
PersistentStatisticsStore::score() const
{
    QReadLocker locker( &m_lock );
    return m_score;
}

void
PersistentStatisticsStore::setScore( double newScore )
{
    QWriteLocker locker( &m_lock );
    m_score = newScore;
    commitIfInNonBatchUpdate();
}

int
PersistentStatisticsStore::rating() const
{
    // no lock, int fetching is atomic
    return m_rating;
}

void
PersistentStatisticsStore::setRating( int newRating )
{
    QWriteLocker locker( &m_lock );
    m_rating = newRating;
    commitIfInNonBatchUpdate();
}

QDateTime
PersistentStatisticsStore::lastPlayed() const
{
    QReadLocker locker( &m_lock );
    return m_lastPlayed;
}

void
PersistentStatisticsStore::setLastPlayed( const QDateTime &dt )
{
    QWriteLocker locker( &m_lock );
    m_lastPlayed = dt;
    commitIfInNonBatchUpdate();
}

QDateTime
PersistentStatisticsStore::firstPlayed() const
{
    QReadLocker locker( &m_lock );
    return m_firstPlayed;
}

void
PersistentStatisticsStore::setFirstPlayed( const QDateTime &dt )
{
    QWriteLocker locker( &m_lock );
    m_firstPlayed = dt;
    commitIfInNonBatchUpdate();
}

int
PersistentStatisticsStore::playCount() const
{
    // no lock, in fetching is atomic
    return m_playCount;
}

void
PersistentStatisticsStore::setPlayCount( int playCount )
{
    QWriteLocker locker( &m_lock );
    m_playCount = playCount;
    commitIfInNonBatchUpdate();
}

void PersistentStatisticsStore::beginUpdate()
{
    QWriteLocker locker( &m_lock );
    m_batch++;
}

void PersistentStatisticsStore::endUpdate()
{
    QWriteLocker locker( &m_lock );
    Q_ASSERT( m_batch > 0 );
    m_batch--;
    commitIfInNonBatchUpdate();
}

void
PersistentStatisticsStore::entityDestroyed()
{
    QWriteLocker locker( &m_lock );
    m_track = nullptr; // prevent stale pointer
}

void
PersistentStatisticsStore::commitIfInNonBatchUpdate()
{
    if( m_batch > 0 )
        return;

    save();
    if( m_track )
    {
        m_lock.unlock(); // better call the notify without lock hold to prevent deadlocks
        m_track->notifyObservers();
        m_lock.lockForWrite();
    }
}
