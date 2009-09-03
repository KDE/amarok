/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef SHOUTCASTMETA_H
#define SHOUTCASTMETA_H

#include "meta/stream/Stream.h"
#include "meta/Playlist.h"

/**
A specialized track inheriting from MetaStream::track that only loads the shoutcast playlist on demand
	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/

class ShoutcastTrack : public MetaStream::Track {

public:
    ShoutcastTrack( const QString &name, const KUrl &playlistUrl );
    ~ShoutcastTrack();

    virtual QString name() const;

    //we need to return something else here or we will be dowloading every single playlist when we insert into the
    //memory collection
    virtual QString uidUrl() const;
    
    KUrl playableUrl() const;
    virtual Meta::GenrePtr genre() const;
    virtual void setGenre( const QString& ) {}; //Keep the compiler happy
    void setGenre( Meta::GenrePtr genre );

private:
    mutable Meta::PlaylistPtr m_playlist;
    KUrl m_playlistUrl;
    Meta::GenrePtr m_genre;
    QString m_title;
};

#endif
