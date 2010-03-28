#ifndef MEDIADEVICEPODCASTPROVIDER_H
#define MEDIADEVICEPODCASTPROVIDER_H

#include "core/podcasts/PodcastProvider.h"
#include "handler/MediaDeviceHandler.h"

namespace Podcasts {

class MediaDevicePodcastProvider : public Podcasts::PodcastProvider
{
    public:
        MediaDevicePodcastProvider( Meta::MediaDeviceHandler *handler );

        //TODO:implement these
        virtual bool possiblyContainsTrack( const KUrl &url ) const { Q_UNUSED(url); return false;}
        virtual Meta::TrackPtr trackForUrl( const KUrl &url ) { Q_UNUSED(url); return Meta::TrackPtr();  }

        virtual void addPodcast( const KUrl &url );

        virtual Podcasts::PodcastChannelPtr addChannel( Podcasts::PodcastChannelPtr channel );
        virtual Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode );

        virtual Podcasts::PodcastChannelList channels();

        virtual void removeSubscription( Podcasts::PodcastChannelPtr channel );

        virtual void configureProvider();
        virtual void configureChannel( Podcasts::PodcastChannelPtr channel );

        // PlaylistProvider methods
        virtual QString prettyName() const;
        virtual int category() const { return (int)Meta::PodcastChannelPlaylist; }

        virtual Meta::PlaylistList playlists();

    private:
        Meta::MediaDeviceHandler *m_handler;
};

} //namespace Podcasts

#endif // MEDIADEVICEPODCASTPROVIDER_H
