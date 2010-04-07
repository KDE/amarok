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

#include "TagMatch.h"

#include "core/meta/support/MetaConstants.h"

ConstraintTypes::TagMatchFieldsModel::TagMatchFieldsModel()
{
    // if you add something here, you need to update TagMatch::matches() to handle it
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
    m_fieldMetaValues.insert( "url", Meta::valUrl );

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
    return m_fieldNames.at( idx );
}

qint64
ConstraintTypes::TagMatchFieldsModel::meta_value_of( const QString& f ) const
{
    return m_fieldMetaValues.value( f );
}

ConstraintTypes::TagMatch::FieldTypes
ConstraintTypes::TagMatchFieldsModel::type_of( const QString& f ) const
{
    return m_fieldTypes.value( f );
}
