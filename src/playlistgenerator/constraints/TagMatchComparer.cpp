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

#define DEBUG_PREFIX "Constraint::TagMatch"

#include "TagMatch.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"

#include <math.h>

int
ConstraintTypes::TagMatch::Comparer::rangeNum( const double strictness, const qint64 field )
{
    if( strictness > 0.99 )
        return 0;
    const double s = strictness * strictness;
    const double w = fieldWeight( field );
    return static_cast<int>( ceil( 0.460517 * w / ( 0.1 + s ) ) );
}

double
ConstraintTypes::TagMatch::Comparer::compareNum( const double test,
                                                 MetaQueryWidget::FilterCondition condition,
                                                 double target,
                                                 const double strictness,
                                                 const qint64 field )
{
    const double weight = fieldWeight( field );

    if ( condition == MetaQueryWidget::Equals ) {
        // fuzzy equals -- within 1%, or within 0.001
        if ( ( abs( test - target ) < ( abs( test + target ) / 200.0 ) ) || ( abs( test - target ) < 0.001 ) ) {
            return 1.0;
        } else {
            return fuzzyProb( test, target, strictness, weight );
        }

    } else if ( condition == MetaQueryWidget::GreaterThan ) {
        return ( test > target ) ? 1.0 : fuzzyProb( test, target, strictness, weight );

    } else if ( condition == MetaQueryWidget::LessThan ) {
        return ( test < target ) ? 1.0 : fuzzyProb( test, target, strictness, weight );

    } else if ( condition == MetaQueryWidget::Within ) {
        target += QDateTime::currentDateTime().toTime_t();
        return ( test > target ) ? 1.0 : fuzzyProb( test, target, strictness, weight );

    } else if ( condition == MetaQueryWidget::OlderThan ) {
        target += QDateTime::currentDateTime().toTime_t();
        return ( test < target ) ? 1.0 : fuzzyProb( test, target, strictness, weight );

    } else if ( condition == MetaQueryWidget::NewerThan ) {
        target += QDateTime::currentDateTime().toTime_t();
        return ( test > target ) ? 1.0 : fuzzyProb( test, target, strictness, weight );

    } else {
        return 0.0;
    }
    return 0.0;
}

double
ConstraintTypes::TagMatch::Comparer::compareStr( const QString& test,
                                                 MetaQueryWidget::FilterCondition condition,
                                                 const QString& target )
{
    if ( condition == MetaQueryWidget::Matches ) {
        if ( test.compare( target, Qt::CaseInsensitive ) == 0 )
            return 1.0;
    } else if ( condition == MetaQueryWidget::StartsWith ) {
        if ( test.startsWith( target, Qt::CaseInsensitive ) )
            return 1.0;
    } else if ( condition == MetaQueryWidget::EndsWith ) {
        if ( test.endsWith( target, Qt::CaseInsensitive ) )
            return 1.0;
    } else if ( condition == MetaQueryWidget::Contains ) {
        if ( test.contains( target, Qt::CaseInsensitive ) )
            return 1.0;
    /*
    } else if ( comparison == CompareStrRegExp ) {
        QRegularExpression rx( target );
        if ( rx.indexIn( test ) >= 0 )
            return 1.0;
            */
    } else {
        return 0.0;
    }
    return 0.0;
}

double
ConstraintTypes::TagMatch::Comparer::compareLabels( const Meta::TrackPtr t,
                                                    MetaQueryWidget::FilterCondition condition,
                                                    const QString& target )
{
    Meta::LabelList labelList = t->labels();

    double v = 0.0;
    foreach ( Meta::LabelPtr label, labelList ) {
        // this is technically more correct ...
        // v = qMax( compare( label->prettyName(), comparison, target ), v );

        // ... but as long as compareStr() returns only 0.0 or 1.0, the following is faster:
        v = compareStr( label->prettyName(), condition, target );
        if ( v > 0.99 ) {
            return 1.0;
        }
    }

    return v;
}

double
ConstraintTypes::TagMatch::Comparer::fieldWeight( qint64 field )
{
    if( MetaQueryWidget::isDate( field ) )
        return 1209600.0;

    switch( field )
    {
    case Meta::valYear: return 8.0;
    case Meta::valTrackNr: return 5.0;
    case Meta::valDiscNr: return 0.75;
    case Meta::valBpm: return 30.0;
    case Meta::valLength: return 50.0; // 50 seconds
    case Meta::valBitrate: return 30.0;
    case Meta::valSamplerate: return 100.0;
    case Meta::valFilesize: return 100000.0;
    case Meta::valFormat: return 0.5;
    case Meta::valScore: return 20.0;
    case Meta::valRating: return 3.0;
    case Meta::valPlaycount: return 4.0;
    default: return 0.0;
    }
}

double
ConstraintTypes::TagMatch::Comparer::fuzzyProb( const double a, const double b,
                                                const double strictness, const double weight )
{
    const double s = strictness * strictness;
    return exp( -10.0 * ( 0.1 + s ) / weight * ( 1 + qAbs( a - b ) ) );
}


