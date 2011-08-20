/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#include "TagsFromFileNameGuesser.h"

#include <QStringList>

const QStringList m_schemes( QStringList()
    //01 Artist - Title.ext
    << "^%track%\\W*-?\\W*%artist%\\W*-\\W*%title%\\.+(?:\\w{2,5})$"
    //01 Title.ext
    << "^%track%\\W*-?\\W*%title%\\.+?:\\w{2,5}$"
    //Album - 01 - Artist - Title.ext
    << "^%album%\\W*-\\W*%track%\\W*-\\W*%artist%\\W*-\\W*%title%\\.+(?:\\w{2,5})$"
    //Artist - Album - 01 - Title.ext
    << "^%artist%\\W*-\\W*%album%\\W*-\\W*%track%\\W*-\\W*%title%\\.+(?:\\w{2,5})$"
    // Artist - Album - Title.ext
    << "^%artist%\\W*-\\W*%album%\\W*-\\W*%title%\\.+(?:\\w{2,5})$"
    //Artist - Title.ext
    << "^%artist%\\W*-\\W*%title%\\.+(?:\\w{2,5})$"
    //Title.ext
    << "^%title%\\.+(?:\\w{2,5})$"
);

const QRegExp m_digitalFields( "(%(?:discnumber|track|year)%)" );
const QRegExp m_literalFields( "(%(?:album|albumartist|artist|comment|composer|genre|title)%)" );

quint64
fieldName( const QString &field )
{
    if( field == "album" )
        return Meta::valAlbum;
    else if( field == "albumartist" )
        return Meta::valAlbumArtist;
    else if( field == "artist" )
        return Meta::valArtist;
    else if( field == "comment" )
        return Meta::valComment;
    else if( field == "composer" )
        return Meta::valComposer;
    else if( field == "discnumber" )
        return Meta::valDiscNr;
    else if( field == "genre" )
        return Meta::valGenre;
    else if( field == "title" )
        return Meta::valTitle;
    else if( field == "track" )
        return Meta::valTrackNr;
    else if( field == "year" )
        return Meta::valYear;

    return 0;
}

QList< qint64 >
parseTokens( const QString &scheme )
{
    QRegExp rxm( "%(\\w+)%" );
    QList< qint64 > tokens;

    int pos = 0;
    qint64 field;
    while( ( pos = rxm.indexIn( scheme, pos ) ) != -1 )
    {
        field = fieldName( rxm.cap( 1 ) );
        if( field )
            tokens << field;
        pos += rxm.matchedLength();
    }

    return tokens;
}

Meta::FieldHash
Meta::Tag::TagGuesser::guessTagsByScheme( const QString &fileName, const QString &scheme,
                                          bool cutTrailingSpaces, bool convertUnderscores )
{
    Meta::FieldHash metadata;

    QRegExp rx;

    QString m_fileName = fileName;
    QString m_scheme = scheme;

    QList< qint64 > tokens = parseTokens( m_scheme );
    
    // Screen all special symbols
    QRegExp escape( "([~!\\^&*()\\-+\\[\\]{}\\\\:\"?\\.])" );
    QRegExp spaces( "(\\s+)" );
    rx.setPattern( m_scheme.replace( escape,"\\\\1" )
                           .replace( spaces, "\\s+" )
                           .replace( m_digitalFields, "(\\d+)" )
                           .replace( m_literalFields, "(.+)" )
                           .replace( "%ignore%", "(?:.+)" ) );

    if( !rx.exactMatch( m_fileName ) )
        return metadata;

    QString value;
    for( int i = 0; i < tokens.count(); i++ )
    {
        value = rx.cap( i + 1 );
        if( convertUnderscores )
            value.replace( "_", " " );
        if( cutTrailingSpaces )
            value = value.trimmed();
        metadata.insert( tokens[i], value );
    }
    return metadata;
}

Meta::FieldHash
Meta::Tag::TagGuesser::guessTags( const QString &fileName )
{
    QString tmpStr = fileName;
    int pos = 0;
    if( ( pos = fileName.lastIndexOf( '/' ) ) != -1 )
            tmpStr = fileName.mid( pos + 1 );

    foreach( QString scheme, m_schemes )
    {
        Meta::FieldHash metadata = guessTagsByScheme( tmpStr, scheme );
        if( !metadata.isEmpty() )
            return metadata;
    }

    return Meta::FieldHash();
}
