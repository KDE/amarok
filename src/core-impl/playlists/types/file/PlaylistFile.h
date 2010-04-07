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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef METAPLAYLISTFILE_H
#define METAPLAYLISTFILE_H

#include "core/playlists/Playlist.h"
#include "core/meta/Meta.h"

class PlaylistProvider;

namespace Playlists
{

    class PlaylistFile;

    typedef KSharedPtr<PlaylistFile> PlaylistFilePtr;
    typedef QList<PlaylistFilePtr> PlaylistFileList;

    /**
     * Base class for all playlist files
     *
     **/
    class AMAROK_EXPORT PlaylistFile : public Playlist
    {
        public:
            PlaylistFile() : Playlist(), m_provider( 0 ) {}
            virtual ~PlaylistFile() {}

            virtual bool isWritable() { return false; }

            virtual bool save( const KUrl &url, bool relative )
                { Q_UNUSED( url ); Q_UNUSED( relative ); return false; }
            virtual bool load( QTextStream &stream ) { Q_UNUSED( stream ); return false; }

            virtual Meta::TrackList queue() { return Meta::TrackList(); }
            virtual void setQueue( const Meta::TrackList &tracks ) { Q_UNUSED( tracks ); }

            virtual void setName( const QString &name ) = 0;
            virtual void setGroups( const QStringList &groups ) { m_groups = groups; }
            virtual QStringList groups() { return m_groups; }

            //default implementation prevents crashes related to PlaylistFileProvider
            virtual void setProvider( PlaylistProvider *provider ) { m_provider = provider; }

            /* Playlist Methods */
            virtual PlaylistProvider *provider() const { return m_provider; }

        protected:
            PlaylistProvider *m_provider;
            QStringList m_groups;
    };

}

Q_DECLARE_METATYPE( Playlists::PlaylistFilePtr )
Q_DECLARE_METATYPE( Playlists::PlaylistFileList )

#endif
