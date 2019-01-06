/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 *           (c) 2010 Jeff Mitchell <mitchell@kde.org>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "core/playlists/PlaylistFormat.h"

#include "core/support/Amarok.h"

#include <QUrl>

#include <QString>

namespace Playlists {

PlaylistFormat
getFormat( const QUrl &path )
{
    const QString ext = Amarok::extension( path.fileName() );

    if( ext == QLatin1String("m3u") || ext == QLatin1String("m3u8") ) return M3U; //m3u8 is M3U in UTF8
    if( ext == QLatin1String("pls") ) return PLS;
    if( ext == QLatin1String("ram") ) return RAM;
    if( ext == QLatin1String("smil")) return SMIL;
    if( ext == QLatin1String("asx") || ext == QLatin1String("wax") || ext == QLatin1String("asf") ) return ASX;
    if( ext == QLatin1String("xml") ) return XML;
    if( ext == QLatin1String("xspf") ) return XSPF;

    return Unknown;
}

bool
isPlaylist( const QUrl &path )
{
    return ( getFormat( path ) != Unknown );
}

}
