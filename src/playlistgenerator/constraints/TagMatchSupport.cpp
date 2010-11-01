/****************************************************************************************
 * Copyright (c) 2010 Soren Harward <stharward@gmail.com>                               *
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

#define DEBUG_PREFIX "Constraint::TagMatchSupport"

#include "TagMatch.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"

#include <math.h>

ConstraintTypes::TagMatchFieldsModel::TagMatchFieldsModel()
{
    m_fieldNames << "url"
                 << "title"
                 << "artist name"
                 << "album name"
                 << "genre"
                 << "composer"
                 << "year"
                 << "comment"
                 << "track number"
                 << "disc number"
                 << "length"
                 << "score"
                 << "rating"
                 << "create date"
                 << "first played"
                 << "last played"
                 << "play count"
                 << "label";

    m_fieldTypes.insert( "url", TagMatch::FieldTypeString );
    m_fieldTypes.insert( "title", TagMatch::FieldTypeString );
    m_fieldTypes.insert( "artist name", TagMatch::FieldTypeString );
    m_fieldTypes.insert( "album name", TagMatch::FieldTypeString );
    m_fieldTypes.insert( "genre", TagMatch::FieldTypeString );
    m_fieldTypes.insert( "composer", TagMatch::FieldTypeString );
    m_fieldTypes.insert( "year", TagMatch::FieldTypeInt );
    m_fieldTypes.insert( "comment", TagMatch::FieldTypeString );
    m_fieldTypes.insert( "track number", TagMatch::FieldTypeInt );
    m_fieldTypes.insert( "disc number", TagMatch::FieldTypeInt );
    m_fieldTypes.insert( "length", TagMatch::FieldTypeInt );
    m_fieldTypes.insert( "create date", TagMatch::FieldTypeDate);
    m_fieldTypes.insert( "score", TagMatch::FieldTypeInt );
    m_fieldTypes.insert( "rating", TagMatch::FieldTypeInt );
    m_fieldTypes.insert( "first played", TagMatch::FieldTypeDate );
    m_fieldTypes.insert( "last played", TagMatch::FieldTypeDate );
    m_fieldTypes.insert( "play count", TagMatch::FieldTypeInt );
    m_fieldTypes.insert( "label", TagMatch::FieldTypeString );

    m_fieldMetaValues.insert( "url", Meta::valUrl );
    m_fieldMetaValues.insert( "title", Meta::valTitle );
    m_fieldMetaValues.insert( "artist name", Meta::valArtist );
    m_fieldMetaValues.insert( "album name", Meta::valAlbum );
    m_fieldMetaValues.insert( "genre", Meta::valGenre );
    m_fieldMetaValues.insert( "composer", Meta::valComposer );
    m_fieldMetaValues.insert( "year", Meta::valYear );
    m_fieldMetaValues.insert( "comment", Meta::valComment );
    m_fieldMetaValues.insert( "track number", Meta::valTrackNr );
    m_fieldMetaValues.insert( "disc number", Meta::valDiscNr );
    m_fieldMetaValues.insert( "length", Meta::valLength );
    m_fieldMetaValues.insert( "create date", Meta::valCreateDate);
    m_fieldMetaValues.insert( "score", Meta::valScore );
    m_fieldMetaValues.insert( "rating", Meta::valRating );
    m_fieldMetaValues.insert( "first played", Meta::valFirstPlayed );
    m_fieldMetaValues.insert( "last played", Meta::valLastPlayed );
    m_fieldMetaValues.insert( "play count", Meta::valPlaycount );
    m_fieldMetaValues.insert( "label", Meta::valLabel );

    m_fieldPrettyNames.insert( "url", i18n("url") );
    m_fieldPrettyNames.insert( "title", i18n("title") );
    m_fieldPrettyNames.insert( "artist name", i18n("artist name") );
    m_fieldPrettyNames.insert( "album name", i18n("album name") );
    m_fieldPrettyNames.insert( "genre", i18n("genre") );
    m_fieldPrettyNames.insert( "composer", i18n("composer") );
    m_fieldPrettyNames.insert( "year", i18n("year") );
    m_fieldPrettyNames.insert( "comment", i18n("comment") );
    m_fieldPrettyNames.insert( "track number", i18n("track number") );
    m_fieldPrettyNames.insert( "disc number", i18n("disc number") );
    m_fieldPrettyNames.insert( "length", i18n("length") );
    m_fieldPrettyNames.insert( "create date", i18n("added to collection") );
    m_fieldPrettyNames.insert( "score", i18n("score") );
    m_fieldPrettyNames.insert( "rating", i18n("rating") );
    m_fieldPrettyNames.insert( "first played", i18n("first played") );
    m_fieldPrettyNames.insert( "last played", i18n("last played") );
    m_fieldPrettyNames.insert( "play count", i18n("play count") );
    m_fieldPrettyNames.insert( "label", i18n("label") );
}

ConstraintTypes::TagMatchFieldsModel::~TagMatchFieldsModel()
{
}

int
ConstraintTypes::TagMatchFieldsModel::rowCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent )
    return m_fieldNames.length();
}

QVariant
ConstraintTypes::TagMatchFieldsModel::data( const QModelIndex& idx, int role ) const
{
    QString s = m_fieldNames.at( idx.row() );

    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return QVariant( m_fieldPrettyNames.value( s ) );
            break;
        default:
            return QVariant();
    }
    return QVariant();
}

bool
ConstraintTypes::TagMatchFieldsModel::contains( const QString& s ) const
{
    return m_fieldNames.contains( s );
}

int
ConstraintTypes::TagMatchFieldsModel::index_of( const QString& s ) const
{
    return m_fieldNames.indexOf( s );
}

QString
ConstraintTypes::TagMatchFieldsModel::field_at( int idx ) const
{
    if ( ( idx >= 0 ) && ( idx < m_fieldNames.length() ) )
        return m_fieldNames.at( idx );
    else
        return QString();
}

qint64
ConstraintTypes::TagMatchFieldsModel::meta_value_of( const QString& f ) const
{
    return m_fieldMetaValues.value( f );
}

QString
ConstraintTypes::TagMatchFieldsModel::pretty_name_of( const QString& f ) const
{
    return m_fieldPrettyNames.value( f );
}

ConstraintTypes::TagMatch::FieldTypes
ConstraintTypes::TagMatchFieldsModel::type_of( const QString& f ) const
{
    return m_fieldTypes.value( f );
}

/*************************************
**************************************/

ConstraintTypes::TagMatch::Comparer::Comparer() : m_dateWeight( 1209600.0 )
{
    m_numFieldWeight.insert( Meta::valYear, 8.0 );
    m_numFieldWeight.insert( Meta::valTrackNr, 5.0 );
    m_numFieldWeight.insert( Meta::valDiscNr, 0.75 );
    m_numFieldWeight.insert( Meta::valLength, 100000.0 );
    m_numFieldWeight.insert( Meta::valScore, 20.0 );
    m_numFieldWeight.insert( Meta::valRating, 3.0 );
    m_numFieldWeight.insert( Meta::valPlaycount, 4.0 );
}

ConstraintTypes::TagMatch::Comparer::~Comparer()
{
}

double
ConstraintTypes::TagMatch::Comparer::compareNum( const double test,
                                                 const int comparison,
                                                 const double target,
                                                 const double strictness,
                                                 const qint64 field ) const
{
    const double weight = m_numFieldWeight.value( field );

    if ( comparison == CompareNumEquals ) {
        // fuzzy equals -- within 1%, or within 0.001
        if ( ( abs( test - target ) < ( abs( test + target ) / 200.0 ) ) || ( abs( test - target ) < 0.001 ) ) {
            return 1.0;
        } else {
            return fuzzyProb( test, target, strictness, weight );
        }
    } else if ( comparison == CompareNumGreaterThan ) {
        return ( test > target ) ? 1.0 : fuzzyProb( test, target, strictness, weight );
    } else if ( comparison == CompareNumLessThan ) {
        return ( test < target ) ? 1.0 : fuzzyProb( test, target, strictness, weight );
    } else {
        return 0.0;
    }
    return 0.0;
}

double
ConstraintTypes::TagMatch::Comparer::compareStr( const QString& test,
                                                 const int comparison,
                                                 const QString& target ) const
{
    if ( comparison == CompareStrEquals ) {
        if ( test.compare( target, Qt::CaseInsensitive ) == 0 )
            return 1.0;
    } else if ( comparison == CompareStrStartsWith ) {
        if ( test.startsWith( target, Qt::CaseInsensitive ) )
            return 1.0;
    } else if ( comparison == CompareStrEndsWith ) {
        if ( test.endsWith( target, Qt::CaseInsensitive ) )
            return 1.0;
    } else if ( comparison == CompareStrContains ) {
        if ( test.contains( target, Qt::CaseInsensitive ) )
            return 1.0;
    } else if ( comparison == CompareStrRegExp ) {
        QRegExp rx( target );
        if ( rx.indexIn( test ) >= 0 )
            return 1.0;
    } else {
        return 0.0;
    }
    return 0.0;
}


double
ConstraintTypes::TagMatch::Comparer::compareDate( const QDateTime test,
                                                  const int comparison,
                                                  const QVariant& targetVar,
                                                  const double strictness ) const
{
    const double weight = m_dateWeight;

    int comp = comparison;
    QDateTime target;
    if ( comparison == CompareDateWithin ) {
        comp = CompareDateAfter;
        QDateTime now = QDateTime::currentDateTime();
        DateRange r = targetVar.value<DateRange>();
        switch ( r.second ) {
            case 0:
                target = now.addDays( -1 * r.first );
                break;
            case 1:
                target = now.addMonths( -1 * r.first );
                break;
            case 2:
                target = now.addYears( -1 * r.first );
                break;
            default:
                break;
        }
    } else {
        target = QDateTime::fromTime_t(targetVar.value<uint>());
    }

    const double dte = static_cast<double>(test.toTime_t());
    const double dta = static_cast<double>(target.toTime_t());
    if ( comp == CompareDateOn ) {
        // fuzzy equals -- within 1%, or within 10.0
        if ( ( abs( dte - dta ) < ( abs( dte + dta ) / 200.0 ) ) || ( abs( dte - dta ) < 10.0 ) ) {
            return 1.0;
        } else {
            return fuzzyProb( dte, dta, strictness, weight );
        }
    } else if ( comp == CompareDateAfter ) {
        return ( test > target ) ? 1.0 : fuzzyProb( dte, dta, strictness, weight );
    } else if ( comp == CompareDateBefore ) {
        return ( test < target ) ? 1.0 : fuzzyProb( dte, dta, strictness, weight );
    } else {
        return 0.0;
    }
    return 0.0;
}

double
ConstraintTypes::TagMatch::Comparer::compareLabels( const Meta::TrackPtr t,
                                                    const int comparison,
                                                    const QString& target ) const
{
    Meta::LabelList labelList = t->labels();

    double v = 0.0;
    foreach ( Meta::LabelPtr label, labelList ) {
        // this is technically more correct ...
        // v = qMax( compare( label->prettyName(), comparison, target ), v );

        // ... but as long as compareStr() returns only 0.0 or 1.0, the following is faster:
        v = compareStr( label->prettyName(), comparison, target );
        if ( v > 0.99 ) {
            return 1.0;
        }
    }

    return v;
}

uint
ConstraintTypes::TagMatch::Comparer::rangeDate( const double strictness ) const
{
    if ( strictness > 0.99 ) return 0;
    const double s = strictness * strictness;
    return static_cast<uint>( ceil( 0.460517 * m_dateWeight / ( 0.1 + s ) ) );
}

int
ConstraintTypes::TagMatch::Comparer::rangeNum( const double strictness, const qint64 field ) const
{
    if ( strictness > 0.99 ) return 0;
    const double s = strictness * strictness;
    const double w = m_numFieldWeight.value( field );
    return static_cast<int>( ceil( 0.460517 * w / ( 0.1 + s ) ) );
}

double
ConstraintTypes::TagMatch::Comparer::fuzzyProb( const double a, const double b, const double strictness, const double w ) const
{
    const double s = strictness * strictness;
    return exp( -10.0 * ( 0.1 + s ) / w * ( 1 + abs( a - b ) ) );
}
