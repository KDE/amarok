/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef UMSPODCASTPROVIDER_H
#define UMSPODCASTPROVIDER_H

#include "core/podcasts/PodcastProvider.h"
#include "UmsPodcastMeta.h"

class KJob;

namespace Podcasts {

class UmsPodcastProvider : public PodcastProvider
{
    Q_OBJECT
    public:
        UmsPodcastProvider( QUrl scanDirectory );
        ~UmsPodcastProvider();

        UmsPodcastEpisodePtr addFile( MetaFile::TrackPtr metafileTrack );
        int addPath( const QString &path );

        virtual bool possiblyContainsTrack( const QUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const QUrl &url );

        virtual Podcasts::PodcastEpisodePtr episodeForGuid( const QString &guid );

        virtual void addPodcast( const QUrl &url );

        virtual Podcasts::PodcastChannelPtr addChannel( Podcasts::PodcastChannelPtr channel );
        virtual Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode );

        virtual Podcasts::PodcastChannelList channels();

        virtual void removeSubscription( Podcasts::PodcastChannelPtr channel );

        virtual void configureProvider();
        virtual void configureChannel( Podcasts::PodcastChannelPtr channel );

        // PlaylistProvider methods
        virtual QString prettyName() const;
        virtual QIcon icon() const;

        virtual Playlists::PlaylistList playlists();

        virtual QActionList playlistActions( const Playlists::PlaylistList &playlists );
        virtual QActionList trackActions( const QMultiHash<Playlists::PlaylistPtr, int> &playlistTracks );

        virtual void completePodcastDownloads();

    public slots:
        virtual void updateAll();
        virtual void update( Podcasts::PodcastChannelPtr channel );
        virtual void downloadEpisode( Podcasts::PodcastEpisodePtr episode );
        virtual void deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr episode );
        virtual void slotUpdated();
        virtual void scan();

    signals:
        void updated();

    private slots:
        void slotDeleteEpisodes();
        void slotDeleteChannels();
        void deleteJobComplete( KJob *job );
        void slotCopyComplete( KJob *job );

    private:
        QList<QAction *> episodeActions( Podcasts::PodcastEpisodeList );
        QList<QAction *> channelActions( Podcasts::PodcastChannelList );
        void deleteEpisodes( UmsPodcastEpisodeList umsEpisodes );

        QUrl m_scanDirectory;
        QStringList m_dirList;

        UmsPodcastChannelList m_umsChannels;

        QAction *m_deleteEpisodeAction; //delete a downloaded Episode
        QAction *m_deleteChannelAction; //delete a everything from one channel
        QList<QAction *> m_providerActions;

        QMap<KJob *,UmsPodcastEpisodeList> m_deleteJobMap;
};

} //namespace Podcasts

#endif // UMSPODCASTPROVIDER_H
