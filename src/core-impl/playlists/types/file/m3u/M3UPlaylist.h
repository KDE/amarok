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

class QTextStream;
class QString;
class QFile;

namespace Playlists {

class M3UPlaylist;

typedef KSharedPtr<M3UPlaylist> M3UPlaylistPtr;
typedef QList<M3UPlaylistPtr> M3UPlaylistList;

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT_TESTS M3UPlaylist : public PlaylistFile
{
    public:
        M3UPlaylist();
        M3UPlaylist( Meta::TrackList tracks );
        M3UPlaylist( const KUrl &url );

        ~M3UPlaylist();

        /* Playlist virtual functions */
        virtual KUrl uidUrl() const { return m_url; }
        virtual QString name() const { return prettyName(); }
        virtual QString prettyName() const { return m_url.fileName(); }
        virtual QString description() const;

        virtual int trackCount() const { return -1; }
        /** returns all tracks in this playlist */
        virtual Meta::TrackList tracks();

       /* the following has been copied from Meta.h
        * it is my hope that we can integrate Playlists
        * better into the rest of the Meta framework someday ~Bart Cerneels
        * TODO: Playlist : public MetaBase
        */
        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const { Q_UNUSED( type ); return false; }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) { Q_UNUSED( type ); return 0; }

        virtual QStringList groups() { return m_groups; }

        /* PlaylistFile methods */
        bool isWritable();
        void setName( const QString &name );

        bool save( const KUrl &location, bool relative );
        bool load( QTextStream &stream ) { return loadM3u( stream ); }

    private:
        bool loadM3u( QTextStream &stream );

        KUrl m_url;

        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        QString m_name;
        QStringList m_groups;
};

}

Q_DECLARE_METATYPE( Playlists::M3UPlaylistPtr )
Q_DECLARE_METATYPE( Playlists::M3UPlaylistList )

#endif
