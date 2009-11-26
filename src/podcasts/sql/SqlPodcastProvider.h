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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef SQLPODCASTPROVIDER_H
#define SQLPODCASTPROVIDER_H

#include "EngineObserver.h"
#include "PodcastProvider.h"
#include "SqlPodcastMeta.h"

#include <kio/jobclasses.h>
#include <klocale.h>

class PodcastImageFetcher;

class KUrl;
class PodcastReader;
class SqlStorage;
class QTimer;

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class SqlPodcastProvider : public PodcastProvider, public EngineObserver
{
    Q_OBJECT
    public:
        SqlPodcastProvider();
        ~SqlPodcastProvider();

        bool possiblyContainsTrack( const KUrl &url ) const;
        Meta::TrackPtr trackForUrl( const KUrl &url );

        QString prettyName() const { return i18n("Local Podcasts"); };
        KIcon icon() const { return KIcon( "server-database" ); }

        int category() const { return (int)PlaylistManager::PodcastChannel; };

        Meta::PlaylistList playlists();

        //PodcastProvider methods
        void addPodcast( const KUrl &url );

        Meta::PodcastChannelPtr addChannel( Meta::PodcastChannelPtr channel );
        Meta::PodcastEpisodePtr addEpisode( Meta::PodcastEpisodePtr episode );

        Meta::PodcastChannelList channels();

        void removeSubscription( Meta::PodcastChannelPtr channel );

        void configureProvider();
        void configureChannel( Meta::PodcastChannelPtr channel );

        QList<QAction *> episodeActions( Meta::PodcastEpisodeList );
        QList<QAction *> channelActions( Meta::PodcastChannelList );

        void completePodcastDownloads();

        //EngineObserver methods
        virtual void engineNewTrackPlaying();

        //SqlPodcastProvider specific methods
        Meta::SqlPodcastChannelPtr podcastChannelForId( int podcastChannelDbId );

    public slots:
        void updateAll();
        void update( Meta::PodcastChannelPtr channel );
        void downloadEpisode( Meta::PodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Meta::PodcastEpisodePtr episode );
        void slotUpdated();

        void slotReadResult( PodcastReader *podcastReader );
        void update( Meta::SqlPodcastChannelPtr channel );
        void downloadEpisode( Meta::SqlPodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Meta::SqlPodcastEpisodePtr episode );

    private slots:
        void downloadResult( KJob * );
        void addData( KIO::Job * job, const QByteArray & data );
        void redirected( KIO::Job *, const KUrl& );
        void autoUpdate();
        void slotDeleteSelectedEpisodes();
        void slotDownloadEpisodes();
        void slotConfigureChannel();
        void slotRemoveChannels();
        void slotUpdateChannels();
        void slotDownloadProgress( KJob *job, unsigned long percent );
        void slotWriteTagsToFiles();

    signals:
        void updated();
        void totalPodcastDownloadProgress( int progress );

    private slots:
        void channelImageReady( Meta::PodcastChannelPtr, QPixmap );
        void podcastImageFetcherDone( PodcastImageFetcher * );

    private:
        /** creates all the necessary tables, indexes etc. for the database */
        void createTables() const;
        void loadPodcasts();
        void updateDatabase( int fromVersion, int toVersion );
        void fetchImage( Meta::SqlPodcastChannelPtr channel );

        void deleteEpisodes( Meta::PodcastEpisodeList & episodes );

        void subscribe( const KUrl &url );
        QFile* createTmpFile ( KJob* job );
        void cleanupDownload( KJob* job, bool downloadFailed );

        Meta::SqlPodcastChannelList m_channels;

        QTimer *m_updateTimer;
        unsigned int m_autoUpdateInterval; //interval between autoupdate attempts in minutes
        unsigned int m_updatingChannels;
        unsigned int m_maxConcurrentUpdates;
        Meta::PodcastChannelList m_updateQueue;
        QList<KUrl> m_subscribeQueue;

        QHash<KJob *, Meta::SqlPodcastEpisode *> m_downloadJobMap;
        QHash<KJob *, QString> m_fileNameMap;
        QHash<KJob *, QFile*> m_tmpFileMap;

        Meta::SqlPodcastEpisodeList m_downloadQueue;
        int m_maxConcurrentDownloads;
        int m_completedDownloads;

        QAction * m_configureAction; //Configure a Channel
        QAction * m_deleteAction; //delete a downloaded Episode
        QAction * m_downloadAction;
        QAction * m_removeAction; //remove a subscription
        QAction * m_renameAction; //rename a Channel or Episode
        QAction * m_updateAction;
        QAction * m_writeTagsAction; //write feed information to downloaded file

        PodcastImageFetcher *m_podcastImageFetcher;
};

#endif
