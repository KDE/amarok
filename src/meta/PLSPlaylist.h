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

#ifndef METAPLSPLAYLIST_H
#define METAPLSPLAYLIST_H

#include <Playlist.h>

class QTextStream;
class QFile;

namespace Meta {

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class PLSPlaylist : public Playlist
{
    public:
        PLSPlaylist();
        PLSPlaylist( TrackList tracks );
        PLSPlaylist( QTextStream &stream );

        ~PLSPlaylist();

        bool save( QFile &file, bool relative );

        /* Meta::Playlist virtual functions */
        QString name() const;
        QString prettyName() const { return name(); };

        /** returns all tracks in this playlist */
        TrackList tracks() { return m_tracks; };

        bool hasCapabilityInterface( Meta::Capability::Type type ) const { return false; };

        Capability* asCapabilityInterface( Capability::Type type ) { return 0; };

        KUrl retrievableUrl() { return KUrl(); };

        bool load( QTextStream &stream ) { return loadPls( stream ); };

    private:
        bool loadPls( QTextStream &stream );
        unsigned int loadPls_extractIndex( const QString &str ) const;

        Meta::TrackList m_tracks;

};

}

#endif
