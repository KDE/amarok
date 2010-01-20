#ifndef MEDIADEVICEPODCASTPROVIDER_H
#define MEDIADEVICEPODCASTPROVIDER_H

#include "PodcastProvider.h"
#include "handler/MediaDeviceHandler.h"

class MediaDevicePodcastProvider : public PodcastProvider
{
    public:
        MediaDevicePodcastProvider( Meta::MediaDeviceHandler *handler );

        //TODO:implement these
        virtual bool possiblyContainsTrack( const KUrl &url ) const { Q_UNUSED(url); return false;}
        virtual Meta::TrackPtr trackForUrl( const KUrl &url ) { Q_UNUSED(url); return Meta::TrackPtr();  }

        virtual void addPodcast( const KUrl &url );

        virtual Meta::PodcastChannelPtr addChannel( Meta::PodcastChannelPtr channel );
        virtual Meta::PodcastEpisodePtr addEpisode( Meta::PodcastEpisodePtr episode );

        virtual Meta::PodcastChannelList channels();

        virtual void removeSubscription( Meta::PodcastChannelPtr channel );

        virtual void configureProvider();
        virtual void configureChannel( Meta::PodcastChannelPtr channel );

        // PlaylistProvider methods
        virtual QString prettyName() const;
        virtual int category() const { return (int)PlaylistManager::PodcastChannel; }

        virtual Meta::PlaylistList playlists();

    private:
        Meta::MediaDeviceHandler *m_handler;
};

#endif // MEDIADEVICEPODCASTPROVIDER_H
