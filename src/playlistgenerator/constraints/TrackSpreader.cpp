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

#define DEBUG_PREFIX "Constraint::TrackSpreader"

#include "TrackSpreader.h"

#include "playlistgenerator/Constraint.h"

#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"

#include <QtGlobal>
#include <math.h>
#include <stdlib.h>

Constraint*
ConstraintTypes::TrackSpreader::createNew( ConstraintNode* p )
{
    if ( p )
        return new TrackSpreader( p );
    else
        return 0;
}

ConstraintFactoryEntry*
ConstraintTypes::TrackSpreader::registerMe()
{
    return 0;
}

ConstraintTypes::TrackSpreader::TrackSpreader( ConstraintNode* p ) : Constraint( p ) {}

QWidget*
ConstraintTypes::TrackSpreader::editWidget() const
{
    return 0;
}

void
ConstraintTypes::TrackSpreader::toXml( QDomDocument&, QDomElement& ) const {}

Collections::QueryMaker*
ConstraintTypes::TrackSpreader::initQueryMaker( Collections::QueryMaker* qm ) const
{
    return qm;
}

double
ConstraintTypes::TrackSpreader::satisfaction( const Meta::TrackList& tl )
{
    m_trackLocations.clear();
    m_trackLocations.fill( tl );
    m_satisfaction = satisfactionFromTrackLocations( m_trackLocations );
    return m_satisfaction;
}

double
ConstraintTypes::TrackSpreader::deltaS_insert( const Meta::TrackList& tl, const Meta::TrackPtr t, const int idx ) const
{
    TrackLocations newtracklocs( m_trackLocations );
    newtracklocs.insertTrack( tl, t, idx );
    return satisfactionFromTrackLocations( newtracklocs ) - m_satisfaction;
}

double
ConstraintTypes::TrackSpreader::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int idx ) const
{
    TrackLocations newtracklocs( m_trackLocations );
    newtracklocs.replaceTrack( tl, t, idx );
    return satisfactionFromTrackLocations( newtracklocs ) - m_satisfaction;
}

double
ConstraintTypes::TrackSpreader::deltaS_delete( const Meta::TrackList& tl, const int idx ) const
{
    TrackLocations newtracklocs( m_trackLocations );
    newtracklocs.deleteTrack( tl, idx );
    return satisfactionFromTrackLocations( newtracklocs ) - m_satisfaction;
}

double
ConstraintTypes::TrackSpreader::deltaS_swap( const Meta::TrackList& tl, const int place, const int other ) const
{
    TrackLocations newtracklocs( m_trackLocations );
    newtracklocs.swapTracks( tl, place, other );
    return satisfactionFromTrackLocations( newtracklocs ) - m_satisfaction;
}

void
ConstraintTypes::TrackSpreader::insertTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int idx )
{
    m_trackLocations.insertTrack( tl, t, idx );
    m_satisfaction = satisfactionFromTrackLocations( m_trackLocations );
}

void
ConstraintTypes::TrackSpreader::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int idx )
{
    m_trackLocations.replaceTrack( tl, t, idx );
    m_satisfaction = satisfactionFromTrackLocations( m_trackLocations );
}

void
ConstraintTypes::TrackSpreader::deleteTrack( const Meta::TrackList& tl, const int idx )
{
    m_trackLocations.deleteTrack( tl, idx );
    m_satisfaction = satisfactionFromTrackLocations( m_trackLocations );
}

void
ConstraintTypes::TrackSpreader::swapTracks( const Meta::TrackList& tl, const int place, const int other )
{
    m_trackLocations.swapTracks( tl, place, other );
    m_satisfaction = satisfactionFromTrackLocations( m_trackLocations );
}

#ifndef KDE_NO_DEBUG_OUTPUT
void
ConstraintTypes::TrackSpreader::audit( const Meta::TrackList& tl ) const
{
    TrackLocations tracklocs;
    tracklocs.fill( tl );
    debug() << "audit final satisfaction" << satisfactionFromTrackLocations( tracklocs );
}
#endif

double
ConstraintTypes::TrackSpreader::satisfactionFromTrackLocations( const TrackLocations& tracklocs ) const
{
    double sum = 0.0;
    foreach( const Meta::TrackPtr t, tracklocs.keys() ) {
        sum += listToDist( tracklocs.values( t ) );
    }
    double sat = sumToSat( sum );
    return sat;
}

double
ConstraintTypes::TrackSpreader::listToDist( const QList<int>& locs ) const
{
    double sum = 0.0;
    if ( locs.size() > 1 ) {
        for ( int i = 0; i < locs.size(); i++ ) {
            for ( int j = 0; j < i; j++ ) {
                sum += distance( locs.at(i), locs.at(j) );
            }
        }
    }
    return sum;
}

double
ConstraintTypes::TrackSpreader::distance( const int a, const int b ) const
{
    if ( a == b ) {
        return 0.0;
    }

    int d = qAbs( a - b ) - 1;
    return exp( -0.05 * ( double )d );
}

double
ConstraintTypes::TrackSpreader::sumToSat( const double s ) const
{
    return 1.0 / exp( 0.1 * s );
}

/*******************************
 * TrackLocations nested class *
 *******************************/

ConstraintTypes::TrackSpreader::TrackLocations::TrackLocations( const TrackLocations& other )
{
    //debug() << "clearing tracked locations";
    m_counts = other.m_counts;
    m_locations = other.m_locations;
}

void
ConstraintTypes::TrackSpreader::TrackLocations::fill( const Meta::TrackList& tl )
{
    //DEBUG_BLOCK
    //debug() << "Filling tracked locations with the following playlist:";
    for ( int i = 0; i < tl.size() ; i++ ) {
        //debug() << "\t" << i << tl.at( i )->prettyName();
    }
    for ( int i = 0; i < tl.size() ; i++ ) {
        insertTrackInternal( tl, tl.at( i ), i );
    }
}

void
ConstraintTypes::TrackSpreader::TrackLocations::insertTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int idx )
{
    //DEBUG_BLOCK
    QMultiHash<Meta::TrackPtr, int>::iterator i;
    for ( i = m_locations.begin(); i != m_locations.end(); ++i )
        if ( i.value() >= idx )
            i.value() += 1;

    insertTrackInternal( tl, t, idx );
}

void
ConstraintTypes::TrackSpreader::TrackLocations::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr newT, const int idx )
{
    //DEBUG_BLOCK
    Meta::TrackPtr oldT = tl.at( idx );

    if ( oldT == newT )
        return;

    // insert the new
    if ( !m_counts.contains( newT ) || ( m_counts.value ( newT ) == 0 ) ) {
        //debug() << "new unique track" << newT->prettyName() << "at" << idx;
        m_counts.insert( newT, 1 );
    } else {
        m_counts[newT] += 1;
        if ( !m_locations.contains( newT ) ) {
            //debug() << "creating a new duplicate registry for" << newT->prettyName() << "at" << idx;
            m_locations.insert( newT, tl.indexOf( newT ) );
            m_locations.insert( newT, idx );
        } else {
            //debug() << "registering another instance of" << newT->prettyName() << "at" << idx;
            m_locations.insert( newT, idx );
        }
    }

    // remove the old
    if ( m_locations.contains( oldT ) ) {
        QMultiHash<Meta::TrackPtr, int>::iterator i = m_locations.find( oldT, idx );
        if ( i != m_locations.end() ) {
            m_locations.erase( i );
        } else {
            warning() << "was not able to find" << oldT->prettyName() << "at" << idx;
        }
        //debug() << "removed" << oldT->prettyName() << "from location tracking";
        if ( m_locations.count ( oldT ) == 1 ) {
            //debug() << "there's only one instance of" << oldT->prettyName() << "now, so removing it from tracking";
            i = m_locations.find( oldT );
            m_locations.erase( i );
        }
    }
    m_counts[oldT] -= 1;

    Meta::TrackList newtl( tl );
    newtl.replace( idx, newT );
    dumpTracked( newtl );
}

void
ConstraintTypes::TrackSpreader::TrackLocations::deleteTrack( const Meta::TrackList& tl, const int idx )
{
    //DEBUG_BLOCK
    Meta::TrackPtr t = tl.at( idx );
    //debug() << "removing" << t->prettyName() << "at position"<< idx;
    QMultiHash<Meta::TrackPtr, int>::iterator i = m_locations.begin();
    while ( i != m_locations.end() ) {
        if ( i.value() > idx ) {
            i.value() -= 1;
            ++i;
        } else if ( i.value() == idx ) {
            i = m_locations.erase( i );
        } else {
            ++i;
        }
    }

    if ( m_locations.contains( t ) ) {
        //debug() << "removed" << t->prettyName() << "from location tracking";
        if ( m_locations.count ( t ) == 1 ) {
            //debug() << "there's only one instance of" << t->prettyName() << "now, so removing it from tracking";
            i = m_locations.find( t );
            m_locations.erase( i );
        }
    }
    m_counts[t] -= 1;

    Meta::TrackList newtl( tl );
    newtl.removeAt( idx );
    dumpTracked( newtl );
}

void
ConstraintTypes::TrackSpreader::TrackLocations::swapTracks( const Meta::TrackList& tl, const int place, const int other )
{
    //DEBUG_BLOCK

    Meta::TrackPtr placeT = tl.at( place );
    Meta::TrackPtr otherT = tl.at( other );

    QMultiHash<Meta::TrackPtr, int>::iterator i;
    if ( m_locations.contains( placeT ) ) {
        i = m_locations.find( placeT, place );
        if ( i != m_locations.end() ) {
            i.value() = other;
        } else {
            warning() << "we weren't able to find" << placeT->prettyName() << "at" << place;
        }
    }
    if ( m_locations.contains( otherT ) ) {
        i = m_locations.find( otherT, other );
        if ( i != m_locations.end() ) {
            i.value() = place;
        } else {
            warning() << "we weren't able to find" << otherT->prettyName() << "at" << other;
        }
    }

    Meta::TrackList newtl( tl );
    newtl.swap( place, other );
    dumpTracked( newtl );
}

void
ConstraintTypes::TrackSpreader::TrackLocations::insertTrackInternal( const Meta::TrackList& tl, const Meta::TrackPtr t, const int idx )
{
    if ( !m_counts.contains( t ) || ( m_counts.value ( t ) == 0 ) ) {
        //debug() << "new unique track" << t->prettyName() << "at" << idx;
        m_counts.insert( t, 1 );
    } else {
        m_counts[t] += 1;
        if ( !m_locations.contains( t ) ) {
            //debug() << "creating a new duplicate registry for" << t->prettyName() << "at" << idx;
            int existingIdx = tl.indexOf( t );
            existingIdx += ( idx <= existingIdx ) ? 1 : 0;
            m_locations.insert( t, existingIdx  );
            m_locations.insert( t, idx );
        } else {
            //debug() << "registering another instance of" << t->prettyName() << "at" << idx;
            m_locations.insert( t, idx );
        }
    }
    Meta::TrackList newtl( tl );
    newtl.insert( idx, t );
    dumpTracked( newtl );
}

void
ConstraintTypes::TrackSpreader::TrackLocations::dumpTracked( const Meta::TrackList& tl ) const
{
    //debug() << "tracked locations are now";
    foreach ( Meta::TrackPtr t, m_locations.uniqueKeys() ) {
        //debug() << "\t" << t->prettyName() << m_locations.values( t );
        foreach ( int i, m_locations.values( t ) ) {
            if ( tl.at( i ) != t ) {
                warning() << "we've become unsynchronized!  expected" << t->prettyName() << "at" << i << "but it's not there!";
            }
        }
    }
}
