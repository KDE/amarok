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

#ifndef MEDIADEVICEPODCASTPROVIDER_H
#define MEDIADEVICEPODCASTPROVIDER_H

#include "core/podcasts/PodcastProvider.h"
#include "core-impl/collections/mediadevicecollection/handler/MediaDeviceHandler.h"

namespace Podcasts {

class MediaDevicePodcastProvider : public Podcasts::PodcastProvider
{
    public:
        MediaDevicePodcastProvider( Meta::MediaDeviceHandler *handler );

        //TODO:implement these
        virtual bool possiblyContainsTrack( const QUrl &url ) const { Q_UNUSED(url); return false;}
        virtual Meta::TrackPtr trackForUrl( const QUrl &url ) { Q_UNUSED(url); return Meta::TrackPtr();  }

        virtual void addPodcast( const QUrl &url );

        virtual Podcasts::PodcastChannelPtr addChannel( Podcasts::PodcastChannelPtr channel );
        virtual Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode );

        virtual Podcasts::PodcastChannelList channels();

        virtual void removeSubscription( Podcasts::PodcastChannelPtr channel );

        virtual void configureProvider();
        virtual void configureChannel( Podcasts::PodcastChannelPtr channel );

        // PlaylistProvider methods
        virtual QString prettyName() const;
        virtual int category() const { return (int)Playlists::PodcastChannelPlaylist; }

        virtual Playlists::PlaylistList playlists();

        virtual Playlists::PlaylistPtr addPlaylist( Playlists::PlaylistPtr playlist );
        virtual Meta::TrackPtr addTrack( Meta::TrackPtr track );

    private:
        Meta::MediaDeviceHandler *m_handler;
};

} //namespace Podcasts

#endif // MEDIADEVICEPODCASTPROVIDER_H
