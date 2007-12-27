/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef METAM3UPLAYLIST_H
#define METAM3UPLAYLIST_H

#include <Playlist.h>

class QTextStream;
class QString;
class QFile;

namespace Meta {

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class M3UPlaylist : public Playlist
{
    public:
        M3UPlaylist();
        M3UPlaylist( Meta::TrackList tracks );
        M3UPlaylist( QTextStream &stream );
        M3UPlaylist( const QString &path );

        ~M3UPlaylist();

        bool save( QFile &file, bool relative );

        /* Playlist virtual functions */
        virtual QString name() const { return prettyName(); };
        virtual QString prettyName() const;

        /** returns all tracks in this playlist */
        virtual TrackList tracks() { return m_tracks; };

            /* the following has been copied from Meta.h
        * it is my hope that we can integrate Playlists
        * better into the rest of the Meta framework someday ~Bart Cerneels
        * TODO: Playlist : public MetaBase
            */
        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const { return false; };

        virtual Capability* asCapabilityInterface( Capability::Type type ) { return 0; };

        virtual KUrl retrievableUrl() { return KUrl( m_path ); };
    private:
        bool loadM3u( QTextStream &stream );

        QString m_path;

        TrackList m_tracks;

};

}

#endif
