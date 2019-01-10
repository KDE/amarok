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
        explicit UmsPodcastProvider( const QUrl &scanDirectory );
        ~UmsPodcastProvider() override;

        UmsPodcastEpisodePtr addFile( MetaFile::TrackPtr metafileTrack );
        int addPath( const QString &path );

        bool possiblyContainsTrack( const QUrl &url ) const override;
        Meta::TrackPtr trackForUrl( const QUrl &url ) override;

        Podcasts::PodcastEpisodePtr episodeForGuid( const QString &guid ) override;

        void addPodcast( const QUrl &url ) override;

        Podcasts::PodcastChannelPtr addChannel( const Podcasts::PodcastChannelPtr &channel ) override;
        Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode ) override;

        Podcasts::PodcastChannelList channels() override;

        virtual void removeSubscription( const Podcasts::PodcastChannelPtr &channel );

        virtual void configureProvider();
        virtual void configureChannel( const Podcasts::PodcastChannelPtr &channel );

        // PlaylistProvider methods
        QString prettyName() const override;
        QIcon icon() const override;

        Playlists::PlaylistList playlists() override;

        QActionList playlistActions( const Playlists::PlaylistList &playlists ) override;
        QActionList trackActions( const QMultiHash<Playlists::PlaylistPtr, int> &playlistTracks ) override;

        void completePodcastDownloads() override;

    public Q_SLOTS:
        void updateAll() override;
        virtual void update( const Podcasts::PodcastChannelPtr &channel );
        virtual void downloadEpisode( const Podcasts::PodcastEpisodePtr &episode );
        virtual void deleteDownloadedEpisode( const Podcasts::PodcastEpisodePtr &episode );
        virtual void slotUpdated();
        virtual void scan();

    Q_SIGNALS:
        void updated();

    private Q_SLOTS:
        void slotDeleteEpisodes();
        void slotDeleteChannels();
        void deleteJobComplete( KJob *job );
        void slotCopyComplete( KJob *job );

    private:
        QList<QAction *> episodeActions( const Podcasts::PodcastEpisodeList &);
        QList<QAction *> channelActions( const Podcasts::PodcastChannelList &);
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
