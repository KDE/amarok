/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "CFStringToQString.h"
#include <QDebug>


QByteArray
CFStringToUtf8( CFStringRef s )
{
    QByteArray result;

    if (s != NULL) 
    {
        CFIndex length;
        length = CFStringGetLength( s );
        length = CFStringGetMaximumSizeForEncoding( length, kCFStringEncodingUTF8 ) + 1;
        char* buffer = new char[length];

        if (CFStringGetCString( s, buffer, length, kCFStringEncodingUTF8 ))
            result = QByteArray( buffer );
        else
            qWarning() << "CFString conversion failed.";

        delete[] buffer;
    }

    return result;
}


#if 0
// this is a Qt implementation I found
QString cfstring2qstring(CFStringRef str)
{
    if(!str)
        return QString();
    
    CFIndex length = CFStringGetLength(str);
    if(const UniChar *chars = CFStringGetCharactersPtr(str))
        return QString((QChar *)chars, length);
    UniChar *buffer = (UniChar*)malloc(length * sizeof(UniChar));
    CFStringGetCharacters(str, CFRangeMake(0, length), buffer);
    QString ret((QChar *)buffer, length);
    free(buffer);
    return ret;
}
#endif
