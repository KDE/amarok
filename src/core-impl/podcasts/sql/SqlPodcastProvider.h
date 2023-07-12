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

#include "core/podcasts/PodcastProvider.h"
#include "core/podcasts/PodcastReader.h"
#include "SqlPodcastMeta.h"

#include <QIcon>

#include <KLocalizedString>

class PodcastImageFetcher;

class QDialog;
class QUrl;
class PodcastReader;
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
        ~SqlPodcastProvider() override;

        //TrackProvider methods
        bool possiblyContainsTrack( const QUrl &url ) const override;
        Meta::TrackPtr trackForUrl( const QUrl &url ) override;

        //PlaylistProvider methods
        QString prettyName() const override { return i18n("Local Podcasts"); }
        QIcon icon() const override { return QIcon::fromTheme( QStringLiteral("server-database") ); }

        Playlists::PlaylistList playlists() override;

        //PlaylistProvider methods
        QActionList providerActions() override;
        QActionList playlistActions( const Playlists::PlaylistList &playlists ) override;
        QActionList trackActions( const QMultiHash<Playlists::PlaylistPtr, int> &playlistTracks ) override;

        //PodcastProvider methods
        Podcasts::PodcastEpisodePtr episodeForGuid( const QString &guid ) override;

        void addPodcast( const QUrl &url ) override;

        Podcasts::PodcastChannelPtr addChannel( const Podcasts::PodcastChannelPtr &channel ) override;
        Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode ) override;

        Podcasts::PodcastChannelList channels() override;

        void completePodcastDownloads() override;

        //SqlPodcastProvider specific methods
        virtual Podcasts::SqlPodcastChannelPtr podcastChannelForId( int podcastChannelDbId );

        virtual QUrl baseDownloadDir() const { return m_baseDownloadDir; }

    public Q_SLOTS:
        void updateAll() override;
        void downloadEpisode( const Podcasts::PodcastEpisodePtr &episode );
        void deleteDownloadedEpisode( const Podcasts::PodcastEpisodePtr &episode );

        void slotReadResult( PodcastReader *podcastReader );
        void downloadEpisode( Podcasts::SqlPodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Podcasts::SqlPodcastEpisodePtr episode );

    private Q_SLOTS:
        void downloadResult( KJob * );
        void addData( KIO::Job *job, const QByteArray & data );
        void redirected( KIO::Job *, const QUrl& );
        void autoUpdate();
        void slotDeleteDownloadedEpisodes();
        void slotDownloadEpisodes();
        void slotSetKeep();
        void slotConfigureChannel();
        void slotRemoveChannels();
        void slotUpdateChannels();
        void slotDownloadProgress( KJob *job, unsigned long percent );
        void slotWriteTagsToFiles();
        void slotConfigChanged();
        void slotExportOpml();

    Q_SIGNALS:
        void totalPodcastDownloadProgress( int progress );

        //SqlPodcastProvider signals
        void episodeDownloaded( Podcasts::PodcastEpisodePtr );
        void episodeDeleted( Podcasts::PodcastEpisodePtr );

    private Q_SLOTS:
        void channelImageReady( Podcasts::PodcastChannelPtr, const QImage &);
        void podcastImageFetcherDone( PodcastImageFetcher * );
        void slotConfigureProvider();

        void slotStatusBarNewProgressOperation( KIO::TransferJob * job,
                                                               const QString &description,
                                                               Podcasts::PodcastReader* reader );
        void slotStatusBarErrorMessage( const QString &message );
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
        void fetchImage( const Podcasts::SqlPodcastChannelPtr &channel );

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

        void subscribe( const QUrl &url );
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
        QList<QUrl> m_subscribeQueue;

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

        QUrl m_baseDownloadDir;

        QDialog *m_providerSettingsDialog;
        Ui::SqlPodcastProviderSettingsWidget *m_providerSettingsWidget;

        QList<QAction *> m_providerActions;

        QAction *m_configureChannelAction; //Configure a Channel
        QAction *m_deleteAction; //delete a downloaded Episode
        QAction *m_downloadAction;
        QAction *m_keepAction;
        QAction *m_removeAction; //remove a subscription
        QAction *m_updateAction;
        QAction *m_writeTagsAction; //write feed information to downloaded file

        PodcastImageFetcher *m_podcastImageFetcher;
};

} //namespace Podcasts

#endif
