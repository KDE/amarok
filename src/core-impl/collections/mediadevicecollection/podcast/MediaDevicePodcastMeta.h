#ifndef MEDIADEVICEPODCASTMETA_H
#define MEDIADEVICEPODCASTMETA_H

#endif // MEDIADEVICEPODCASTMETA_H

#include "core/podcasts/PodcastMeta.h"

namespace Handler
{
    class MediaDevicePodcastChannel;
    typedef KSharedPtr<MediaDevicePodcastChannel> MediaDevicePodcastChannelPtr;
    typedef QList<MediaDevicePodcastChannelPtr> MediaDevicePodcastChannelList;

    class MediaDevicePodcastEpisode;
    typedef KSharedPtr<MediaDevicePodcastEpisode> MediaDevicePodcastEpisodePtr;
    typedef QList<MediaDevicePodcastEpisodePtr> MediaDevicePodcastEpisodeList;

    class MediaDevicePodcastChannel : public Podcasts::PodcastChannel
    {

    };

    class MediaDevicePodcastEpisode : public Podcasts::PodcastEpisode
    {

    };
}

Q_DECLARE_METATYPE( Handler::MediaDevicePodcastChannelPtr )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastChannelList )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastEpisodePtr )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastEpisodeList )
