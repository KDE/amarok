#ifndef MEDIADEVICEPODCASTMETA_H
#define MEDIADEVICEPODCASTMETA_H

#endif // MEDIADEVICEPODCASTMETA_H

#include "podcasts/PodcastMeta.h"

namespace Handler
{
    class MediaDevicePodcastChannel;
    typedef KSharedPtr<MediaDevicePodcastChannel> MediaDevicePodcastChannelPtr;
    typedef QList<MediaDevicePodcastChannelPtr> MediaDevicePodcastChannelList;

    class MediaDevicePodcastEpisode;
    typedef KSharedPtr<MediaDevicePodcastEpisode> MediaDevicePodcastEpisodePtr;
    typedef QList<MediaDevicePodcastEpisodePtr> MediaDevicePodcastEpisodeList;

    class MediaDevicePodcastChannel : public Meta::PodcastChannel
    {

    };

    class MediaDevicePodcastEpisode : public Meta::PodcastEpisode
    {

    };
}

Q_DECLARE_METATYPE( Handler::MediaDevicePodcastChannelPtr )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastChannelList )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastEpisodePtr )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastEpisodeList )
