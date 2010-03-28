#include "MediaDevicePodcastProvider.h"

using namespace Podcasts;

MediaDevicePodcastProvider::MediaDevicePodcastProvider( Meta::MediaDeviceHandler *handler )
        : m_handler( handler )
{
}

void
MediaDevicePodcastProvider::addPodcast( const KUrl &url )
{
    Q_UNUSED( url )
    //can this handler even fetch feeds itself?
}

PodcastChannelPtr
MediaDevicePodcastProvider::addChannel( PodcastChannelPtr channel )
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
    return i18nc( "Podcasts on a media device", "Podcasts on %1" )
            .arg(  m_handler->prettyName() );
}

Playlists::PlaylistList
MediaDevicePodcastProvider::playlists()
{
    Playlists::PlaylistList playlists;

    foreach( PodcastChannelPtr channel, channels() )
        playlists << Playlists::PlaylistPtr::dynamicCast( channel );

    return playlists;
}
