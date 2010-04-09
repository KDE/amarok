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

#include "core/podcasts/PodcastMeta.h"

using namespace Podcasts;

PodcastEpisode::PodcastEpisode()
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
    m_albumPtr = Meta::AlbumPtr( new PodcastAlbum( this ) );
    m_artistPtr = Meta::ArtistPtr( new PodcastArtist( this ) );
    m_composerPtr = Meta::ComposerPtr( new PodcastComposer( this ) );
    m_genrePtr = Meta::GenrePtr( new PodcastGenre( this ) );
    m_yearPtr = Meta::YearPtr( new PodcastYear( this ) );
}

PodcastEpisode::PodcastEpisode( PodcastChannelPtr channel )
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
    m_albumPtr = Meta::AlbumPtr( new PodcastAlbum( this ) );
    m_artistPtr = Meta::ArtistPtr( new PodcastArtist( this ) );
    m_composerPtr = Meta::ComposerPtr( new PodcastComposer( this ) );
    m_genrePtr = Meta::GenrePtr( new PodcastGenre( this ) );
    m_yearPtr = Meta::YearPtr( new PodcastYear( this ) );
}

PodcastEpisode::PodcastEpisode( PodcastEpisodePtr episode,
                                      PodcastChannelPtr channel )
    : m_channel( channel )
{
    m_author = episode->author();
    m_description = episode->description();
    m_duration = episode->duration();
    m_fileSize = episode->filesize();
    m_guid = episode->guid();
    m_isNew = episode->isNew();
    m_keywords = episode->keywords();
    m_localUrl = episode->localUrl();
    m_mimeType = episode->mimeType();
    m_title = episode->title();
    m_pubDate = episode->pubDate();
    m_sequenceNumber = episode->sequenceNumber();
    m_subtitle = episode->subtitle();
    m_summary = episode->summary();
    m_url = episode->uidUrl();
}

PodcastChannel::PodcastChannel( PodcastChannelPtr channel )
{
    m_author = channel->author();
    m_autoScan = channel->autoScan();
    m_copyright = channel->copyright();
    m_description = channel->description();
    m_directory = channel->saveLocation();
    m_episodes = channel->episodes();
    m_fetchType = channel->fetchType();
    m_imageUrl = channel->m_imageUrl;
    m_keywords = channel->keywords();
    m_labels = channel->labels();
    m_name = channel->name();
    m_purge = channel->hasPurge();
    m_purgeCount = channel->purgeCount();
    m_subscribeDate = channel->subscribeDate();
    m_subtitle = channel->subtitle();
    m_summary = channel->summary();
    m_title = channel->title();
    m_url = channel->url();
    m_webLink = channel->webLink();

    foreach( PodcastEpisodePtr episode, channel->episodes() )
    {
        m_episodes << PodcastEpisodePtr(
                new PodcastEpisode( episode, PodcastChannelPtr( this ) ) );
    }
}

Meta::TrackList
PodcastChannel::tracks()
{
    Meta::TrackList tracks;
    foreach( Podcasts::PodcastEpisodePtr episode, episodes() )
        tracks << Meta::TrackPtr::dynamicCast( episode );
    return tracks;
}

void
PodcastChannel::addTrack( Meta::TrackPtr track, int position )
{
    Q_UNUSED( position );
    addEpisode( PodcastEpisodePtr::dynamicCast( track ) );
}

Podcasts::PodcastEpisodePtr
PodcastChannel::addEpisode( PodcastEpisodePtr episode )
{
    if( !episode.isNull() )
        m_episodes << episode;
    return episode;
}
