/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "Constraint::Checkpoint"

#include "Checkpoint.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"

#include <KRandom>

/******************************
 * Track Matcher              *
 ******************************/
ConstraintTypes::Checkpoint::TrackMatcher::TrackMatcher( const Meta::TrackPtr& t )
    : m_trackToMatch( t )
{
}

QList<int>
ConstraintTypes::Checkpoint::TrackMatcher::find( const Meta::TrackList& tl ) const
{
    QList<int> positions;
    for ( int i = 0; i < tl.length(); i++ ) {
        if ( tl.at( i ) == m_trackToMatch ) {
            positions << i;
        }
    }

    return positions;
}

bool
ConstraintTypes::Checkpoint::TrackMatcher::match( const Meta::TrackPtr& t ) const
{
    return ( t == m_trackToMatch );
}

Meta::TrackPtr
ConstraintTypes::Checkpoint::TrackMatcher::suggest( const Meta::TrackList& ) const
{
    return m_trackToMatch;
}

/******************************
 * Artist Matcher             *
 ******************************/
ConstraintTypes::Checkpoint::ArtistMatcher::ArtistMatcher( const Meta::ArtistPtr& a )
    : m_artistToMatch( a )
{
}

QList<int>
ConstraintTypes::Checkpoint::ArtistMatcher::find( const Meta::TrackList& tl ) const
{
    QList<int> positions;
    for ( int i = 0; i < tl.length(); i++ ) {
        if ( tl.at( i )->artist() == m_artistToMatch ) {
            positions << i;
        }
    }

    return positions;
}

bool
ConstraintTypes::Checkpoint::ArtistMatcher::match( const Meta::TrackPtr& t ) const
{
    return ( t->artist() == m_artistToMatch );
}

Meta::TrackPtr
ConstraintTypes::Checkpoint::ArtistMatcher::suggest( const Meta::TrackList& domain ) const
{
    Meta::TrackList possibilities;
    foreach ( const Meta::TrackPtr d, domain ) {
        if ( d->artist() == m_artistToMatch )
            possibilities.append( d );
    }

    return possibilities.at( KRandom::random() % possibilities.size() );
}

/******************************
 * Album Matcher              *
 ******************************/
ConstraintTypes::Checkpoint::AlbumMatcher::AlbumMatcher( const Meta::AlbumPtr& l )
    : m_albumToMatch( l )
{
}

QList<int>
ConstraintTypes::Checkpoint::AlbumMatcher::find( const Meta::TrackList& tl ) const
{
    QList<int> positions;
    for ( int i = 0; i < tl.length(); i++ ) {
        if ( tl.at( i )->album() == m_albumToMatch ) {
            positions << i;
        }
    }

    return positions;
}

bool
ConstraintTypes::Checkpoint::AlbumMatcher::match( const Meta::TrackPtr& t ) const
{
    return ( t->album() == m_albumToMatch );
}

Meta::TrackPtr
ConstraintTypes::Checkpoint::AlbumMatcher::suggest( const Meta::TrackList& domain ) const
{
    Meta::TrackList possibilities;
    foreach ( const Meta::TrackPtr d, domain ) {
        if ( d->album() == m_albumToMatch )
            possibilities.append( d );
    }

    return possibilities.at( KRandom::random() % possibilities.size() );
}

/******************************
 * Boundary Tracking Class    *
 ******************************/
ConstraintTypes::Checkpoint::BoundaryTracker::BoundaryTracker( const Meta::TrackList& tl )
{
    qint64 runningEndpoint = 0;
    foreach ( const Meta::TrackPtr t, tl ) {
        runningEndpoint += t->length();
        m_endpoints << runningEndpoint;
    }
}

ConstraintTypes::Checkpoint::BoundaryTracker::BoundaryTracker( const QList<qint64>& otherBounds )
    : m_endpoints( otherBounds )
{
}

ConstraintTypes::Checkpoint::BoundaryTracker::~BoundaryTracker()
{
}

QPair<qint64, qint64>
ConstraintTypes::Checkpoint::BoundaryTracker::getBoundariesAt( int idx ) const
{
    QPair<qint64, qint64> boundary;
    if ( idx > 0 && ( idx < m_endpoints.length() ) )
        boundary.first = m_endpoints.at( idx - 1 );
    else
        boundary.first = 0;

    if ( idx >= 0 && ( idx < m_endpoints.length() ) )
        boundary.second = m_endpoints.at( idx );
    else
        boundary.second = 0;

    return boundary;
}

int
ConstraintTypes::Checkpoint::BoundaryTracker::indexAtTime( qint64 time ) const
{
    // TODO: there may be a faster way to figure this out
    if ( time < m_endpoints.at( 0 ) )
        return 0;

    for ( int i=1; i < m_endpoints.length(); i++ ) {
        if ( ( time >= m_endpoints.at( i-1 ) ) && ( time < m_endpoints.at( i ) ) ) {
            return i;
        }
    }

    return m_endpoints.length();
}

void
ConstraintTypes::Checkpoint::BoundaryTracker::insertTrack( const Meta::TrackPtr t, const int i )
{
    if ( i < 0 || ( i > m_endpoints.length() ) )
        return;

    qint64 start = 0;
    if ( i > 0 )
        start = m_endpoints.at( i - 1 );

    qint64 delta = t->length();

    m_endpoints.insert( i, start + delta );

    for ( int j = i + 1; j < m_endpoints.length(); j++ ) {
        m_endpoints[j] += delta;
    }
}

void
ConstraintTypes::Checkpoint::BoundaryTracker::replaceTrack( const Meta::TrackPtr t, const int i )
{
    if ( i < 0 || ( i > m_endpoints.length() ) )
        return;

    qint64 delta = t->length() - m_endpoints.at(i);

    for ( int j = i; j < m_endpoints.length(); j++ ) {
        m_endpoints[j] += delta;
    }
}

void
ConstraintTypes::Checkpoint::BoundaryTracker::deleteTrack( const int i )
{
    if ( i < 0 || ( i > m_endpoints.length() ) )
        return;

    QPair<qint64, qint64> b = getBoundariesAt( i );
    qint64 delta = b.second - b.first;

    m_endpoints.removeAt( i );

    for ( int j = i; j < m_endpoints.length(); j++ ) {
        m_endpoints[j] -= delta;
    }
}

void
ConstraintTypes::Checkpoint::BoundaryTracker::swapTracks( const int i, const int j )
{
    if ( i < 0 || ( i > m_endpoints.length() ) )
        return;

    if ( j < 0 || ( j > m_endpoints.length() ) )
        return;

    if ( i == j )
        return;

    int lower = ( i < j ) ? i : j;
    int upper = ( i > j ) ? i : j;

    QPair<qint64, qint64> lowBounds = getBoundariesAt( lower );
    QPair<qint64, qint64> upBounds = getBoundariesAt( upper );

    qint64 delta = ( upBounds.second - upBounds.first ) - ( lowBounds.second - lowBounds.first );

    for ( int k = lower; k <= upper; k++ ) {
        m_endpoints[k] += delta;
    }
}

ConstraintTypes::Checkpoint::BoundaryTracker*
ConstraintTypes::Checkpoint::BoundaryTracker::cloneAndInsert( const Meta::TrackPtr t, const int i ) const
{
    Checkpoint::BoundaryTracker* bt = new Checkpoint::BoundaryTracker( m_endpoints );
    bt->insertTrack( t, i );

    return bt;
}

ConstraintTypes::Checkpoint::BoundaryTracker*
ConstraintTypes::Checkpoint::BoundaryTracker::cloneAndReplace( const Meta::TrackPtr t, const int i ) const
{
    Checkpoint::BoundaryTracker* bt = new Checkpoint::BoundaryTracker( m_endpoints );
    bt->replaceTrack( t, i );

    return bt;
}

ConstraintTypes::Checkpoint::BoundaryTracker*
ConstraintTypes::Checkpoint::BoundaryTracker::cloneAndDelete( const int i ) const
{
    Checkpoint::BoundaryTracker* bt = new Checkpoint::BoundaryTracker( m_endpoints );
    bt->deleteTrack( i );

    return bt;
}

ConstraintTypes::Checkpoint::BoundaryTracker*
ConstraintTypes::Checkpoint::BoundaryTracker::cloneAndSwap( const int i, const int j ) const
{
    Checkpoint::BoundaryTracker* bt = new Checkpoint::BoundaryTracker( m_endpoints );
    bt->swapTracks( i, j );

    return bt;
}

void
ConstraintTypes::Checkpoint::BoundaryTracker::audit( const Meta::TrackList& tl ) const
{
    qint64 runningEndpoint_tl = 0;

    if ( tl.length() != m_endpoints.length() ) {
        warning() << "track list length and boundary tracker length disagree!" << tl.length() << m_endpoints.length();
        return;
    }

    Meta::TrackList::const_iterator itr_tl = tl.begin();
    QList<qint64>::const_iterator itr_ep = m_endpoints.begin();

    while ( itr_tl != tl.end() && itr_ep != m_endpoints.end() ) {
        runningEndpoint_tl += (*itr_tl)->length();
        if ( runningEndpoint_tl != *itr_ep ) {
            warning() << "endpoints have gotten unsynchronized!" << runningEndpoint_tl << *itr_ep;
        }
        ++itr_ep;
        ++itr_tl;
    }
}