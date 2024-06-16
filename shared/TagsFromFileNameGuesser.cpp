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

#include <QRegularExpression>
#include <QStringList>

const QStringList m_schemes( QStringList()
    //01 Artist - Title.ext
    << QStringLiteral("^%track%\\W*-?\\W*%artist%\\W*-\\W*%title%\\.+(?:\\w{2,5})$")
    //01 Title.ext
    << QStringLiteral("^%track%\\W*-?\\W*%title%\\.+?:\\w{2,5}$")
    //Album - 01 - Artist - Title.ext
    << QStringLiteral("^%album%\\W*-\\W*%track%\\W*-\\W*%artist%\\W*-\\W*%title%\\.+(?:\\w{2,5})$")
    //Artist - Album - 01 - Title.ext
    << QStringLiteral("^%artist%\\W*-\\W*%album%\\W*-\\W*%track%\\W*-\\W*%title%\\.+(?:\\w{2,5})$")
    // Artist - Album - Title.ext
    << QStringLiteral("^%artist%\\W*-\\W*%album%\\W*-\\W*%title%\\.+(?:\\w{2,5})$")
    //Artist - Title.ext
    << QStringLiteral("^%artist%\\W*-\\W*%title%\\.+(?:\\w{2,5})$")
    //Title.ext
    << QStringLiteral("^%title%\\.+(?:\\w{2,5})$")
);

const QRegularExpression m_digitalFields( QStringLiteral("(%(?:discnumber|track|year)%)") );
const QRegularExpression m_literalFields( QStringLiteral("(%(?:album|albumartist|artist|comment|composer|genre|title)%)") );

quint64
fieldName( const QString &field )
{
    if( field == QLatin1String("album") )
        return Meta::valAlbum;
    else if( field == QLatin1String("albumartist") )
        return Meta::valAlbumArtist;
    else if( field == QLatin1String("artist") )
        return Meta::valArtist;
    else if( field == QLatin1String("comment") )
        return Meta::valComment;
    else if( field == QLatin1String("composer") )
        return Meta::valComposer;
    else if( field == QLatin1String("discnumber") )
        return Meta::valDiscNr;
    else if( field == QLatin1String("genre") )
        return Meta::valGenre;
    else if( field == QLatin1String("title") )
        return Meta::valTitle;
    else if( field == QLatin1String("track") )
        return Meta::valTrackNr;
    else if( field == QLatin1String("year") )
        return Meta::valYear;

    return 0;
}

QList< qint64 >
parseTokens( const QString &scheme )
{
    QRegularExpression rxm( QStringLiteral("%(\\w+)%") );
    QList< qint64 > tokens;

    int pos = 0;
    qint64 field;
    while( ( pos = scheme.indexOf( rxm, pos ) ) != -1 )
    {
        QRegularExpressionMatch rmatch = rxm.match( scheme, pos );
        field = fieldName( rmatch.captured( 1 ) );
        if( field )
            tokens << field;
        pos += rmatch.capturedLength();
    }

    return tokens;
}

Meta::FieldHash
Meta::Tag::TagGuesser::guessTagsByScheme( const QString &fileName, const QString &scheme,
                                          bool cutTrailingSpaces, bool convertUnderscores,
                                          bool isRegExp )
{
    Meta::FieldHash metadata;

    QRegularExpression rx;

    QString m_fileName = fileName;
    QString m_scheme = scheme;

    QList< qint64 > tokens = parseTokens( m_scheme );
    
    // Screen all special symbols
    if( !isRegExp )
        m_scheme = m_scheme.replace( QRegularExpression( QStringLiteral("([~!\\^&*()\\-+\\[\\]{}\\\\:\"?\\.])" )), QStringLiteral("\\\\1") );
    
    QRegularExpression spaces( QStringLiteral("(\\s+)") );
    rx.setPattern( m_scheme.replace( spaces, QStringLiteral("\\s+") )
                           .replace( m_digitalFields, QStringLiteral("(\\d+)") )
                           .replace( m_literalFields, QStringLiteral("(.+)") )
                           .replace( QLatin1String("%ignore%"), QLatin1String("(?:.+)") ) );

    QRegularExpressionMatch rmatch = QRegularExpression(QRegularExpression::anchoredPattern(rx.pattern())).match( m_fileName );
    if( !rmatch.hasMatch() )
        return metadata;

    QString value;
    for( int i = 0; i < tokens.count(); i++ )
    {
        value = rmatch.captured( i + 1 );
        if( convertUnderscores )
            value.replace( QLatin1Char('_'), QLatin1Char(' ') );
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
    if( ( pos = fileName.lastIndexOf( QLatin1Char('/') ) ) != -1 )
            tmpStr = fileName.mid( pos + 1 );

    for( const auto &scheme : m_schemes )
    {
        Meta::FieldHash metadata = guessTagsByScheme( tmpStr, scheme, true, true, true );
        if( !metadata.isEmpty() )
            return metadata;
    }

    return Meta::FieldHash();
}
