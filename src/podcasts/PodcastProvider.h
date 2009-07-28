/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PODCASTPROVIDER_H
#define PODCASTPROVIDER_H

#include "Collection.h"
#include "PodcastMeta.h"
#include "playlistmanager/PlaylistManager.h"

#include <kio/jobclasses.h>
#include <klocale.h>

class KUrl;
class PopupDropperAction;

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT PodcastProvider : public Amarok::TrackProvider, public PlaylistProvider
{
    //Q_OBJECT
    public:
        virtual ~PodcastProvider() {};

        virtual bool possiblyContainsTrack( const KUrl &url ) const = 0;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url ) = 0;

        virtual void addPodcast( const KUrl &url ) = 0;

        virtual Meta::PodcastChannelPtr addChannel( Meta::PodcastChannelPtr channel ) = 0;
        virtual Meta::PodcastEpisodePtr addEpisode( Meta::PodcastEpisodePtr episode ) = 0;

        virtual Meta::PodcastChannelList channels()= 0;

        virtual void removeSubscription( Meta::PodcastChannelPtr channel ) = 0;

        virtual void configureProvider() = 0;
        virtual void configureChannel( Meta::PodcastChannelPtr channel ) = 0;

        // PlaylistProvider methods
        virtual QString prettyName() const = 0;
        virtual int category() const = 0;

        virtual Meta::PlaylistList playlists() = 0;

        virtual QList<PopupDropperAction *> episodeActions( Meta::PodcastEpisodeList )
            { return QList<PopupDropperAction *>(); }
        virtual QList<PopupDropperAction *> channelActions( Meta::PodcastChannelList )
            { return QList<PopupDropperAction *>(); }

        virtual QList<PopupDropperAction *> playlistActions( Meta::PlaylistPtr playlist )
                { return QList<PopupDropperAction *>(); }
        virtual QList<PopupDropperAction *> trackActions( Meta::PlaylistPtr playlist,
                                                  int trackIndex )
                { return QList<PopupDropperAction *>(); }

    public slots:
        virtual void updateAll() = 0;
        virtual void update( Meta::PodcastChannelPtr channel ) = 0;
        virtual void downloadEpisode( Meta::PodcastEpisodePtr episode ) = 0;
        virtual void deleteDownloadedEpisode( Meta::PodcastEpisodePtr episode ) = 0;
        virtual void slotUpdated() = 0;

    signals:
        virtual void updated() = 0;

};

#endif
