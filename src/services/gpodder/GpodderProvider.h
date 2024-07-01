/****************************************************************************************
 * Copyright (c) 2011 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2011 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2011 Felix Winter <ixos01@gmail.com>                                   *
 * Copyright (c) 2011 Lucas Lira Gomes <x8lucas8x@gmail.com>                            *
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

#ifndef GPODDERPROVIDER_H
#define GPODDERPROVIDER_H

#include "core/podcasts/PodcastProvider.h"
#include "core/podcasts/PodcastReader.h"
#include "GpodderPodcastMeta.h"
#include <mygpo-qt5/ApiRequest.h>
#include <mygpo-qt5/EpisodeActionList.h>
#include "playlistmanager/file/KConfigSyncRelStore.h"
#include "playlistmanager/PlaylistManager.h"

#include <KIO/Job>

#include <QPair>
#include <QQueue>

using namespace mygpo;

class QAction;

namespace Podcasts
{
class GpodderProvider : public PodcastProvider
{
    Q_OBJECT
public:
    GpodderProvider( const QString& username, const QString& devicename, ApiRequest *apiRequest );
    ~GpodderProvider() override;

    //TrackProvider methods
    bool possiblyContainsTrack( const QUrl &url ) const override;
    Meta::TrackPtr trackForUrl( const QUrl &url ) override;

    //PodcastProvider methods
    /** Special function to get an episode for a given guid.
     *
     * note: this functions is required because QUrl does not preserve every possible guids.
     * This means we can not use trackForUrl().
     * Problematic guids contain non-latin characters, percent encoded parts, capitals, etc.
     */
    PodcastEpisodePtr episodeForGuid( const QString &guid ) override;

    void addPodcast( const QUrl &url ) override;

    Podcasts::PodcastChannelPtr addChannel( const Podcasts::PodcastChannelPtr &channel ) override;
    Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode ) override;

    Podcasts::PodcastChannelList channels() override;

    // PlaylistProvider methods
    QString prettyName() const override;
    QIcon icon() const override;
    Playlists::PlaylistList playlists() override;
    void completePodcastDownloads() override;

    /** Copy a playlist to the provider.
    */
    Playlists::PlaylistPtr addPlaylist( Playlists::PlaylistPtr playlist ) override;
    QActionList playlistActions( const Playlists::PlaylistList &playlists ) override;

private Q_SLOTS:
    void requestDeviceUpdates();
    void deviceUpdatesFinished();
    void continueDeviceUpdatesFinished();
    void deviceUpdatesParseError();
    void deviceUpdatesRequestError( QNetworkReply::NetworkError error );

    void requestEpisodeActionsInCascade();
    void episodeActionsInCascadeFinished();
    void episodeActionsInCascadeParseError();
    void episodeActionsInCascadeRequestError( QNetworkReply::NetworkError error );

    void timerGenerateEpisodeAction();
    void timerSynchronizeStatus();
    void timerSynchronizeSubscriptions();
    void timerPrepareToSyncPodcastStatus();

    void slotRemoveChannels();
    void synchronizeStatusParseError();
    void synchronizeStatusRequestError( QNetworkReply::NetworkError error );
    void slotSuccessfulStatusSynchronisation();
    void slotSuccessfulSubscriptionSynchronisation();

    void slotSyncPlaylistAdded( Playlists::PlaylistPtr playlist );
    void slotSyncPlaylistRemoved( Playlists::PlaylistPtr playlist );

    void slotPaused();
    void slotTrackChanged( Meta::TrackPtr track );
    void slotTrackPositionChanged( qint64 position, bool userSeek );

    void requestUrlResolve( GpodderPodcastChannelPtr channel );
    void urlResolvePermanentRedirection ( KIO::Job *job, const QUrl &fromUrl,
            const QUrl &toUrl );
    void urlResolveFinished( KJob * );

    void slotEpisodeDownloaded( Podcasts::PodcastEpisodePtr episode );
    void slotEpisodeDeleted( Podcasts::PodcastEpisodePtr episode );
    void slotEpisodeMarkedAsNew( Podcasts::PodcastEpisodePtr episode );

private:
    QActionList channelActions( PodcastChannelList episodes );

    ApiRequest *m_apiRequest;
    const QString m_username;
    const QString m_deviceName;
    PodcastChannelList m_channels;
    KIO::TransferJob *m_resolveUrlJob;

    AddRemoveResultPtr m_addRemoveResult;
    DeviceUpdatesPtr m_deviceUpdatesResult;
    AddRemoveResultPtr m_episodeActionsResult;
    EpisodeActionListPtr m_episodeActionListResult;

    qulonglong m_timestampStatus;
    qulonglong m_timestampSubscription;

    qulonglong subscriptionTimestamp();
    void setSubscriptionTimestamp( qulonglong newTimestamp );

    void removeChannel( const QUrl &url );
    void createPlayStatusBookmark();

    void synchronizeStatus();
    void synchronizeSubscriptions();
    void updateLocalPodcasts( const QList< QPair<QUrl,QUrl> > updatedUrls );

    KConfigGroup gpodderActionsConfig() const;
    void loadCachedEpisodeActions();
    void saveCachedEpisodeActions();

    KConfigGroup gpodderPodcastsConfig() const;
    void loadCachedPodcastsChanges();
    void saveCachedPodcastsChanges();

    QAction *m_removeAction;

    //Lists of podcasts to be added or removed from gpodder.net
    QList<QUrl> m_addList;
    QList<QUrl> m_removeList;

    QUrl resolvedPodcastUrl( const PodcastEpisodePtr episode );

    QMap<QUrl,QUrl> m_redirectionUrlMap;
    QQueue<QUrl> m_channelsToRequestActions;
    QMap<KIO::TransferJob *,GpodderPodcastChannelPtr> m_resolvedPodcasts;
    //Used as a temporary container for podcasts with already urls resolved
    //before adding them to m_channels
    QQueue<GpodderPodcastChannelPtr> m_resolvedChannelsToBeAdded;

    QMap<QUrl,EpisodeActionPtr> m_episodeStatusMap;
    QMap<QUrl,EpisodeActionPtr> m_uploadEpisodeStatusMap;

    QTimer *m_timerGeneratePlayAction;
    QTimer *m_timerSynchronizeStatus;
    QTimer *m_timerSynchronizeSubscriptions;

    Meta::TrackPtr m_trackToSyncStatus;
};

}

#endif
