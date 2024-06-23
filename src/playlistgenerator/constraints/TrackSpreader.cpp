/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#include "core/meta/Meta.h"
#include "playlistgenerator/Constraint.h"

#include <QHash>

#include <cmath>
#include <cstdlib>

Constraint*
ConstraintTypes::TrackSpreader::createNew( ConstraintNode* p )
{
    if ( p )
        return new TrackSpreader( p );
    else
        return nullptr;
}

ConstraintFactoryEntry*
ConstraintTypes::TrackSpreader::registerMe()
{
    return nullptr;
}

ConstraintTypes::TrackSpreader::TrackSpreader( ConstraintNode* p ) : Constraint( p ) {
}

QWidget*
ConstraintTypes::TrackSpreader::editWidget() const
{
    return nullptr;
}

void
ConstraintTypes::TrackSpreader::toXml( QDomDocument&, QDomElement& ) const {}

double
ConstraintTypes::TrackSpreader::satisfaction( const Meta::TrackList& tl ) const
{
    QMultiHash<Meta::TrackPtr, int> locations;
    double dist = 0.0;
    for ( int i = 0; i < tl.size(); i++ ) {
        Meta::TrackPtr t = tl.value( i );
        if ( locations.contains( t ) ) {
            for( int j : locations.values( t ) ) {
                dist += distance( i, j );
            }
        }
        locations.insert( tl.value( i ), i );
    }

    return 1.0 / exp( 0.1 * dist );
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
