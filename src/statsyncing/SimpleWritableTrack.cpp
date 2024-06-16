/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "SimpleWritableTrack.h"

#include <QReadLocker>
#include <QWriteLocker>

using namespace StatSyncing;

SimpleWritableTrack::SimpleWritableTrack( const Meta::FieldHash &metadata,
                                          const QSet<QString> &labels )
    : SimpleTrack( metadata, labels )
{
    // Move statistics to separate container, so we don't have to lock for other metadata
    for( const qint64 metaValue : metadata.keys() )
    {
        switch( metaValue )
        {
            case Meta::valFirstPlayed:
            case Meta::valLastPlayed:
            case Meta::valRating:
            case Meta::valPlaycount:
                m_metadata.remove( metaValue );
                m_statistics.insert( metaValue, metadata[metaValue] );
                break;

            default:
                break;
        }
    }
}

SimpleWritableTrack::~SimpleWritableTrack()
{
}

QDateTime
SimpleWritableTrack::firstPlayed() const
{
    QReadLocker lock( &m_lock );
    return getDateTime( m_statistics.value( Meta::valFirstPlayed ) );
}

void
SimpleWritableTrack::setFirstPlayed( const QDateTime &firstPlayed )
{
    QWriteLocker lock( &m_lock );
    m_statistics.insert( Meta::valFirstPlayed, firstPlayed.isValid()
                                               ? firstPlayed.toSecsSinceEpoch() : 0u );
    m_changes |= Meta::valFirstPlayed;
}

QDateTime
SimpleWritableTrack::lastPlayed() const
{
    QReadLocker lock( &m_lock );
    return getDateTime( m_statistics.value( Meta::valLastPlayed ) );
}

void
SimpleWritableTrack::setLastPlayed( const QDateTime &lastPlayed )
{
    QWriteLocker lock( &m_lock );
    m_statistics.insert( Meta::valLastPlayed, lastPlayed.isValid()
                                              ? lastPlayed.toSecsSinceEpoch() : 0u );
    m_changes |= Meta::valLastPlayed;
}

int
SimpleWritableTrack::rating() const
{
    QReadLocker lock( &m_lock );
    return m_statistics.value( Meta::valRating ).toInt();
}

void
SimpleWritableTrack::setRating( int rating )
{
    QWriteLocker lock( &m_lock );
    m_statistics.insert( Meta::valRating, rating );
    m_changes |= Meta::valRating;
}

int
SimpleWritableTrack::playCount() const
{
    QReadLocker lock( &m_lock );
    return m_statistics.value( Meta::valPlaycount ).toInt();
}

void
SimpleWritableTrack::setPlayCount( int playCount )
{
    QWriteLocker lock( &m_lock );
    m_statistics.insert( Meta::valPlaycount, playCount );
    m_changes |= Meta::valPlaycount;
}

QSet<QString>
SimpleWritableTrack::labels() const
{
    QReadLocker lock( &m_lock );
    return m_labels;
}

void
SimpleWritableTrack::setLabels( const QSet<QString> &labels )
{
    QWriteLocker lock( &m_lock );
    m_labels = labels;
    m_changes |= Meta::valLabel;
}

void
SimpleWritableTrack::commit()
{
    QWriteLocker lock( &m_lock );
    doCommit( m_changes );
    m_changes = 0;
}
