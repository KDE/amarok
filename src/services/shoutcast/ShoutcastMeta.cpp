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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "ShoutcastMeta.h"
#include "meta/PlaylistFileSupport.h"

#include "Debug.h"
#include "Amarok.h"

ShoutcastTrack::ShoutcastTrack( const QString &name, const KUrl & playlistUrl )
    : MetaStream::Track( KUrl() )
    , m_playlistUrl( playlistUrl )
    , m_title( name )
{
    setTitle( name );
}

ShoutcastTrack::~ShoutcastTrack()
{}

KUrl ShoutcastTrack::playableUrl() const
{
    if( !MetaStream::Track::playableUrl().url().isEmpty() )
        return MetaStream::Track::playableUrl();

    if( !m_playlist )
        m_playlist = Meta::loadPlaylist( m_playlistUrl );

    //did it go well?
    if ( !m_playlist )
        return KUrl();

    if ( m_playlist->tracks().size() > 0 ) {
        //debug() << "returning url: " << m_playlist->tracks()[0]->playableUrl();
        //updateUrl( m_playlist->tracks()[0]->playableUrl() );
        return m_playlist->tracks()[0]->playableUrl();
    }

    return KUrl();
}

Meta::GenrePtr ShoutcastTrack::genre() const
{
    return m_genre;
}

void ShoutcastTrack::setGenre( Meta::GenrePtr genre )
{
    m_genre = genre;
}

QString ShoutcastTrack::name() const
{
    const QString ancestorName = MetaStream::Track::name();
    
    if ( ancestorName.isEmpty() )
        return m_title;

    return ancestorName;
}

QString ShoutcastTrack::uidUrl() const
{
    return m_playlistUrl.url();
}


