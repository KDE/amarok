/****************************************************************************************
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef METAPLAYLISTFILE_H
#define METAPLAYLISTFILE_H

#include "Playlist.h"

namespace Meta
{

    class PlaylistFile;

    typedef KSharedPtr<PlaylistFile> PlaylistFilePtr;
    typedef QList<PlaylistFilePtr> PlaylistFileList;

    /**
     * Base class for all playlist files
     *
     **/
    class PlaylistFile : public Playlist
    {
        public:
            PlaylistFile() {};
            PlaylistFile( const KUrl &url ) { Q_UNUSED( url ); }
            virtual ~PlaylistFile() {};

            virtual bool isWritable() { return false; }

            virtual bool save( const KUrl &url, bool relative )
                { Q_UNUSED( url ); Q_UNUSED( relative ); return false; }
            virtual bool load( QTextStream &stream ) { Q_UNUSED( stream ); return false; }

            virtual void setName( const QString &name ) = 0;
            virtual void setGroups( const QStringList &groups ) { Q_UNUSED( groups ); }
    };

}

Q_DECLARE_METATYPE( Meta::PlaylistFilePtr )
Q_DECLARE_METATYPE( Meta::PlaylistFileList )

#endif
