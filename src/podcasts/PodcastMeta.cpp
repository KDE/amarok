/****************************************************************************************
 * Copyright (c) 2007-2009 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#include "PodcastMeta.h"

Meta::PodcastEpisode::PodcastEpisode()
    : PodcastMetaCommon()
    , Track()
    , m_channel( 0 )
    , m_guid()
    , m_mimeType()
    , m_pubDate()
    , m_duration( 0 )
    , m_fileSize( 0 )
    , m_sequenceNumber( 0 )
    , m_isNew( true )
{
    m_albumPtr = Meta::AlbumPtr( new Meta::PodcastAlbum( this ) );
    m_artistPtr = Meta::ArtistPtr( new Meta::PodcastArtist( this ) );
    m_composerPtr = Meta::ComposerPtr( new Meta::PodcastComposer( this ) );
    m_genrePtr = Meta::GenrePtr( new Meta::PodcastGenre( this ) );
    m_yearPtr = Meta::YearPtr( new Meta::PodcastYear( this ) );
}

Meta::PodcastEpisode::PodcastEpisode( PodcastChannelPtr channel )
    : PodcastMetaCommon()
    , Track()
    , m_channel( channel )
    , m_guid()
    , m_mimeType()
    , m_pubDate()
    , m_duration( 0 )
    , m_fileSize( 0 )
    , m_sequenceNumber( 0 )
    , m_isNew( true )
{
    m_albumPtr = Meta::AlbumPtr( new Meta::PodcastAlbum( this ) );
    m_artistPtr = Meta::ArtistPtr( new Meta::PodcastArtist( this ) );
    m_composerPtr = Meta::ComposerPtr( new Meta::PodcastComposer( this ) );
    m_genrePtr = Meta::GenrePtr( new Meta::PodcastGenre( this ) );
    m_yearPtr = Meta::YearPtr( new Meta::PodcastYear( this ) );
}

