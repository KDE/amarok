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

#include <KIcon>
#include <KLocale>

class PodcastImageFetcher;

class KDialog;
class KUrl;
class PodcastReader;
class SqlStorage;
class QTimer;

namespace Ui {
    class SqlPodcastProviderSettingsWidget;
}

namespace Podcasts {

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT SqlPodcastProvider : public Podcasts::PodcastProvider
{
    Q_OBJECT
    public:
        SqlPodcastProvider();
        virtual ~SqlPodcastProvider();

        //TrackProvider methods
        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        //PlaylistProvider methods
        virtual QString prettyName() const { return i18n("Local Podcasts"); }
        virtual KIcon icon() const { return KIcon( "server-database" ); }

        virtual Playlists::PlaylistList playlists();

        //PlaylistProvider methods
        virtual QList<QAction *> providerActions();
        virtual QList<QAction *> playlistActions( Playlists::PlaylistPtr playlist );
        virtual QList<QAction *> trackActions( Playlists::PlaylistPtr playlist, int trackIndex );

        //PodcastProvider methods
        virtual Podcasts::PodcastEpisodePtr episodeForGuid( const QString &guid );

        virtual void addPodcast( const KUrl &url );

        virtual Podcasts::PodcastChannelPtr addChannel( Podcasts::PodcastChannelPtr channel );
        virtual Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode );

        virtual Podcasts::PodcastChannelList channels();

        virtual QList<QAction *> episodeActions( Podcasts::PodcastEpisodeList );
        virtual QList<QAction *> channelActions( Podcasts::PodcastChannelList );

        virtual void completePodcastDownloads();

        //SqlPodcastProvider specific methods
        virtual Podcasts::SqlPodcastChannelPtr podcastChannelForId( int podcastChannelDbId );

        virtual KUrl baseDownloadDir() const { return m_baseDownloadDir; }

    public slots:
        void updateAll();
        void downloadEpisode( Podcasts::PodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr episode );

        void slotReadResult( PodcastReader *podcastReader );
        void downloadEpisode( Podcasts::SqlPodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Podcasts::SqlPodcastEpisodePtr episode );

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
        void slotExportOpml();

    signals:
        void totalPodcastDownloadProgress( int progress );

        //SqlPodcastProvider signals
        void episodeDownloaded( Podcasts::PodcastEpisodePtr );
        void episodeDeleted( Podcasts::PodcastEpisodePtr );

    private slots:
        void channelImageReady( Podcasts::PodcastChannelPtr, QImage );
        void podcastImageFetcherDone( PodcastImageFetcher * );
        void slotConfigureProvider();

        void slotStatusBarNewProgressOperation( KIO::TransferJob * job,
                                                               const QString &description,
                                                               Podcasts::PodcastReader* reader );
        void slotStatusBarSorryMessage( const QString &message );
        void slotOpmlWriterDone( int result );

    private:
        void startTimer();
        void configureProvider();
        void configureChannel( Podcasts::SqlPodcastChannelPtr channel );
        void updateSqlChannel( Podcasts::SqlPodcastChannelPtr channel );

        /** creates all the necessary tables, indexes etc. for the database */
        void createTables() const;
        void loadPodcasts();

        /** @arg string: a url, localUrl or guid in string form */
        Podcasts::SqlPodcastEpisodePtr sqlEpisodeForString( const QString &string );

        void updateDatabase( int fromVersion, int toVersion );
        void fetchImage( Podcasts::SqlPodcastChannelPtr channel );

        /** shows a modal dialog asking the user if he really wants to unsubscribe
            and if he wants to keep the podcast media */
        QPair<bool, bool> confirmUnsubscribe( Podcasts::SqlPodcastChannelPtr channel );

        /** remove the episodes in the list from the filesystem */
        void deleteDownloadedEpisodes( Podcasts::SqlPodcastEpisodeList &episodes );

        void moveDownloadedEpisodes( Podcasts::SqlPodcastChannelPtr channel );

        /** Removes a podcast from the list. Will ask for confirmation to delete the episodes
          * as well
          */
        void removeSubscription( Podcasts::SqlPodcastChannelPtr channel );

        void subscribe( const KUrl &url );
        QFile* createTmpFile ( Podcasts::SqlPodcastEpisodePtr sqlEpisode );
        void cleanupDownload( KJob *job, bool downloadFailed );

        /** returns true if the file that is downloaded by 'job' is already locally available */
        bool checkEnclosureLocallyAvailable( KIO::Job *job );

        Podcasts::SqlPodcastChannelList m_channels;

        QTimer *m_updateTimer;
        int m_autoUpdateInterval; //interval between autoupdate attempts in minutes
        unsigned int m_updatingChannels;
        unsigned int m_maxConcurrentUpdates;
        Podcasts::SqlPodcastChannelList m_updateQueue;
        QList<KUrl> m_subscribeQueue;

        struct PodcastEpisodeDownload {
            Podcasts::SqlPodcastEpisodePtr episode;
            QFile *tmpFile;
            QString fileName;
            bool finalNameReady;
        };

        QHash<KJob *, struct PodcastEpisodeDownload> m_downloadJobMap;

        Podcasts::SqlPodcastEpisodeList m_downloadQueue;
        int m_maxConcurrentDownloads;
        int m_completedDownloads;

        KUrl m_baseDownloadDir;

        KDialog *m_providerSettingsDialog;
        Ui::SqlPodcastProviderSettingsWidget *m_providerSettingsWidget;

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
