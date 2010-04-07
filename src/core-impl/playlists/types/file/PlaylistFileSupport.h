/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
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

#ifndef AMAROK_META_PLAYLISTFILESUPPORT_H
#define AMAROK_META_PLAYLISTFILESUPPORT_H

#include "shared/amarok_export.h"
#include "core/meta/Meta.h"
#include "core-impl/playlists/types/file/PlaylistFile.h"

#include <QString>
#include <QTextStream>

#include <KUrl>
#include <kio/job.h>
#include <kio/jobclasses.h>

class QFile;

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
    PlaylistFormat getFormat( const KUrl &path );
    AMAROK_EXPORT bool isPlaylist( const KUrl &path );

    AMAROK_EXPORT PlaylistFilePtr loadPlaylistFile( const KUrl &url );
    bool exportPlaylistFile( const Meta::TrackList &list, const KUrl &path );
    bool exportPlaylistFile( const Meta::TrackList &list, const KUrl &path, const QList<int> &queued );

    /* HACK:
     * the next two functions are needed to support some services that have no other way
     * of presenting data to the user than wrapping the url to a playlist in a track.
     */
    bool canExpand( Meta::TrackPtr track );
    PlaylistPtr expand( Meta::TrackPtr track );

    AMAROK_EXPORT KUrl newPlaylistFilePath( const QString& fileExtension );

}

#endif
