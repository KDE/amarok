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

#ifndef STRINGHELPER_H
#define STRINGHELPER_H

#include <QString>
#include <tstring.h>
class QTextCodec;

#ifdef Qt4QStringToTString
    #undef Qt4QStringToTString
#endif
#ifdef TStringToQString
    #undef TStringToQString
#endif

namespace Meta
{
    namespace Tag
    {
        /**
         * Convert TString to QString, trimmes spaces in the begin and at the end
         * and fixes encoding if needed.
         */
        QString TStringToQString( const TagLib::String &str );

        /**
         * Convert QString to TString and trimmes spaces in the begin and at the end.
         */
        TagLib::String Qt4QStringToTString( const QString &str );

        /**
         * Set codec for TStringToQString conversion.
         */
        void setCodec( QTextCodec *codec );
        void setCodecByName( const QByteArray &codecName );
    }
}

#endif // STRINGHELPER_H
