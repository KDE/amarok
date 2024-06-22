/***************************************************************************
 *   Copyright (C) 2010 Ralf Engels <ralf-engels@gmx.de>                   *
 *   Copyright (C) 2012 Edward Toroshchin <amarok@hades.name>              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef COLLECTIONSCANNER_UTILS_H
#define COLLECTIONSCANNER_UTILS_H

#include <QString>

namespace CollectionScanner
{

/** Removes all characters not allowed by xml 1.0 specification.
    We need this because the Qt 4.6 xml scanner is behaving very badly
    when encountering such characters in the input.
    ...and because Qt 4.whatever xml writer outputs such characters without
    any remorse.
    see http://en.wikipedia.org/wiki/Valid_Characters_in_XML
    */
inline QString
escapeXml10( QString str )
{
    for( int i = 0; i < str.length(); ++i )
    {
        ushort c = str.at(i).unicode();
        if( (c < 0x20 && c != 0x09 && c != 0x0A && c != 0x0D) ||
            (c > 0xD7FF && c < 0xE000) ||
            (c > 0xFFFD) )
            str[i] = QLatin1Char( '?' );
    }
    return str;
}

}

#endif // COLLECTIONSCANNER_UTILS_H
