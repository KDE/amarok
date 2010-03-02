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

#include "podcasts/PodcastProvider.h"
#include "UmsHandler.h"
#include "UmsPodcastMeta.h"

class KJob;

class UmsPodcastProvider : public PodcastProvider
{
    Q_OBJECT
    public:
        UmsPodcastProvider( Meta::UmsHandler *handler, KUrl scanDirectory );
        ~UmsPodcastProvider();

        UmsPodcastEpisodePtr addFile( MetaFile::TrackPtr metafileTrack );
        int addPath( const QString &path );

        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        virtual void addPodcast( const KUrl &url );

        virtual Meta::PodcastChannelPtr addChannel( Meta::PodcastChannelPtr channel );
        virtual Meta::PodcastEpisodePtr addEpisode( Meta::PodcastEpisodePtr episode );

        virtual Meta::PodcastChannelList channels();

        virtual void removeSubscription( Meta::PodcastChannelPtr channel );

        virtual void configureProvider();
        virtual void configureChannel( Meta::PodcastChannelPtr channel );

        // PlaylistProvider methods
        virtual QString prettyName() const;
        virtual KIcon icon() const;

        virtual Meta::PlaylistList playlists();

        virtual QList<QAction *> episodeActions( Meta::PodcastEpisodeList );
        virtual QList<QAction *> channelActions( Meta::PodcastChannelList );

        virtual QList<QAction *> playlistActions( Meta::PlaylistPtr playlist );
        virtual QList<QAction *> trackActions( Meta::PlaylistPtr playlist,
                                                  int trackIndex );

        virtual void completePodcastDownloads();

    public slots:
        virtual void updateAll();
        virtual void update( Meta::PodcastChannelPtr channel );
        virtual void downloadEpisode( Meta::PodcastEpisodePtr episode );
        virtual void deleteDownloadedEpisode( Meta::PodcastEpisodePtr episode );
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

        void deleteEpisodes( UmsPodcastEpisodeList umsEpisodes );

        Meta::UmsHandler *m_handler;
        KUrl m_scanDirectory;
        QStringList m_dirList;

        UmsPodcastChannelList m_umsChannels;

        QAction *m_deleteEpisodeAction; //delete a downloaded Episode
        QAction *m_deleteChannelAction; //delete a everything from one channel
        QList<QAction *> m_providerActions;

        QMap<KJob *,UmsPodcastEpisodeList> m_deleteJobMap;
};

#endif // UMSPODCASTPROVIDER_H
