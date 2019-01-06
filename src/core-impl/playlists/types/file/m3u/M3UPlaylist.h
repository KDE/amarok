/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef METAM3UPLAYLIST_H
#define METAM3UPLAYLIST_H

#include "core-impl/playlists/types/file/PlaylistFile.h"

namespace Playlists {
/**
 * @author Bart Cerneels <bart.cerneels@kde.org>
 */
class AMAROK_EXPORT M3UPlaylist : public PlaylistFile
{
    public:
        explicit M3UPlaylist( const QUrl &url, PlaylistProvider *provider = 0 );

        /* PlaylistFile methods */
        using PlaylistFile::load;
        bool load( QTextStream &stream ) override { return loadM3u( stream ); }

        QString extension() const override { return QStringLiteral("m3u"); }
        QString mimetype() const override { return QStringLiteral("audio/x-mpegurl"); }

    protected:
        void savePlaylist( QFile &file ) override;

    private:
        bool loadM3u( QTextStream &stream );
};
}

#endif
