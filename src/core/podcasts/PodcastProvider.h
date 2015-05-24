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

#ifndef PODCASTPROVIDER_H
#define PODCASTPROVIDER_H

#include "core/collections/Collection.h"
#include "core/playlists/PlaylistProvider.h"
#include "core/podcasts/PodcastMeta.h"

namespace Podcasts {

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_CORE_EXPORT PodcastProvider : public Collections::TrackProvider, public Playlists::PlaylistProvider
{
    public:
        static bool couldBeFeed( const QString &urlString );
        static QUrl toFeedUrl( const QString &urlString );

        virtual bool possiblyContainsTrack( const QUrl &url ) const = 0;
        virtual Meta::TrackPtr trackForUrl( const QUrl &url ) = 0;

        /** Special function to get an episode for a given guid.
          *
          * note: this functions is required because QUrl does not preserve every possible guids.
          * This means we can not use trackForUrl().
          * Problematic guids contain non-latin characters, percent encoded parts, capitals, etc.
          */
        virtual Podcasts::PodcastEpisodePtr episodeForGuid( const QString &guid ) = 0;

        virtual void addPodcast( const QUrl &url ) = 0;
        virtual void updateAll() {}

        virtual Podcasts::PodcastChannelPtr addChannel( Podcasts::PodcastChannelPtr channel ) = 0;
        virtual Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode ) = 0;

        virtual Podcasts::PodcastChannelList channels() = 0;

        //TODO: need to move this to SqlPodcastProvider since it's provider specific.
        //perhaps use a more general transferprogress for playlists
        virtual void completePodcastDownloads() = 0;

        // PlaylistProvider methods
        virtual int category() const { return Playlists::PodcastChannelPlaylist; }

        /** convenience function that downcast the argument to PodcastChannel and calls addChannel()
          */
        virtual Playlists::PlaylistPtr addPlaylist( Playlists::PlaylistPtr playlist );

        /** convenience function that downcast the argument to PodcastEpisode and calls addEpisode()
          */
        virtual Meta::TrackPtr addTrack( Meta::TrackPtr track );
};

} //namespace Podcasts

#endif
