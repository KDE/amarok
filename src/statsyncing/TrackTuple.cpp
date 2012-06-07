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

#include "TrackTuple.h"

#include "MetaValues.h"
#include "statsyncing/Options.h"

using namespace StatSyncing;

TrackTuple::TrackTuple()
    : m_ratingProvider( 0 )
{
}

void
TrackTuple::insert( const Provider *provider, const TrackPtr &track )
{
    m_map.insert( provider, track );
}

QList<const Provider *>
TrackTuple::providers() const
{
    return m_map.keys();
}

const Provider *
TrackTuple::provider( int i ) const
{
    return m_map.keys().value( i );
}

TrackPtr
TrackTuple::track( const Provider *provider ) const
{
    Q_ASSERT( m_map.contains( provider ) );
    return m_map.value( provider );
}

int
TrackTuple::count() const
{
    return m_map.count();
}

bool
TrackTuple::isEmpty() const
{
    return m_map.isEmpty();
}

bool
TrackTuple::fieldUpdated( qint64 field, const Options &options, const Provider *provider ) const
{
    if( isEmpty() ||
        ( provider && !m_map.contains( provider ) ) ||
        !(options.syncedFields() & field) )
    {
        return false;
    }

    switch( field )
    {
        case Meta::valRating:
        {
            int rating = syncedRating( options );
            if( rating < 0 )
                return false; // unresolved conflict, not going to write that
            if( provider )
                return track( provider )->rating() != rating;

            foreach( TrackPtr track, m_map )
            {
                if( track->rating() != rating )
                    return true;
            }
            return false;
        }

        case Meta::valFirstPlayed:
        {
            QDateTime firstPlayed = syncedFirstPlayed( options );
            if( provider )
                return track( provider )->firstPlayed() != firstPlayed;

            foreach( TrackPtr track, m_map )
            {
                if( track->firstPlayed() != firstPlayed )
                    return true;
            }
            return false;
        }

        case Meta::valLastPlayed:
        {
            QDateTime lastPlayed = syncedLastPlayed( options );
            if( provider )
                return track( provider )->lastPlayed() != lastPlayed;

            foreach( TrackPtr track, m_map )
            {
                if( track->lastPlayed() != lastPlayed )
                    return true;
            }
            return false;
        }

        case Meta::valPlaycount:
        {
            int playcount = syncedPlaycount( options );
            if( provider )
                return track( provider )->playcount() != playcount;

            foreach( TrackPtr track, m_map )
            {
                if( track->playcount() != playcount )
                    return true;
            }
            return false;
        }

        case Meta::valLabel:
        {
            QSet<QString> labels = syncedLabels( options );
            if( provider )
                return track( provider )->labels() != labels;

            foreach( TrackPtr track, m_map )
            {
                if( track->labels() != labels )
                    return true;
            }
            return false;
        }
    }
    return false;
}

bool
TrackTuple::hasUpdate( const Options &options ) const
{
    static const QList<qint64> fields = QList<qint64>() << Meta::valRating
        << Meta::valFirstPlayed << Meta::valLastPlayed << Meta::valPlaycount
        << Meta::valLabel;

    foreach( qint64 field, fields )
    {
        if( fieldUpdated( field, options ) )
            return true;
    }
    return false;
}

bool
TrackTuple::hasConflict( const Options &options ) const
{
    if( isEmpty() )
        return false;
    int firstRating = track( provider( 0 ) )->rating();
    QMapIterator<const Provider *, TrackPtr> it( m_map );
    it.next(); // skip the first one
    while( it.hasNext() )
    {
        it.next();
        if( it.value()->rating() != firstRating )
            return true;
    }
    return false;
}

int
TrackTuple::syncedRating( const Options &options ) const
{
    if( isEmpty() || !(options.syncedFields() & Meta::valRating) )
        return 0;
    if( m_ratingProvider ) // a provider has been chosen
        return track( m_ratingProvider )->rating();
    if( hasConflict( options ) )
        return -1;
    // all ratings are the same
    return track( provider( 0 ) )->rating();
}

QDateTime
TrackTuple::syncedFirstPlayed( const Options &options ) const
{
    QDateTime first;
    if( isEmpty() || !(options.syncedFields() & Meta::valFirstPlayed) )
        return first;
    foreach( TrackPtr track, m_map )
    {
        if( !first.isValid() || track->firstPlayed() < first )
            first = track->firstPlayed();
    }
    return first;
}

QDateTime
TrackTuple::syncedLastPlayed( const Options &options ) const
{
    QDateTime last;
    if( isEmpty() || !(options.syncedFields() & Meta::valLastPlayed) )
        return last;
    foreach( TrackPtr track, m_map )
    {
        if( !last.isValid() || track->lastPlayed() > last )
            last = track->lastPlayed();
    }
    return last;
}

int
TrackTuple::syncedPlaycount( const Options &options ) const
{
    int max = 0;
    if( isEmpty() || !(options.syncedFields() & Meta::valPlaycount) )
        return max;
    foreach( TrackPtr track, m_map )
    {
        max = qMax( max, track->playcount() );
    }
    return max;
}

QSet<QString>
TrackTuple::syncedLabels( const Options &options ) const
{
    QSet<QString> labels;
    if( isEmpty() || !(options.syncedFields() & Meta::valLabel) )
        return labels;
    foreach( TrackPtr track, m_map )
    {
        // TODO: this is just basic "unite" synchronization, add more options
        labels |= track->labels();
    }
    return labels;
}
