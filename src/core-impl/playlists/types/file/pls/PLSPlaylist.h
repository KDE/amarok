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

#ifndef METAPLSPLAYLIST_H
#define METAPLSPLAYLIST_H

#include "core-impl/playlists/types/file/PlaylistFile.h"

namespace Playlists {
/**
 * @author Bart Cerneels <bart.cerneels@kde.org>
 */
class AMAROK_EXPORT PLSPlaylist : public PlaylistFile
{
    public:
        explicit PLSPlaylist( const QUrl &url, PlaylistProvider *provider = 0 );

        /* PlaylistFile methods */
        using PlaylistFile::load;
        virtual bool load( QTextStream &stream ) { return loadPls( stream ); }

        virtual QString extension() const { return "pls"; }
        virtual QString mimetype() const { return "audio/x-scpls"; }

    protected:
        virtual void savePlaylist( QFile &file );

    private:
        bool loadPls( QTextStream &stream );
        unsigned int loadPls_extractIndex( const QString &str ) const;
};
}

#endif
