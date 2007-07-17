/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_META_PLAYLISTFILESUPPORT_H
#define AMAROK_META_PLAYLISTFILESUPPORT_H

#include "meta.h"

#include <QString>
#include <QTextStream>

#include <KUrl>

namespace Meta
{
    /**
     * Save the given TrackList tracks at the specified path as a M3U file.
     */
    bool saveM3u( TrackList tracks, KUrl path, bool relative );

    TrackList loadM3u( QTextStream &stream, const QString &playlistDir );

    /**
     * Save the given TrackList tracks at the specified path as a M3U file.
     */
    void saveXspf( TrackList tracks, KUrl path );

    

}

#endif
