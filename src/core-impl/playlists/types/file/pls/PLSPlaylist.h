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

class QTextStream;
class QFile;

namespace Playlists {

class PLSPlaylist;

typedef KSharedPtr<PLSPlaylist> PLSPlaylistPtr;
typedef QList<PLSPlaylistPtr> PLSPlaylistList;

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT_TESTS PLSPlaylist : public PlaylistFile
{
    public:
        PLSPlaylist();
        PLSPlaylist( Meta::TrackList tracks );
        PLSPlaylist( const KUrl &url );

        ~PLSPlaylist();

        /* Playlist virtual functions */
        virtual QString name() const { return prettyName(); }
        virtual QString prettyName() const { return m_url.fileName(); }
        virtual QString description() const;

        /** returns all tracks in this playlist */
        Meta::TrackList tracks() { return m_tracks; }

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const { Q_UNUSED( type ); return false; }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) { Q_UNUSED( type ); return 0; }

        KUrl retrievableUrl() { return m_url; }

        /* PlaylistFile methods */
        bool isWritable();
        void setName( const QString &name );
        bool save( const KUrl &location, bool relative );
        bool load( QTextStream &stream ) { return loadPls( stream ); }

    private:
        bool loadPls( QTextStream &stream );
        unsigned int loadPls_extractIndex( const QString &str ) const;

        Meta::TrackList m_tracks;
        KUrl m_url;
};

}

Q_DECLARE_METATYPE( Playlists::PLSPlaylistPtr )
Q_DECLARE_METATYPE( Playlists::PLSPlaylistList )

#endif
