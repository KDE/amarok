/****************************************************************************************
 * Copyright (c) 2004 Shintaro Matsuoka <shin@shoegazed.org>                            *
 * Copyright (c) 2006 Martin Aumueller <aumuell@reserv.at>                              *
 * Copyright (c) 2011 Sergey Ivanov <123kash@gmail.com>                                 *
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

#include "QStringx.h"

#include <QRegularExpression>

Amarok::QStringx::QStringx()
{
}

Amarok::QStringx::QStringx( QChar ch )
                : QString( ch )
{
}

Amarok::QStringx::QStringx( const QString &s )
                : QString( s )
{
}

Amarok::QStringx::QStringx( const QByteArray &ba )
                : QString( ba )
{
}

Amarok::QStringx::QStringx( const QChar *unicode, uint length )
                : QString( unicode, length )
{
}

Amarok::QStringx::QStringx( const char *str )
                : QString( str )
{
}

Amarok::QStringx::~QStringx()
{
}

QString
Amarok::QStringx::args( const QStringList &args ) const
{
    const QStringList text = (*this).split( QRegularExpression( "%\\d+" ), Qt::KeepEmptyParts );

    QList<QString>::ConstIterator itrText = text.constBegin();
    QList<QString>::ConstIterator itrArgs = args.constBegin();
    QList<QString>::ConstIterator endText = text.constEnd();
    QList<QString>::ConstIterator endArgs = args.constEnd();
    QString merged = (*itrText);
    ++itrText;
    while( itrText != endText && itrArgs != endArgs )
    {
        merged += (*itrArgs) + (*itrText);
        ++itrText;
        ++itrArgs;
    }

    Q_ASSERT( itrText == text.end() || itrArgs == args.end() );

    return merged;
}

QString
Amarok::QStringx::namedArgs( const QMap<QString, QString> &args, bool opt ) const
{
    // Screen all kinds of brackets and format string with namedOptArgs.
    QString formatString = *this;
    formatString.replace( QRegularExpression( "([\\[\\]{}])" ),"\\\\1" );

    // Legacy code returned empty string if any token was empty, so do the same
    if( opt )
        formatString = QLatin1Char( '{' ) + formatString + QLatin1Char( '}' );

    QStringx fmtx( formatString );
    return fmtx.namedOptArgs( args );
}

QString
Amarok::QStringx::namedOptArgs( const QMap<QString, QString> &args ) const
{
    int pos = 0;
    return parse( &pos, args );
}

Amarok::QStringx::CharType
Amarok::QStringx::testChar( int *pos ) const
{
    if( *pos >= length() )
        return CTNone;

    QChar c = this->at( *pos );

    if( c == QLatin1Char( '\\' ) )
    {
        ( *pos )++;
        return ( *pos >= length() ) ? CTNone : CTRegular;
    }

    if( c == QLatin1Char( '{' ) )
        return CTBraceOpen;

    if( c == QLatin1Char( '}' ) )
        return CTBraceClose;

    if( c == QLatin1Char( '[' ) )
        return CTBracketOpen;

    if( c == QLatin1Char( QLatin1Char(':') ) )
        return CTBracketSeparator;

    if( c == QLatin1Char( ']' ) )
        return CTBracketClose;

    if( c == QLatin1Char( '%' ) )
        return CTToken;

    return CTRegular;
}

QString
Amarok::QStringx::parseToken( int *pos, const QMap<QString, QString> &dict ) const
{
    if( testChar( pos ) != CTToken )
        return QString();

    ( *pos )++;

    CharType ct = testChar( pos );
    QString key;

    while( ct == CTRegular )
    {
        key += this->at( *pos );
        ( *pos )++;
        ct = testChar( pos );
    }

    if( ct == CTToken )
    {
        ( *pos )++;
        return dict.value( key );
    }

    *pos -= key.length();

    return QLatin1String( "%" );
}

QString
Amarok::QStringx::parseBraces( int *pos, const QMap<QString, QString> &dict ) const
{
    if( testChar( pos ) != CTBraceOpen )
        return QString();

    ( *pos )++;

    int retPos = *pos;
    QString result;
    bool isPritable = true;
    CharType ct = testChar( pos );

    while( ct != CTNone && ct != CTBraceClose )
    {
        switch( ct )
        {
        case CTBraceOpen:
            {
                result += parseBraces( pos, dict );
                break;
            }
        case CTBracketOpen:
            {
                result += parseBrackets( pos, dict );
                break;
            }
        case CTToken:
            {
                QString part = parseToken( pos, dict );
                if( part.isEmpty() )
                    isPritable = false;

                result += part;
                break;
            }

        default:
            {
                result += this->at( *pos );
                ( *pos )++;
            }
        }

        ct = testChar( pos );
    }

    if( ct == CTBraceClose )
    {
        ( *pos )++;
        if( isPritable )
            return result;

        return QString();
    }

    *pos = retPos;
    return QLatin1String( "{" );
}

QString
Amarok::QStringx::parseBrackets( int *pos, const QMap<QString, QString> &dict ) const
{
    if( testChar( pos ) != CTBracketOpen )
        return QString();

    ( *pos )++;

    // Check if next char is %
    if( testChar( pos ) != CTToken )
        return QLatin1String( "[" );

    int retPos = *pos;

    ( *pos )++;

    // Parse token manually (not by calling parseToken), because we need token name.
    CharType ct = testChar( pos );
    QString key;

    while( ct == CTRegular )
    {
        key += this->at( *pos );
        ( *pos )++;
        ct = testChar( pos );
    }

    if( ct != CTToken || key.isEmpty() )
    {
        *pos = retPos;
        return QLatin1String( "[" );
    }

    ( *pos )++;

    QString replacement;

    // Parse replacement string if we have one
    if( testChar( pos ) == CTBracketSeparator )
    {
        ( *pos )++;

        ct = testChar( pos );

        while( ct != CTNone && ct != CTBracketClose )
        {
            switch( ct )
            {
            case CTBraceOpen:
                {
                    replacement += parseBraces( pos, dict );
                    break;
                }
            case CTBracketOpen:
                {
                    replacement += parseBrackets( pos, dict );
                    break;
                }
            case CTToken:
                {
                    replacement += parseToken( pos, dict );
                    break;
                }

            default:
                {
                    replacement += this->at( *pos );
                    ( *pos )++;
                }
            }

            ct = testChar( pos );
        }

        if( ct == CTNone )
        {
            *pos = retPos;
            return QLatin1String( "[" );
        }
    }

    if( testChar( pos ) == CTBracketClose )
    {
        ( *pos )++;

        if( !dict.value( key ).isEmpty() )
            return dict.value( key );

        if( !replacement.isEmpty() )
            return replacement;

        if( !dict.value( QLatin1String( "default_" ) + key ).isEmpty() )
            return dict.value( QLatin1String( "default_" ) + key );

        return QLatin1String( "Unknown " ) + key;
    }

    *pos = retPos;
    return QLatin1String( "[" );
}

QString
Amarok::QStringx::parse( int *pos, const QMap<QString, QString> &dict ) const
{
    CharType ct = testChar( pos );
    QString result;

    while( ct != CTNone )
    {
        switch( ct )
        {
        case CTBraceOpen:
            {
                result += parseBraces( pos, dict );
                break;
            }
        case CTBracketOpen:
            {
                result += parseBrackets( pos, dict );
                break;
            }
        case CTToken:
            {
                result += parseToken( pos, dict );
                break;
            }

        default:
            {
                result += this->at( *pos );
                ( *pos )++;
            }
        }

        ct = testChar( pos );
    }

    return result;
}
