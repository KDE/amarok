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

#include "core/podcasts/PodcastReader.h"
#include "core/podcasts/PodcastProvider.h"
#include "SqlPodcastMeta.h"

#include <kio/jobclasses.h>
#include <klocale.h>

class PodcastImageFetcher;

class KDialog;
class KUrl;
class PodcastReader;
class SqlStorage;
class QTimer;

namespace Podcasts {

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class SqlPodcastProvider : public Podcasts::PodcastProvider
{
    Q_OBJECT
    public:
        SqlPodcastProvider();
        ~SqlPodcastProvider();

        bool possiblyContainsTrack( const KUrl &url ) const;
        Meta::TrackPtr trackForUrl( const KUrl &url );

        QString prettyName() const { return i18n("Local Podcasts"); }
        KIcon icon() const { return KIcon( "server-database" ); }

        Playlists::PlaylistList playlists();

        //PodcastProvider methods
        virtual Podcasts::PodcastEpisodePtr episodeForGuid( const QString &guid );

        void addPodcast( const KUrl &url );

        Podcasts::PodcastChannelPtr addChannel( Podcasts::PodcastChannelPtr channel );
        Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode );

        Podcasts::PodcastChannelList channels();

        void removeSubscription( Podcasts::PodcastChannelPtr channel );

        void configureProvider();
        void configureChannel( Podcasts::PodcastChannelPtr channel );

        QList<QAction *> episodeActions( Podcasts::PodcastEpisodeList );
        QList<QAction *> channelActions( Podcasts::PodcastChannelList );

        virtual QList<QAction *> providerActions();

        void completePodcastDownloads();

        //SqlPodcastProvider specific methods
        Podcasts::SqlPodcastChannelPtr podcastChannelForId( int podcastChannelDbId );

        KUrl baseDownloadDir() const { return m_baseDownloadDir; }

    public slots:
        void updateAll();
        void update( Podcasts::PodcastChannelPtr channel );
        void downloadEpisode( Podcasts::PodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr episode );
        void slotUpdated();

        void slotReadResult( Podcasts::PodcastReader *podcastReader );
        void update( Podcasts::SqlPodcastChannelPtr channel );
        void downloadEpisode( Podcasts::SqlPodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Podcasts::SqlPodcastEpisodePtr episode );

        void slotStatusBarSorryMessage( const QString &message );
        void slotStatusBarNewProgressOperation( KIO::TransferJob * job, const QString & description, Podcasts::PodcastReader* reader );

    private slots:
        void downloadResult( KJob * );
        void addData( KIO::Job *job, const QByteArray & data );
        void redirected( KIO::Job *, const KUrl& );
        void autoUpdate();
        void slotDeleteDownloadedEpisodes();
        void slotDownloadEpisodes();
        void slotConfigureChannel();
        void slotRemoveChannels();
        void slotUpdateChannels();
        void slotDownloadProgress( KJob *job, unsigned long percent );
        void slotWriteTagsToFiles();
        void slotConfigChanged();

    signals:
        void updated();
        void totalPodcastDownloadProgress( int progress );

    private slots:
        void channelImageReady( Podcasts::PodcastChannelPtr, QPixmap );
        void podcastImageFetcherDone( PodcastImageFetcher * );
        void slotConfigureProvider();

    private:
        /** creates all the necessary tables, indexes etc. for the database */
        void createTables() const;
        void loadPodcasts();

        /** @arg string: a url, localUrl or guid in string form */
        Podcasts::SqlPodcastEpisodePtr sqlEpisodeForString( const QString &string );

        void updateDatabase( int fromVersion, int toVersion );
        void fetchImage( Podcasts::SqlPodcastChannelPtr channel );

        /** shows a modal dialog asking the user if he really wants to unsubscribe
            and if he wants to keep the podcast media */
        QPair<bool, bool> confirmUnsubscribe(Podcasts::PodcastChannelPtr channel);

        /** remove the episodes in the list from the filesystem */
        void deleteDownloadedEpisodes( Podcasts::PodcastEpisodeList &episodes );

        void subscribe( const KUrl &url );
        QFile* createTmpFile ( KJob *job );
        void cleanupDownload( KJob *job, bool downloadFailed );

        /** returns true if the file that is downloaded by 'job' is already locally available */
        bool checkEnclosureLocallyAvailable( KIO::Job *job );

        Podcasts::SqlPodcastChannelList m_channels;

        QTimer *m_updateTimer;
        int m_autoUpdateInterval; //interval between autoupdate attempts in minutes
        unsigned int m_updatingChannels;
        unsigned int m_maxConcurrentUpdates;
        Podcasts::PodcastChannelList m_updateQueue;
        QList<KUrl> m_subscribeQueue;

        QHash<KJob *, Podcasts::SqlPodcastEpisodePtr> m_downloadJobMap;
        QHash<KJob *, QString> m_fileNameMap;
        QHash<KJob *, QFile*> m_tmpFileMap;

        Podcasts::SqlPodcastEpisodeList m_downloadQueue;
        int m_maxConcurrentDownloads;
        int m_completedDownloads;

        KUrl m_baseDownloadDir;

        KDialog *m_providerSettingsDialog;

        QList<QAction *> m_providerActions;

        QAction *m_configureChannelAction; //Configure a Channel
        QAction *m_deleteAction; //delete a downloaded Episode
        QAction *m_downloadAction;
        QAction *m_removeAction; //remove a subscription
        QAction *m_renameAction; //rename a Channel or Episode
        QAction *m_updateAction;
        QAction *m_writeTagsAction; //write feed information to downloaded file

        PodcastImageFetcher *m_podcastImageFetcher;
};

} //namespace Podcasts

#endif
