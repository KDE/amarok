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

#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"

#include <QList>
#include <QString>
#include <QStringList>

QStringList Constraint::s_fieldNames;
QHash<QString, Constraint::FieldType> Constraint::s_fieldTypes;
QHash<QString, qint64> Constraint::s_fieldValues;

QStringList
Constraint::fieldNames()
{
    if ( s_fieldNames.isEmpty() )
        fillFieldNames();

    return s_fieldNames;
}

Constraint::FieldType
Constraint::typeOfField( const QString& s )
{
    if ( s_fieldTypes.isEmpty() )
        fillFieldTypes();

    return s_fieldTypes.value( s );
}

qint64
Constraint::metaValueOfField( const QString& s )
{
    if ( s_fieldValues.isEmpty() )
        fillFieldValues();

    return s_fieldValues.value( s );
}

QString
Constraint::nameOfMetaValue( const qint64 v )
{
    if ( s_fieldValues.isEmpty() )
        fillFieldValues();

    return s_fieldValues.key( v );
}

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

void
Constraint::fillFieldNames()
{
    s_fieldNames << i18n("url") << i18n("title") << i18n("artist name") << i18n("album name")
                 << i18n("genre") << i18n("composer") << i18n("year") << i18n("comment")
                 << i18n("track number") << i18n("disc number") << i18n("length")
                 << i18n("score") << i18n("rating") << i18n("added to collection")
                 << i18n("first played") << i18n("last played")
                 << i18n("play count") << i18n("label");
}

void
Constraint::fillFieldTypes()
{
    s_fieldTypes.insert( i18n("url"), FieldTypeString );
    s_fieldTypes.insert( i18n("title"), FieldTypeString );
    s_fieldTypes.insert( i18n("artist name"), FieldTypeString );
    s_fieldTypes.insert( i18n("album name"), FieldTypeString );
    s_fieldTypes.insert( i18n("genre"), FieldTypeString );
    s_fieldTypes.insert( i18n("composer"), FieldTypeString );
    s_fieldTypes.insert( i18n("year"), FieldTypeInt );
    s_fieldTypes.insert( i18n("comment"), FieldTypeString );
    s_fieldTypes.insert( i18n("track number"), FieldTypeInt );
    s_fieldTypes.insert( i18n("disc number"), FieldTypeInt );

    //s_fieldTypes.insert( i18n("bpm"), FieldTypeInt);
    s_fieldTypes.insert( i18n("length"), FieldTypeInt );
    //s_fieldTypes.insert( i18n("bitrate"), FieldTypeInt );
    //s_fieldTypes.insert( i18n("samplerate"), FieldTypeInt);
    //s_fieldTypes.insert( i18n("filesize"), FieldTypeInt );
    //s_fieldTypes.insert( i18n("filetype"), FieldTypeInt);
    s_fieldTypes.insert( i18n("added to collection"), FieldTypeDate);

    s_fieldTypes.insert( i18n("score"), FieldTypeInt );
    s_fieldTypes.insert( i18n("rating"), FieldTypeInt );
    s_fieldTypes.insert( i18n("first played"), FieldTypeDate );
    s_fieldTypes.insert( i18n("last played"), FieldTypeDate );
    s_fieldTypes.insert( i18n("play count"), FieldTypeInt );
    //s_fieldTypes.insert( i18n("uniqueid"), FieldTypeString);

    s_fieldTypes.insert( i18n("label"), FieldTypeString );
}

void
Constraint::fillFieldValues()
{
    s_fieldValues.insert( i18n("url"), Meta::valUrl );
    s_fieldValues.insert( i18n("title"), Meta::valTitle );
    s_fieldValues.insert( i18n("artist name"), Meta::valArtist );
    s_fieldValues.insert( i18n("album name"), Meta::valAlbum );
    s_fieldValues.insert( i18n("genre"), Meta::valGenre );
    s_fieldValues.insert( i18n("composer"), Meta::valComposer );
    s_fieldValues.insert( i18n("year"), Meta::valYear );
    s_fieldValues.insert( i18n("comment"), Meta::valComment );
    s_fieldValues.insert( i18n("track number"), Meta::valTrackNr );
    s_fieldValues.insert( i18n("disc number"), Meta::valDiscNr );

    //s_fieldValues.insert( i18n("bpm"), Meta::valBPM);
    s_fieldValues.insert( i18n("length"), Meta::valLength );
    //s_fieldValues.insert( i18n("bitrate"), Meta::valBitrate );
    //s_fieldValues.insert( i18n("samplerate"), Meta::valSamplerate);
    //s_fieldValues.insert( i18n("filesize"), Meta::valFilesize );
    //s_fieldValues.insert( i18n("filetype"), Meta::valFormat);
    s_fieldValues.insert( i18n("added to collection"), Meta::valCreateDate);

    s_fieldValues.insert( i18n("score"), Meta::valScore );
    s_fieldValues.insert( i18n("rating"), Meta::valRating );
    s_fieldValues.insert( i18n("first played"), Meta::valFirstPlayed );
    s_fieldValues.insert( i18n("last played"), Meta::valLastPlayed );
    s_fieldValues.insert( i18n("play count"), Meta::valPlaycount );
    //s_fieldValues.insert( i18n("uniqueid"), Meta::valUniqueId);

    /* a bit of a kludge that hopefully doesn't cause problems down the road
     * later -- sth */
    s_fieldValues.insert( i18n("label"), 1LL << 63 );
}
