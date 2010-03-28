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
#include "core/podcasts/PodcastMeta.h"

#include <kio/jobclasses.h>
#include <KLocale>

class KUrl;
class QAction;

namespace Podcasts {

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT PodcastProvider : public Collections::TrackProvider, public Playlists::PlaylistProvider
{
    //Q_OBJECT
    public:
        static bool couldBeFeed( const QString &urlString );
        static KUrl toFeedUrl( const QString &urlString );

        virtual ~PodcastProvider() {}

        virtual bool possiblyContainsTrack( const KUrl &url ) const = 0;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url ) = 0;

        /** Special function to get an episode for a given guid.
          *
          * note: this functions is required because KUrl does not preserve every possible guids.
          * This means we can not use trackForUrl().
          * Problematic guids contain non-latin characters, percent encoded parts, capitals, etc.
          */
        virtual Podcasts::PodcastEpisodePtr episodeForGuid( const QString &guid ) = 0;

        virtual void addPodcast( const KUrl &url ) = 0;

        virtual Podcasts::PodcastChannelPtr addChannel( Podcasts::PodcastChannelPtr channel ) = 0;
        virtual Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode ) = 0;

        virtual Podcasts::PodcastChannelList channels() = 0;

        virtual void removeSubscription( Podcasts::PodcastChannelPtr channel ) = 0;

        virtual void configureProvider() = 0;
        virtual void configureChannel( Podcasts::PodcastChannelPtr channel ) = 0;

        // PlaylistProvider methods
        virtual QString prettyName() const = 0;
        virtual KIcon icon() const = 0;

        virtual int category() const { return (int)Playlists::PodcastChannelPlaylist; }

        virtual Playlists::PlaylistList playlists() = 0;

        virtual QList<QAction *> episodeActions( Podcasts::PodcastEpisodeList )
            { return QList<QAction *>(); }
        virtual QList<QAction *> channelActions( Podcasts::PodcastChannelList )
            { return QList<QAction *>(); }

        virtual QList<QAction *> providerActions() { return QList<QAction *>(); }
        virtual QList<QAction *> playlistActions( Playlists::PlaylistPtr playlist )
                { Q_UNUSED( playlist ) return QList<QAction *>(); }
        virtual QList<QAction *> trackActions( Playlists::PlaylistPtr playlist,
                                                  int trackIndex )
                { Q_UNUSED( playlist) Q_UNUSED( trackIndex ) return QList<QAction *>(); }

        //TODO: need to move this to SqlPodcastProvider since it's provider specific.
        //perhaps use a more general transferprogress for playlists
        virtual void completePodcastDownloads() = 0;

    public slots:
        virtual void updateAll() = 0;
        virtual void update( Podcasts::PodcastChannelPtr channel ) = 0;
        virtual void downloadEpisode( Podcasts::PodcastEpisodePtr episode ) = 0;
        virtual void deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr episode ) = 0;
        virtual void slotUpdated() = 0;

    signals:
        virtual void updated() = 0;

};

} //namespace Podcasts

#endif
