/****************************************************************************************
 * Copyright (c) 2010-2012 Soren Harward <stharward@gmail.com>                          *
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

#include <QRegularExpression>

#include <cmath>

ConstraintTypes::TagMatchFieldsModel::TagMatchFieldsModel()
{
    m_fieldNames << QStringLiteral("url")
                 << QStringLiteral("title")
                 << QStringLiteral("artist name")
                 << QStringLiteral("album name")
                 << QStringLiteral("genre")
                 << QStringLiteral("composer")
                 << QStringLiteral("year")
                 << QStringLiteral("comment")
                 << QStringLiteral("track number")
                 << QStringLiteral("disc number")
                 << QStringLiteral("length")
                 << QStringLiteral("score")
                 << QStringLiteral("rating")
                 << QStringLiteral("create date")
                 << QStringLiteral("first played")
                 << QStringLiteral("last played")
                 << QStringLiteral("play count")
                 << QStringLiteral("label");

    m_fieldTypes.insert( QStringLiteral("url"), TagMatch::FieldTypeString );
    m_fieldTypes.insert( QStringLiteral("title"), TagMatch::FieldTypeString );
    m_fieldTypes.insert( QStringLiteral("artist name"), TagMatch::FieldTypeString );
    m_fieldTypes.insert( QStringLiteral("album name"), TagMatch::FieldTypeString );
    m_fieldTypes.insert( QStringLiteral("genre"), TagMatch::FieldTypeString );
    m_fieldTypes.insert( QStringLiteral("composer"), TagMatch::FieldTypeString );
    m_fieldTypes.insert( QStringLiteral("year"), TagMatch::FieldTypeInt );
    m_fieldTypes.insert( QStringLiteral("comment"), TagMatch::FieldTypeString );
    m_fieldTypes.insert( QStringLiteral("track number"), TagMatch::FieldTypeInt );
    m_fieldTypes.insert( QStringLiteral("disc number"), TagMatch::FieldTypeInt );
    m_fieldTypes.insert( QStringLiteral("length"), TagMatch::FieldTypeInt );
    m_fieldTypes.insert( QStringLiteral("create date"), TagMatch::FieldTypeDate);
    m_fieldTypes.insert( QStringLiteral("score"), TagMatch::FieldTypeInt );
    m_fieldTypes.insert( QStringLiteral("rating"), TagMatch::FieldTypeInt );
    m_fieldTypes.insert( QStringLiteral("first played"), TagMatch::FieldTypeDate );
    m_fieldTypes.insert( QStringLiteral("last played"), TagMatch::FieldTypeDate );
    m_fieldTypes.insert( QStringLiteral("play count"), TagMatch::FieldTypeInt );
    m_fieldTypes.insert( QStringLiteral("label"), TagMatch::FieldTypeString );

    m_fieldMetaValues.insert( QStringLiteral("url"), Meta::valUrl );
    m_fieldMetaValues.insert( QStringLiteral("title"), Meta::valTitle );
    m_fieldMetaValues.insert( QStringLiteral("artist name"), Meta::valArtist );
    m_fieldMetaValues.insert( QStringLiteral("album name"), Meta::valAlbum );
    m_fieldMetaValues.insert( QStringLiteral("genre"), Meta::valGenre );
    m_fieldMetaValues.insert( QStringLiteral("composer"), Meta::valComposer );
    m_fieldMetaValues.insert( QStringLiteral("year"), Meta::valYear );
    m_fieldMetaValues.insert( QStringLiteral("comment"), Meta::valComment );
    m_fieldMetaValues.insert( QStringLiteral("track number"), Meta::valTrackNr );
    m_fieldMetaValues.insert( QStringLiteral("disc number"), Meta::valDiscNr );
    m_fieldMetaValues.insert( QStringLiteral("length"), Meta::valLength );
    m_fieldMetaValues.insert( QStringLiteral("create date"), Meta::valCreateDate);
    m_fieldMetaValues.insert( QStringLiteral("score"), Meta::valScore );
    m_fieldMetaValues.insert( QStringLiteral("rating"), Meta::valRating );
    m_fieldMetaValues.insert( QStringLiteral("first played"), Meta::valFirstPlayed );
    m_fieldMetaValues.insert( QStringLiteral("last played"), Meta::valLastPlayed );
    m_fieldMetaValues.insert( QStringLiteral("play count"), Meta::valPlaycount );
    m_fieldMetaValues.insert( QStringLiteral("label"), Meta::valLabel );

    m_fieldPrettyNames.insert( QStringLiteral("url"), i18n("url") );
    m_fieldPrettyNames.insert( QStringLiteral("title"), i18n("title") );
    m_fieldPrettyNames.insert( QStringLiteral("artist name"), i18n("artist name") );
    m_fieldPrettyNames.insert( QStringLiteral("album name"), i18n("album name") );
    m_fieldPrettyNames.insert( QStringLiteral("genre"), i18n("genre") );
    m_fieldPrettyNames.insert( QStringLiteral("composer"), i18n("composer") );
    m_fieldPrettyNames.insert( QStringLiteral("year"), i18nc("Field name", "year") );
    m_fieldPrettyNames.insert( QStringLiteral("comment"), i18n("comment") );
    m_fieldPrettyNames.insert( QStringLiteral("track number"), i18n("track number") );
    m_fieldPrettyNames.insert( QStringLiteral("disc number"), i18n("disc number") );
    m_fieldPrettyNames.insert( QStringLiteral("length"), i18n("length") );
    m_fieldPrettyNames.insert( QStringLiteral("create date"), i18n("added to collection") );
    m_fieldPrettyNames.insert( QStringLiteral("score"), i18n("score") );
    m_fieldPrettyNames.insert( QStringLiteral("rating"), i18n("rating") );
    m_fieldPrettyNames.insert( QStringLiteral("first played"), i18n("first played") );
    m_fieldPrettyNames.insert( QStringLiteral("last played"), i18n("last played") );
    m_fieldPrettyNames.insert( QStringLiteral("play count"), i18n("play count") );
    m_fieldPrettyNames.insert( QStringLiteral("label"), i18n("label") );
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
        QRegularExpression rx( target );
        if ( test.indexOf( rx ) >= 0 )
            return 1.0;
    } else {
        return 0.0;
    }
    return 0.0;
}


double
ConstraintTypes::TagMatch::Comparer::compareDate( const uint test,
                                                  const int comparison,
                                                  const QVariant& targetVar,
                                                  const double strictness ) const
{
    const double weight = m_dateWeight;

    int comp = comparison;
    uint target = 0;
    if ( comparison == CompareDateWithin ) {
        comp = CompareDateAfter;
        QDateTime now = QDateTime::currentDateTime();
        DateRange r = targetVar.value<DateRange>();
        switch ( r.second ) {
            case 0:
                target = now.addDays( -1 * r.first ).toSecsSinceEpoch();
                break;
            case 1:
                target = now.addMonths( -1 * r.first ).toSecsSinceEpoch();
                break;
            case 2:
                target = now.addYears( -1 * r.first ).toSecsSinceEpoch();
                break;
            default:
                break;
        }
    } else {
        target = targetVar.value<uint>();
    }

    const double dte = static_cast<double>( test );
    const double dta = static_cast<double>( target );
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
ConstraintTypes::TagMatch::Comparer::compareLabels( const Meta::TrackPtr &t,
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
