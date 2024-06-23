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
 
#ifndef AMAROK_QSTRINGX_H
#define AMAROK_QSTRINGX_H

#include <QStringList>
#include <QMap>

namespace Amarok
{

class QStringx : public QString
{
public:
    QStringx();
    explicit QStringx( QChar ch );
    explicit QStringx( const QString &s );
    QStringx( const QChar *unicode, uint length );
    virtual ~QStringx();

    // the numbers following % obviously are not taken into account
    QString args( const QStringList& args ) const;

    // %something% gets replaced by the value corresponding to key "something" in args
    // if opt is true: return empty string if it has empty tokens
    QString namedArgs( const QMap<QString, QString> &args, bool opt = false ) const;

    // %something% gets replaced by the value corresponding to key "something" in args,
    // however, if key "something" is not available,
    // then replace everything within surrounding { } by an empty string
    QString namedOptArgs( const QMap<QString, QString> &args ) const;

private:
    enum CharType
    {
        CTRegular,
        CTToken,
        CTBraceOpen,
        CTBraceClose,
        CTBracketOpen,
        CTBracketSeparator,
        CTBracketClose,
        CTNone
    };
        
    CharType testChar( int *pos ) const;
    QString parseToken( int *pos, const QMap<QString, QString> &dict ) const;
    QString parseBraces( int *pos, const QMap<QString, QString> &dict ) const;
    QString parseBrackets( int *pos, const QMap<QString, QString> &dict ) const;
    QString parse( int *pos, const QMap<QString, QString> &dict ) const;
};

} // namespace Amarok

#endif // AMAROK_QSTRINGX_H
