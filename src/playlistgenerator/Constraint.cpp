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

#define DEBUG_PREFIX "APG::Constraint"

#include "Constraint.h"

#include "core/support/Debug.h"

#include <QString>

const double Constraint::magicStrictnessWeight = 3.0;

Constraint::Constraint( ConstraintNode* p ) : ConstraintNode( p ) {}

double Constraint::compare( const QString& a, const int comparison, const QString& b, double strictness ) const
{
    Q_UNUSED( strictness ); // strictness is (currently) meaningless for string comparisons
    if ( comparison == CompareStrEquals ) {
        if ( a.compare( b, Qt::CaseInsensitive ) == 0 )
            return 1.0;
    } else if ( comparison == CompareStrStartsWith ) {
        if ( a.startsWith( b, Qt::CaseInsensitive ) )
            return 1.0;
    } else if ( comparison == CompareStrEndsWith ) {
        if ( a.endsWith( b, Qt::CaseInsensitive ) )
            return 1.0;
    } else if ( comparison == CompareStrContains ) {
        if ( a.contains( b, Qt::CaseInsensitive ) )
            return 1.0;
    } else if ( comparison == CompareStrRegExp ) {
        QRegExp rx( b );
        if ( rx.indexIn( a ) >= 0 )
            return 1.0;
    } else {
        return 0.0;
    }
    return 0.0;
}
