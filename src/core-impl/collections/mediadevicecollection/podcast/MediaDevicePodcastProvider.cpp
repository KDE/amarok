/****************************************************************************************
 * Copyright 2010 Bart Cerneels <bart.cerneels@kde.org>                                 *
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


#include "MediaDevicePodcastProvider.h"

using namespace Podcasts;

MediaDevicePodcastProvider::MediaDevicePodcastProvider( Meta::MediaDeviceHandler *handler )
        : m_handler( handler )
{
}

void
MediaDevicePodcastProvider::addPodcast( const QUrl &url )
{
    Q_UNUSED( url )
    //can this handler even fetch feeds itself?
}

PodcastChannelPtr
MediaDevicePodcastProvider::addChannel( const PodcastChannelPtr &channel )
{
    Q_UNUSED( channel )
    return PodcastChannelPtr();
}

PodcastEpisodePtr
MediaDevicePodcastProvider::addEpisode( PodcastEpisodePtr episode )
{
    Q_UNUSED( episode )
    return PodcastEpisodePtr();
}

PodcastChannelList
MediaDevicePodcastProvider::channels()
{
    PodcastChannelList channels;
    return channels;
}

void
MediaDevicePodcastProvider::removeSubscription( PodcastChannelPtr channel )
{
    Q_UNUSED( channel )
}

void
MediaDevicePodcastProvider::configureProvider()
{
}

void
MediaDevicePodcastProvider::configureChannel( PodcastChannelPtr channel )
{
    Q_UNUSED( channel )
}

QString
MediaDevicePodcastProvider::prettyName() const
{
    return i18nc( "Podcasts on a media device", "Podcasts on %1", m_handler->prettyName() );
}

Playlists::PlaylistList
MediaDevicePodcastProvider::playlists()
{
    Playlists::PlaylistList playlists;

    for( PodcastChannelPtr channel : channels() )
        playlists << Playlists::PlaylistPtr::dynamicCast( channel );

    return playlists;
}

Playlists::PlaylistPtr
MediaDevicePodcastProvider::addPlaylist(Playlists::PlaylistPtr playlist )
{
    PodcastChannelPtr channel = PodcastChannelPtr::dynamicCast( playlist );
    if( channel.isNull() )
        return Playlists::PlaylistPtr();

    return Playlists::PlaylistPtr::dynamicCast( addChannel( channel ) );
}

Meta::TrackPtr
MediaDevicePodcastProvider::addTrack(const Meta::TrackPtr &track )
{
    PodcastEpisodePtr episode = PodcastEpisodePtr::dynamicCast( track );
    if( episode.isNull() )
        return Meta::TrackPtr();

    return Meta::TrackPtr::dynamicCast( addEpisode( episode ) );
}
