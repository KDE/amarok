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

#include "StringHelper.h"
#include <QTextCodec>

static QTextCodec *s_codec = QTextCodec::codecForName( "UTF-8" );

void
Meta::Tag::setCodec( QTextCodec *codec )
{
    s_codec = codec;
}

void
Meta::Tag::setCodecByName( const QByteArray &codecName )
{
    s_codec = QTextCodec::codecForName( codecName );
}

TagLib::String
Meta::Tag::Qt4QStringToTString( const QString &str )
{
    // Declare new var to prevent double call of trimmed func
    QString val = str.trimmed();
    return val.isEmpty() ? TagLib::String() : TagLib::String( val.toUtf8().data(), TagLib::String::UTF8 );
}

QString
Meta::Tag::TStringToQString( const TagLib::String &str )
{
    return s_codec->toUnicode( str.toCString( true ) ).trimmed();
}
