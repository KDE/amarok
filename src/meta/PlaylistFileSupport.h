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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_META_PLAYLISTFILESUPPORT_H
#define AMAROK_META_PLAYLISTFILESUPPORT_H

#include "amarok_export.h"
#include "Meta.h"
#include "Playlist.h"

#include <QString>
#include <QTextStream>

#include <KUrl>
#include <kio/job.h>
#include <kio/jobclasses.h>

class QFile;

namespace Meta
{
    enum Format { M3U, PLS, XML, RAM, SMIL, ASX, XSPF, Unknown, NotPlaylist = Unknown };

    Format getFormat( const QString &filename );

    AMAROK_EXPORT PlaylistPtr loadPlaylist( const KUrl &url );

}

#endif
