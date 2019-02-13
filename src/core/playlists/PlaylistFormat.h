/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 *           (c) 2010 Jeff Mitchell <mitchell@kde.org>                                  *
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

#ifndef AMAROK_PLAYLISTFORMAT_H
#define AMAROK_PLAYLISTFORMAT_H

#include "core/amarokcore_export.h"

#include <QUrl>


namespace Playlists
{
    enum PlaylistFormat
    {
        M3U,
        PLS,
        XML,
        RAM,
        SMIL,
        ASX,
        XSPF,
        Unknown,
        NotPlaylist = Unknown
    };

    AMAROKCORE_EXPORT PlaylistFormat getFormat( const QUrl &path );
    AMAROKCORE_EXPORT bool isPlaylist( const QUrl &path );
}

#endif
