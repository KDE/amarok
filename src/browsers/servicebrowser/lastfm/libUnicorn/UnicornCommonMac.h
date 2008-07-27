/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef UNICORNCOMMONMAC_H
#define UNICORNCOMMONMAC_H

/** @author <erik@last.fm> */

#include <QString>
#include <CoreFoundation/CoreFoundation.h>
#include <QLocale>
#include <QDebug>

namespace UnicornUtils
{
    /**
     * Returns the path to the user's Application Support directory.
     */
    QString
    applicationSupportFolderPath();

    QLocale::Language
    osxLanguageCode();

    inline QString
    CFStringToQString( CFStringRef s )
    {
        QString result;

        if (s != NULL){
            // Can be 32 bit unicode, hence 4 times + 8 bits for the null --mxcl
            CFIndex length = 1 + 4 * CFStringGetLength( s );
            char* buffer = new char[length];

            if (CFStringGetCString( s, buffer, length, kCFStringEncodingUTF8 ))
                result = QString::fromUtf8(buffer);
            else
                qWarning() << "itunesplayer.cpp: CFString conversion failed.";

            delete buffer;
        }

        return result;
    }

}


#endif // UNICORNCOMMONMAC_H
