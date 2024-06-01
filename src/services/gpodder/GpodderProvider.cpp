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

#define DEBUG_PREFIX "GpodderProvider"

#include "GpodderProvider.h"

#include "core-impl/capabilities/timecode/TimecodeWriteCapability.h"
#include "core-impl/podcasts/sql/SqlPodcastProvider.h"
#include "core/logger/Logger.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "gpodder/GpodderServiceConfig.h"
#include "NetworkAccessManagerProxy.h"
#include "PodcastModel.h"

#include <QAction>
#include <QLabel>
#include <QNetworkConfigurationManager>
#include <QTimer>

#include <KIO/TransferJob>


using namespace Podcasts;

GpodderProvider::GpodderProvider( const QString& username,
                                  const QString& devicename,
                                  ApiRequest *apiRequest )
    : m_apiRequest( apiRequest )
    , m_username( username )
    , m_deviceName( devicename )
    , m_channels()
    , m_addRemoveResult()
    , m_deviceUpdatesResult()
    , m_episodeActionListResult()
    , m_timestampStatus( 0 )
    , m_timestampSubscription( subscriptionTimestamp() )
    , m_removeAction( nullptr )
    , m_addList()
    , m_removeList()
    , m_timerGeneratePlayAction( new QTimer( this ) )
    , m_timerSynchronizeStatus( new QTimer( this ) )
    , m_timerSynchronizeSubscriptions( new QTimer( this ) )
{
    //We have to load episode actions and podcasts subscriptions changes
    //that weren't uploaded before the time we closed amarok
    loadCachedEpisodeActions();
    loadCachedPodcastsChanges();

    //Request all channels and episodes from m_devicename device and after it
    //request episode actions too
    requestDeviceUpdates();

    //Connect default podcasts signals to make possible to ask the user if he wants
    //to upload a new local podcast to gpodder.net
    connect( The::playlistManager()->defaultPodcasts(), &PodcastProvider::playlistAdded,
             this, &GpodderProvider::slotSyncPlaylistAdded );
    connect( The::playlistManager()->defaultPodcasts(), &PodcastProvider::playlistRemoved,
             this, &GpodderProvider::slotSyncPlaylistRemoved );

    Podcasts::SqlPodcastProvider *sqlPodcastProvider;

    sqlPodcastProvider = dynamic_cast<Podcasts::SqlPodcastProvider *>
                        ( The::playlistManager()->defaultPodcasts() );

    connect( The::podcastModel(), &PlaylistBrowserNS::PodcastModel::episodeMarkedAsNew,
             this, &GpodderProvider::slotEpisodeMarkedAsNew );

    if( sqlPodcastProvider )
    {
        connect( sqlPodcastProvider, &SqlPodcastProvider::episodeDeleted,
                 this, &GpodderProvider::slotEpisodeDeleted );
        connect( sqlPodcastProvider,&SqlPodcastProvider::episodeDownloaded,
                 this, &GpodderProvider::slotEpisodeDownloaded );
    }

    //Connect engine controller signals to make possible to synchronize podcast status
    connect( The::engineController(), &EngineController::trackChanged,
             this, &GpodderProvider::slotTrackChanged );
    connect( The::engineController(), &EngineController::trackPositionChanged,
             this, &GpodderProvider::slotTrackPositionChanged );
    connect( The::engineController(), &EngineController::paused,
             this, &GpodderProvider::slotPaused );

    //These timers will periodically synchronize data between local podcasts and gpodder.net
    connect( m_timerSynchronizeStatus, &QTimer::timeout,
             this, &GpodderProvider::timerSynchronizeStatus );
    connect( m_timerSynchronizeSubscriptions, &QTimer::timeout,
             this, &GpodderProvider::timerSynchronizeSubscriptions );
    connect( m_timerGeneratePlayAction, &QTimer::timeout,
             this, &GpodderProvider::timerGenerateEpisodeAction );

    m_timerGeneratePlayAction->stop();
    m_timerSynchronizeStatus->stop();
    m_timerSynchronizeSubscriptions->stop();
}

GpodderProvider::~GpodderProvider()
{
    delete m_timerGeneratePlayAction;
    delete m_timerSynchronizeStatus;
    delete m_timerSynchronizeSubscriptions;

    //Save cached episode actions and Podcast changes, in order to
    //upload them to gpodder.net in the next
    saveCachedEpisodeActions();
    saveCachedPodcastsChanges();

    m_uploadEpisodeStatusMap.clear();
    m_episodeStatusMap.clear();
    m_redirectionUrlMap.clear();

    m_channels.clear();
}

bool
GpodderProvider::possiblyContainsTrack( const QUrl &url ) const
{
    DEBUG_BLOCK

    foreach( PodcastChannelPtr ptr, m_channels )
    {
        foreach( PodcastEpisodePtr episode, ptr->episodes() )
        {
            if( episode->uidUrl() == url.url() )
                return true;
        }
    }

    return false;
}

Meta::TrackPtr
GpodderProvider::trackForUrl( const QUrl &url )
{
    DEBUG_BLOCK

    if( url.isEmpty() )
        return Meta::TrackPtr();

    foreach( PodcastChannelPtr podcast, m_channels )
    {
        foreach( PodcastEpisodePtr episode, podcast->episodes() )
        {
            if( episode->uidUrl() == url.url() )
            {
                return Meta::TrackPtr::dynamicCast( episode );
            }
        }
    }

    return Meta::TrackPtr();
}

PodcastEpisodePtr
GpodderProvider::episodeForGuid( const QString &guid )
{
    foreach( PodcastChannelPtr ptr, m_channels )
    {
        foreach( PodcastEpisodePtr episode, ptr->episodes() )
        {
            if( episode->guid() == guid )
                return episode;
        }
    }

    return PodcastEpisodePtr();
}

void
GpodderProvider::addPodcast( const QUrl &url )
{
    Q_UNUSED( url )
}

Playlists::PlaylistPtr
GpodderProvider::addPlaylist( Playlists::PlaylistPtr playlist )
{
    DEBUG_BLOCK

    PodcastChannelPtr channel = PodcastChannelPtr::dynamicCast( playlist );
    if( channel.isNull() )
        return Playlists::PlaylistPtr();

    //This function is executed every time a new channel is found on gpodder.net
    PodcastChannelPtr master;
    PodcastChannelPtr slave;

    foreach( PodcastChannelPtr tempChannel,
             The::playlistManager()->defaultPodcasts()->channels() )
        if( tempChannel->url() == channel->url() )
            master = tempChannel;

    foreach( PodcastChannelPtr tempChannel, this->channels() )
        if( tempChannel->url() == channel->url() )
            slave = tempChannel;

    if( !master )
        master =  The::playlistManager()->defaultPodcasts()->addChannel( channel );

    if( !slave )
    {
        slave = this->addChannel( master );

        //If playlist is not a GpodderPodcastChannelPtr then we must subscribe
        //it in gpodder.net
        if( !GpodderPodcastChannelPtr::dynamicCast( playlist ) )
        {
            //The service will try to subscribe this podcast in gpodder.net in
            //the next synchronization
            QUrl url = QUrl( slave->url().url() );
            m_removeList.removeAll( url );
            m_addList << url;
        }
    }

    //Create a playlist synchronization between master and slave
    The::playlistManager()->setupSync( Playlists::PlaylistPtr::dynamicCast( master ),
                                       Playlists::PlaylistPtr::dynamicCast( slave )
                                       );

    return Playlists::PlaylistPtr::dynamicCast( slave );
}


PodcastChannelPtr
GpodderProvider::addChannel( const PodcastChannelPtr &channel )
{
    DEBUG_BLOCK

    GpodderPodcastChannelPtr gpodderChannel( new GpodderPodcastChannel( this, channel ) );

    m_channels << PodcastChannelPtr::dynamicCast( gpodderChannel );

    Q_EMIT playlistAdded( Playlists::PlaylistPtr::dynamicCast( gpodderChannel ) );

    return PodcastChannelPtr::dynamicCast( gpodderChannel );
}

PodcastEpisodePtr
GpodderProvider::addEpisode( PodcastEpisodePtr episode )
{
    if( episode.isNull() )
        return PodcastEpisodePtr();

    if( episode->channel().isNull() )
    {
        debug() << "channel is null";

        return PodcastEpisodePtr();
    }

    return episode;
}

PodcastChannelList
GpodderProvider::channels()
{
    DEBUG_BLOCK

    PodcastChannelList list;

    foreach( PodcastChannelPtr channel, m_channels )
        list << PodcastChannelPtr::dynamicCast( channel );

    return list;
}

QString
GpodderProvider::prettyName() const
{
    return i18n( "Gpodder Podcasts" );
}

QIcon
GpodderProvider::icon() const
{
    return QIcon::fromTheme( "view-services-gpodder-amarok" );
}

Playlists::PlaylistList
GpodderProvider::playlists()
{
    Playlists::PlaylistList playlists;

    foreach( PodcastChannelPtr channel, m_channels )
        playlists << Playlists::PlaylistPtr::staticCast( channel );

    return playlists;
}

void
GpodderProvider::completePodcastDownloads()
{
}

void
GpodderProvider::removeChannel( const QUrl &url )
{
    for( int i = 0; i < m_channels.size(); i++ )
    {
        if( m_channels.at( i )->url() == url )
        {
            PodcastChannelPtr channel = m_channels.at( i );
            QUrl url = QUrl( channel->url().url() );

            m_channels.removeAll( channel );
            m_episodeStatusMap.remove( url );
            m_uploadEpisodeStatusMap.remove( url );
            m_addList.removeAll( url );

            Q_EMIT playlistRemoved(
                        Playlists::PlaylistPtr::dynamicCast( channel ) );

            return;
        }
    }
}

QActionList
GpodderProvider::channelActions( PodcastChannelList channels )
{
    QActionList actions;
    if( channels.isEmpty() )
        return actions;

    if( m_removeAction == nullptr )
    {
        m_removeAction = new QAction( QIcon::fromTheme( "edit-delete" ),
                i18n( "&Delete Channel and Episodes" ), this );
        m_removeAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_removeAction,  SIGNAL(triggered()), SLOT(slotRemoveChannels()) );
    }
    //Set the episode list as data that we'll retrieve in the slot
    m_removeAction->setData( QVariant::fromValue( channels ) );
    actions << m_removeAction;

    return actions;
}

QActionList
GpodderProvider::playlistActions( const Playlists::PlaylistList &playlists )
{
    PodcastChannelList channels;
    foreach( const Playlists::PlaylistPtr &playlist, playlists )
    {
        PodcastChannelPtr channel = PodcastChannelPtr::dynamicCast( playlist );
        if( channel )
            channels << channel;
    }

    return channelActions( channels );
}

void
GpodderProvider::slotRemoveChannels()
{
    DEBUG_BLOCK

    QAction *action = qobject_cast<QAction *>( QObject::sender() );

    if( action == nullptr )
        return;

    PodcastChannelList channels = action->data().value<PodcastChannelList>();
    action->setData( QVariant() );      //Clear data

    foreach( PodcastChannelPtr channel, channels )
    {
        removeChannel( channel->url() );

        //The service will try to unsubscribe this podcast from gpodder.net
        //in the next synchronization
        m_removeList << channel->url();
    }
}

void
GpodderProvider::slotSyncPlaylistAdded( Playlists::PlaylistPtr playlist )
{
    PodcastChannelPtr channel = Podcasts::PodcastChannelPtr::dynamicCast( playlist );
    //If the new channel already exist in gpodder channels, then
    //we don't have to add it to gpodder.net again
    foreach( PodcastChannelPtr tempChannel, m_channels )
        if( channel->url() == tempChannel->url() )
            return;

    addPlaylist( playlist );
    m_timerSynchronizeSubscriptions->start( 60000 );
}

void
GpodderProvider::slotSyncPlaylistRemoved( Playlists::PlaylistPtr playlist )
{
    Podcasts::PodcastChannelPtr channel = Podcasts::PodcastChannelPtr::dynamicCast( playlist );
    //If gpodder channels doesn't contains the removed channel from default
    //podcast provider, then we don't have to remove it from gpodder.net
    foreach( PodcastChannelPtr tempChannel, m_channels )
        if( channel->url() == tempChannel->url() )
        {
            removeChannel( tempChannel->url() );

            //The service will try to unsubscribe this podcast from gpodder.net
            //in the next synchronization
            m_removeList << tempChannel->url();
            m_timerSynchronizeSubscriptions->start( 60000 );
            return;
        }
}

qulonglong
GpodderProvider::subscriptionTimestamp()
{
    KConfigGroup config = Amarok::config( GpodderServiceConfig::configSectionName() );
    return config.readEntry( "subscriptionTimestamp", 0 );
}

void
GpodderProvider::setSubscriptionTimestamp( qulonglong newTimestamp )
{
    KConfigGroup config = Amarok::config( GpodderServiceConfig::configSectionName() );
    config.writeEntry( "subscriptionTimestamp", newTimestamp );
}

void GpodderProvider::synchronizeStatus()
{
    DEBUG_BLOCK

    debug() << "new episodes status: " << m_uploadEpisodeStatusMap.size();

    if( !QNetworkConfigurationManager().isOnline() )
        return;

    if( !m_uploadEpisodeStatusMap.isEmpty() )
    {
        m_episodeActionsResult =
                m_apiRequest->uploadEpisodeActions( m_username,
                                                    m_uploadEpisodeStatusMap.values() );

        //Only clear m_episodeStatusList if the synchronization with gpodder.net really worked
        connect( m_episodeActionsResult.data(), SIGNAL(finished()),
                 SLOT(slotSuccessfulStatusSynchronisation()) );
        connect( m_episodeActionsResult.data(),
                 SIGNAL(requestError(QNetworkReply::NetworkError)),
                 SLOT(synchronizeStatusRequestError(QNetworkReply::NetworkError)) );
        connect( m_episodeActionsResult.data(), SIGNAL(parseError()),
                 SLOT(synchronizeStatusParseError()) );

        Amarok::Logger::shortMessage( i18n( "Trying to synchronize statuses with gpodder.net" ) );
    }
    else
        m_timerSynchronizeStatus->stop();
}

void GpodderProvider::slotSuccessfulStatusSynchronisation()
{
    DEBUG_BLOCK

    m_timestampStatus = QDateTime::currentMSecsSinceEpoch();

    m_uploadEpisodeStatusMap.clear();

    //In addition, the server MUST send any URLs that have been rewritten (sanitized, see bug:747)
    //as a list of tuples with the key "update_urls". The client SHOULD parse this list and update
    //the local subscription list accordingly (the server only sanitizes the URL, so the semantic
    //"content" should stay the same and therefore the client can simply update the URL value
    //locally and use it for future updates
    updateLocalPodcasts( m_episodeActionsResult->updateUrlsList() );
}

void GpodderProvider::synchronizeStatusParseError()
{
    DEBUG_BLOCK

    QTimer::singleShot( 20000, this, SLOT(timerSynchronizeStatus()) );

    debug() << "synchronizeStatus [Status Synchronization] - Parse error";
}

void GpodderProvider::synchronizeStatusRequestError(QNetworkReply::NetworkError error)
{
    DEBUG_BLOCK

    QTimer::singleShot( 20000, this, SLOT(timerSynchronizeStatus()) );

    debug() << "synchronizeStatus [Status Synchronization] - Request error nr.: " << error;
}

void
GpodderProvider::synchronizeSubscriptions()
{
    DEBUG_BLOCK

    debug() << "add: " << m_addList.size();
    debug() << "remove: " << m_removeList.size();

    if( !QNetworkConfigurationManager().isOnline() )
        return;

    if( !m_removeList.isEmpty() || !m_addList.isEmpty() )
    {
        m_addRemoveResult =
                m_apiRequest->addRemoveSubscriptions( m_username, m_deviceName, m_addList, m_removeList );

        //Only clear m_addList and m_removeList if the synchronization with gpodder.net really worked
        connect( m_addRemoveResult.data(), SIGNAL(finished()), this,
                 SLOT(slotSuccessfulSubscriptionSynchronisation()) );

        Amarok::Logger::shortMessage( i18n( "Trying to synchronize subscriptions with gpodder.net" ) );
    }
    else
        m_timerSynchronizeSubscriptions->stop();
}

void
GpodderProvider::slotSuccessfulSubscriptionSynchronisation()
{
    DEBUG_BLOCK

    m_timestampSubscription = QDateTime::currentMSecsSinceEpoch();
    setSubscriptionTimestamp( m_timestampSubscription );

    m_addList.clear();
    m_removeList.clear();

    //In addition, the server MUST send any URLs that have been rewritten (sanitized, see bug:747)
    //as a list of tuples with the key "update_urls". The client SHOULD parse this list and update
    //the local subscription list accordingly (the server only sanitizes the URL, so the semantic
    //"content" should stay the same and therefore the client can simply update the URL value
    //locally and use it for future updates
    updateLocalPodcasts( m_addRemoveResult->updateUrlsList() );
}

void
GpodderProvider::slotTrackChanged( Meta::TrackPtr track )
{
    m_trackToSyncStatus = nullptr;

    if( track != Meta::TrackPtr( nullptr ) )
    {
        //If the episode is from one of the gpodder subscribed podcasts, then we must keep looking it
        if( ( this->possiblyContainsTrack( QUrl( track->uidUrl() ) ) ) )
        {
            m_trackToSyncStatus = track;

            QTimer::singleShot( 10000, this, SLOT(timerPrepareToSyncPodcastStatus()) );

            //A bookmark will be created if we have a play status available,
            //for current track, at m_episodeStatusMap
            createPlayStatusBookmark();

            return;
        }
    }

    m_timerGeneratePlayAction->stop();
    //EpisodeActions should be sent when the user clicks
    //stops and doesn't resume listening in e.g. 1 minute
    //Or when the user is not listening a podcast in e.g. 1 minute
    m_timerSynchronizeStatus->start( 60000 );
}

void
GpodderProvider::slotTrackPositionChanged( qint64 position, bool userSeek )
{
    Q_UNUSED( position )

    //If the current track is in one of the subscribed gpodder channels and it's position
    //is not at the beginning of the track, then we probably should sync it status.
    if( m_trackToSyncStatus )
    {
        if( userSeek )
        {
            //Test if this track still playing after 10 seconds to avoid accidentally user changes
            QTimer::singleShot( 10000, this, SLOT(timerPrepareToSyncPodcastStatus()) );
        }
    }
}

void GpodderProvider::slotPaused()
{
    m_timerGeneratePlayAction->stop();
    //EpisodeActions should be sent when the user clicks pause
    //or stop and doesn't resume listening in e.g. 1 minute
    m_timerSynchronizeStatus->start( 60000 );
}

void
GpodderProvider::timerSynchronizeSubscriptions()
{
    synchronizeSubscriptions();
}

void
GpodderProvider::timerSynchronizeStatus()
{
    synchronizeStatus();
}

void
GpodderProvider::timerPrepareToSyncPodcastStatus()
{
    if( The::engineController()->currentTrack() == m_trackToSyncStatus )
    {
        EpisodeActionPtr tempEpisodeAction;
        PodcastEpisodePtr tempEpisode = PodcastEpisodePtr::dynamicCast( m_trackToSyncStatus );

        if( tempEpisode )
        {
            qulonglong positionSeconds = The::engineController()->trackPosition();
            qulonglong lengthSeconds = The::engineController()->trackLength() / 1000;

            QString podcastUrl = resolvedPodcastUrl( tempEpisode ).url();

            tempEpisodeAction = EpisodeActionPtr(
                                    new EpisodeAction( QUrl( podcastUrl ),
                                                       QUrl( tempEpisode->uidUrl() ),
                                                       m_deviceName,
                                                       EpisodeAction::Play,
                                                       QDateTime::currentMSecsSinceEpoch(),
                                                       1,
                                                       positionSeconds + 1,
                                                       lengthSeconds
                                                       ) );

            //Any previous episodeAction, from the same podcast, will be replaced
            m_uploadEpisodeStatusMap.insert( QUrl( tempEpisode->uidUrl() ), tempEpisodeAction );
        }

        //Starts to generate EpisodeActions
        m_timerGeneratePlayAction->start( 30000 );
    }
}

void GpodderProvider::timerGenerateEpisodeAction()
{
    //Create and update episode actions
    if( The::engineController()->currentTrack() == m_trackToSyncStatus )
    {
        EpisodeActionPtr tempEpisodeAction;
        PodcastEpisodePtr tempEpisode = PodcastEpisodePtr::dynamicCast( m_trackToSyncStatus );

        if( tempEpisode )
        {
            qulonglong positionSeconds = The::engineController()->trackPosition();
            qulonglong lengthSeconds = The::engineController()->trackLength() / 1000;

            QString podcastUrl = resolvedPodcastUrl( tempEpisode ).url();

            tempEpisodeAction = EpisodeActionPtr(
                                    new EpisodeAction( QUrl( podcastUrl ),
                                                       QUrl( tempEpisode->uidUrl() ),
                                                       m_deviceName,
                                                       EpisodeAction::Play,
                                                       QDateTime::currentMSecsSinceEpoch(),
                                                       1,
                                                       positionSeconds + 1,
                                                       lengthSeconds
                                                       ) );

            //Any previous episodeAction, from the same podcast, will be replaced
            m_uploadEpisodeStatusMap.insert( QUrl( tempEpisode->uidUrl() ), tempEpisodeAction );
            //Make local podcasts aware of new episodeActions
            m_episodeStatusMap.insert( QUrl( tempEpisode->uidUrl() ), tempEpisodeAction );
        }
    }
}

void
GpodderProvider::requestDeviceUpdates()
{
    DEBUG_BLOCK

    if( !QNetworkConfigurationManager().isOnline() )
    {
        QTimer::singleShot( 10000, this, SLOT(requestDeviceUpdates()) );
        return;
    }

    m_deviceUpdatesResult =
            m_apiRequest->deviceUpdates( m_username,
                                         m_deviceName,
                                         0 );

    connect( m_deviceUpdatesResult.data(), SIGNAL(finished()),
             SLOT(deviceUpdatesFinished()) );
    connect( m_deviceUpdatesResult.data(),
             SIGNAL(requestError(QNetworkReply::NetworkError)),
             SLOT(deviceUpdatesRequestError(QNetworkReply::NetworkError)) );
    connect( m_deviceUpdatesResult.data(), SIGNAL(parseError()),
             SLOT(deviceUpdatesParseError()) );
}

void
GpodderProvider::deviceUpdatesFinished()
{
    DEBUG_BLOCK

    debug() << "DeviceUpdate timestamp: " << m_deviceUpdatesResult->timestamp();

    //Channels to subscribe locally
    foreach( mygpo::PodcastPtr podcast, m_deviceUpdatesResult->addList() )
    {
        debug() << "Subscribing GPO channel: " << podcast->title() << ": " << podcast->url();

        GpodderPodcastChannelPtr channel =
                GpodderPodcastChannelPtr( new GpodderPodcastChannel( this, podcast ) );

        //First we need to resolve redirection url's if there is any
        requestUrlResolve( channel );
    }

    //Request the last episode status for every episode in gpodder.net
    //subscribed podcasts
    QTimer::singleShot( 1000, this, SLOT(requestEpisodeActionsInCascade()) );

    //Only after all subscription changes are committed should we save the timestamp
    m_timestampSubscription = m_deviceUpdatesResult->timestamp();
    setSubscriptionTimestamp( m_timestampSubscription );
}

void
GpodderProvider::continueDeviceUpdatesFinished()
{
    foreach( GpodderPodcastChannelPtr channel, m_resolvedChannelsToBeAdded )
    {
        m_channelsToRequestActions.enqueue( channel->url() );

        PodcastChannelPtr master;
        PodcastChannelPtr slave;

        slave = this->addChannel( PodcastChannelPtr::dynamicCast( channel ) );

        foreach( PodcastChannelPtr tempChannel, The::playlistManager()->defaultPodcasts()->channels() )
            if( tempChannel->url() == channel->url() )
                master = tempChannel;

        if( !master )
            master =  The::playlistManager()->defaultPodcasts()->addChannel( slave );

        //Create a playlist synchronization between master and slave
        The::playlistManager()->setupSync( Playlists::PlaylistPtr::dynamicCast( master ),
                                           Playlists::PlaylistPtr::dynamicCast( slave )
                                           );
    }

    m_resolvedChannelsToBeAdded.clear();
}

void
GpodderProvider::deviceUpdatesParseError()
{
    DEBUG_BLOCK

    QTimer::singleShot( 10000, this, SLOT(requestDeviceUpdates()) );

    debug() << "deviceUpdates [Subscription Synchronization] - Parse error";
    Amarok::Logger::shortMessage( i18n( "GPodder Service failed to get data from the server. Will retry in 10 seconds..." ) );
}

void
GpodderProvider::deviceUpdatesRequestError( QNetworkReply::NetworkError error )
{
    DEBUG_BLOCK

    QTimer::singleShot( 10000, this, SLOT(requestDeviceUpdates()) );

    debug() << "deviceUpdates [Subscription Synchronization] - Request error nr.: " << error;
    Amarok::Logger::shortMessage( i18n( "GPodder Service failed to get data from the server. Will retry in 10 seconds..." ) );
}

void
GpodderProvider::requestEpisodeActionsInCascade()
{
    DEBUG_BLOCK

    if( !QNetworkConfigurationManager().isOnline() )
    {
        QTimer::singleShot( 10000, this, SLOT(requestEpisodeActionsInCascade()) );
        return;
    }

    //This function will download all episode actions for
    //every podcast contained in m_channelsToRequestActions
    if( !m_channelsToRequestActions.isEmpty() )
    {
        QUrl url = m_channelsToRequestActions.head();
        m_episodeActionListResult = m_apiRequest->episodeActionsByPodcast( m_username, url.toString(), true );
        debug() << "Requesting actions for " << url.toString();
        connect( m_episodeActionListResult.data(), SIGNAL(finished()),
                 SLOT(episodeActionsInCascadeFinished()) );
        connect( m_episodeActionListResult.data(),
                 SIGNAL(requestError(QNetworkReply::NetworkError)),
                 SLOT(episodeActionsInCascadeRequestError(QNetworkReply::NetworkError)) );
        connect( m_episodeActionListResult.data(), SIGNAL(parseError()),
                 SLOT(episodeActionsInCascadeParseError()) );
    }
    else
    {
        //We should try to upload cached EpisodeActions to gpodder.net
        synchronizeStatus();
    }
}

void
GpodderProvider::episodeActionsInCascadeFinished()
{
    DEBUG_BLOCK

    m_timestampStatus = m_episodeActionListResult->timestamp();

    foreach( EpisodeActionPtr tempEpisodeAction, m_episodeActionListResult->list() )
    {
        if( tempEpisodeAction->action() == EpisodeAction::Play )
        {
            debug() << QString( "Adding a new play status to episode: %1" )
                       .arg( tempEpisodeAction->episodeUrl().toString() );

            m_episodeStatusMap.insert( tempEpisodeAction->episodeUrl(), tempEpisodeAction );

            //A bookmark will be created if we have a play status available,
            //for current track, at m_episodeStatusMap
            createPlayStatusBookmark();
        }
        else
        {
            PodcastChannelPtr channel;
            PodcastEpisodePtr episode;

            foreach( PodcastChannelPtr tempChannel, m_channels )
                if( tempChannel->url() == tempEpisodeAction->podcastUrl() )
                {
                    channel = tempChannel;

                    foreach( PodcastEpisodePtr tempEpisode, channel->episodes() )
                        if( tempEpisode->uidUrl() == tempEpisodeAction->episodeUrl().toString() )
                            episode = tempEpisode;
                }

            if( channel && episode )
            {
                if( tempEpisodeAction->action() == EpisodeAction::New )
                {
                    if( !episode )
                    {
                        debug() << QString( "New episode to be added found: %1" )
                                   .arg( tempEpisodeAction->episodeUrl().toString() );

                        PodcastEpisodePtr tempEpisode;
                        tempEpisode = PodcastEpisodePtr( new PodcastEpisode() );
                        tempEpisode->setUidUrl( tempEpisodeAction->episodeUrl() );
                        tempEpisode->setChannel( PodcastChannelPtr::dynamicCast( channel ) );

                        channel->addEpisode( tempEpisode );
                    }
                    else
                    {
                        debug() << QString( "Marking an existent episode as new: %1" )
                                   .arg( tempEpisodeAction->episodeUrl().toString() );

                        episode->setNew( true );
                    }
                }
                else if( tempEpisodeAction->action() == EpisodeAction::Download )
                {
                    debug() << QString( "Adding a new download status to episode: %1" )
                               .arg( tempEpisodeAction->episodeUrl().toString() );

                }
                else if( tempEpisodeAction->action() == EpisodeAction::Delete )
                {
                    debug() << QString( "Adding a new delete status to episode: %1" )
                               .arg( tempEpisodeAction->episodeUrl().toString() );

                }

                m_episodeStatusMap.insert( tempEpisodeAction->episodeUrl(), tempEpisodeAction );
            }
            else
            {
                //For some reason the podcast and/or episode for this action
                //wasn't found
                debug() << QString( "Episode and/or channel not found" );
            }

        }
    }

    //We must remove this podcast url and continue with the others
    m_channelsToRequestActions.dequeue();

    QTimer::singleShot( 100, this, SLOT(requestEpisodeActionsInCascade()) );
}

void
GpodderProvider::episodeActionsInCascadeParseError()
{
    DEBUG_BLOCK

    QTimer::singleShot( 10000, this, SLOT(requestEpisodeActionsInCascade()) );
    //If we fail to get EpisodeActions for this channel then we must put it
    //at the end of the list. In order to be synced later on.
    m_channelsToRequestActions.enqueue( m_channelsToRequestActions.dequeue() );

    debug() << "episodeActionsInCascade [Status Synchronization] - Parse Error";
}

void
GpodderProvider::episodeActionsInCascadeRequestError( QNetworkReply::NetworkError error )
{
    DEBUG_BLOCK

    QTimer::singleShot( 10000, this, SLOT(requestEpisodeActionsInCascade()) );
    //If we fail to get EpisodeActions for this channel then we must put it
    //at the end of the list. In order to be synced later on.
    m_channelsToRequestActions.enqueue( m_channelsToRequestActions.dequeue() );

    debug() << "episodeActionsInCascade [Status Synchronization] - Request error nr.: " << error;
}

void
GpodderProvider::updateLocalPodcasts( const QList<QPair<QUrl,QUrl> > updatedUrls )
{
    QList< QPair<QUrl,QUrl> >::const_iterator tempUpdatedUrl = updatedUrls.begin();

    for(; tempUpdatedUrl != updatedUrls.end(); ++tempUpdatedUrl )
    {
        foreach( PodcastChannelPtr tempChannel, The::playlistManager()->defaultPodcasts()->channels() )
        {
            if( tempChannel->url() == (*tempUpdatedUrl).first )
                tempChannel->setUrl( (*tempUpdatedUrl).second );
        }

        foreach( PodcastChannelPtr tempGpodderChannel, m_channels )
        {
            if( tempGpodderChannel->url() == (*tempUpdatedUrl).first )
                tempGpodderChannel->setUrl( (*tempUpdatedUrl).second );
        }
    }
}

void
GpodderProvider::createPlayStatusBookmark()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( track )
    {
        EpisodeActionPtr tempEpisodeAction = m_episodeStatusMap.value( QUrl( track->uidUrl() ) );

        //Create an AutoTimecode at the last position position, so the user always know where he stopped to listen
        if( tempEpisodeAction && ( tempEpisodeAction->action() == EpisodeAction::Play ) )
        {
            if( track && track->has<Capabilities::TimecodeWriteCapability>() )
            {
                QScopedPointer<Capabilities::TimecodeWriteCapability> tcw( track->create<Capabilities::TimecodeWriteCapability>() );
                qint64 positionMiliSeconds = tempEpisodeAction->position() * 1000;

                tcw->writeAutoTimecode( positionMiliSeconds );
            }
        }
    }
}

void
GpodderProvider::requestUrlResolve( Podcasts::GpodderPodcastChannelPtr channel )
{
    if( !channel )
        return;

    m_resolveUrlJob = KIO::get( channel->url(), KIO::Reload, KIO::HideProgressInfo );

    connect( m_resolveUrlJob, &KJob::result,
             this, &GpodderProvider::urlResolveFinished );
    connect( m_resolveUrlJob,
             &KIO::TransferJob::permanentRedirection,
             this, &GpodderProvider::urlResolvePermanentRedirection );

    m_resolvedPodcasts.insert( m_resolveUrlJob, channel );
}

void
GpodderProvider::urlResolvePermanentRedirection( KIO::Job *job, const QUrl &fromUrl, const QUrl &toUrl )
{
    DEBUG_BLOCK

    KIO::TransferJob *transferJob = dynamic_cast<KIO::TransferJob *>( job );
    GpodderPodcastChannelPtr channel = m_resolvedPodcasts.value( transferJob );

    m_redirectionUrlMap.insert( toUrl, channel->url() );

    channel->setUrl( toUrl );

    debug() << fromUrl.url() << " was redirected to " << toUrl.url();

    requestUrlResolve( channel );
}

void
GpodderProvider::urlResolveFinished( KJob * job )
{
    KIO::TransferJob *transferJob = dynamic_cast<KIO::TransferJob *>( job );

    if( transferJob && ( !( transferJob->isErrorPage() || job->error() ) ) )
    {
        m_resolvedChannelsToBeAdded.push_back( m_resolvedPodcasts.value( transferJob ) );
        m_resolvedPodcasts.remove( transferJob );
    }
    else
        requestUrlResolve( m_resolvedPodcasts.value( transferJob ) );

    if( m_resolvedPodcasts.empty() )
        continueDeviceUpdatesFinished();

    m_resolveUrlJob = nullptr;
}

void GpodderProvider::slotEpisodeDownloaded( PodcastEpisodePtr episode )
{
    EpisodeActionPtr tempEpisodeAction;

    QString podcastUrl = resolvedPodcastUrl( episode ).url();

    tempEpisodeAction = EpisodeActionPtr(
                            new EpisodeAction( QUrl( podcastUrl ),
                                               QUrl( episode->uidUrl() ),
                                               m_deviceName,
                                               EpisodeAction::Download,
                                               QDateTime::currentMSecsSinceEpoch(),
                                               0,
                                               0,
                                               0
                                               ) );

    //Any previous episodeAction, from the same podcast, will be replaced
    m_uploadEpisodeStatusMap.insert( QUrl( episode->uidUrl() ), tempEpisodeAction );

    m_timerSynchronizeStatus->start( 60000 );
}

void GpodderProvider::slotEpisodeDeleted( PodcastEpisodePtr episode )
{
    EpisodeActionPtr tempEpisodeAction;

    QString podcastUrl = resolvedPodcastUrl( episode ).url();

    tempEpisodeAction = EpisodeActionPtr(
                            new EpisodeAction( QUrl( podcastUrl ),
                                               QUrl( episode->uidUrl() ),
                                               m_deviceName,
                                               EpisodeAction::Delete,
                                               QDateTime::currentMSecsSinceEpoch(),
                                               0,
                                               0,
                                               0
                                               ) );

    //Any previous episodeAction, from the same podcast, will be replaced
    m_uploadEpisodeStatusMap.insert( QUrl( episode->uidUrl() ), tempEpisodeAction );

    m_timerSynchronizeStatus->start( 60000 );
}

void GpodderProvider::slotEpisodeMarkedAsNew( PodcastEpisodePtr episode )
{
    EpisodeActionPtr tempEpisodeAction;

    QString podcastUrl = resolvedPodcastUrl( episode ).url();

    tempEpisodeAction = EpisodeActionPtr(
                            new EpisodeAction( QUrl( podcastUrl ),
                                               QUrl( episode->uidUrl() ),
                                               m_deviceName,
                                               EpisodeAction::New,
                                               QDateTime::currentMSecsSinceEpoch(),
                                               0,
                                               0,
                                               0
                                               ) );

    //Any previous episodeAction, from the same podcast, will be replaced
    m_uploadEpisodeStatusMap.insert( QUrl( episode->uidUrl() ), tempEpisodeAction );

    m_timerSynchronizeStatus->start( 60000 );
}

inline KConfigGroup
GpodderProvider::gpodderActionsConfig() const
{
    return Amarok::config( "GPodder Cached Episode Actions" );
}

void GpodderProvider::loadCachedEpisodeActions()
{
    DEBUG_BLOCK

    if( !gpodderActionsConfig().exists() )
        return;

    int action;
    bool validActionType;
    bool actionTypeConversion;
    qulonglong timestamp = 0;
    qulonglong started = 0;
    qulonglong position = 0;
    qulonglong total = 0;
    QStringList actionsDetails;
    EpisodeAction::ActionType actionType;

    foreach( QString episodeUrl, gpodderActionsConfig().keyList() )
    {
        actionsDetails.clear();
        actionsDetails = gpodderActionsConfig().readEntry( episodeUrl ).split( ',' );

        if( actionsDetails.count() != 6 )
            debug() << "There are less/more fields than expected.";
        else
        {
            action = actionsDetails[1].toInt( &actionTypeConversion );

            if( !actionTypeConversion )
                debug() << "Failed to convert actionType field to int.";
            else
            {
                validActionType = true;
                timestamp = actionsDetails[2].toULongLong();
                started = actionsDetails[3].toULongLong();
                position = actionsDetails[4].toULongLong();
                total = actionsDetails[5].toULongLong();

                switch( action )
                {
                    case 0: actionType = EpisodeAction::Download; break;
                    case 1: actionType = EpisodeAction::Play; break;
                    case 2: actionType = EpisodeAction::Delete; break;
                    case 3: actionType = EpisodeAction::New; break;
                    default: validActionType = false; break;
                }

                //We can't create a EpisodeAction if action isn't a valid alternative
                if( !validActionType )
                    debug() << "Action isn't a valid alternative.";
                else
                {
                    debug() << QString( "Loaded %1 action." ).arg( episodeUrl );

                    EpisodeActionPtr tempEpisodeAction = EpisodeActionPtr(
                                new EpisodeAction( QUrl( actionsDetails[0] ),
                                                   QUrl( episodeUrl ),
                                                   m_deviceName,
                                                   actionType,
                                                   timestamp,
                                                   started,
                                                   position,
                                                   total
                                                   ) );

                    //Any previous episodeAction, from the same podcast, will be replaced
                    m_uploadEpisodeStatusMap.insert( tempEpisodeAction->episodeUrl(), tempEpisodeAction );
                    m_episodeStatusMap.insert( tempEpisodeAction->episodeUrl(), tempEpisodeAction );
                }
            }
        }
    }

    //We should delete cached EpisodeActions, since we already loaded them
    gpodderActionsConfig().deleteGroup();

    synchronizeStatus();
}

void GpodderProvider::saveCachedEpisodeActions()
{
    DEBUG_BLOCK

    if( m_uploadEpisodeStatusMap.isEmpty() )
        return;

    int actionType;
    QList<QString> actionsDetails;

    foreach( EpisodeActionPtr action, m_uploadEpisodeStatusMap.values() )
    {
        actionsDetails.clear();
        actionsDetails.append( action->podcastUrl().toString() );

        switch( action->action() )
        {
            case EpisodeAction::Download: actionType = 0; break;
            case EpisodeAction::Play: actionType = 1; break;
            case EpisodeAction::Delete: actionType = 2; break;
            case EpisodeAction::New: actionType = 3; break;
            default: actionType = -1; break;
        }

        actionsDetails.append( QString::number( actionType ) );
        actionsDetails.append( QString::number( action->timestamp() ) );
        actionsDetails.append( QString::number( action->started() ) );
        actionsDetails.append( QString::number( action->position() ) );
        actionsDetails.append( QString::number( action->total() ) );

        gpodderActionsConfig().writeEntry( action->episodeUrl().toString(), actionsDetails );
    }
}

inline KConfigGroup
GpodderProvider::gpodderPodcastsConfig() const
{
    return Amarok::config( "GPodder Cached Podcast Changes" );
}

void GpodderProvider::loadCachedPodcastsChanges()
{
    DEBUG_BLOCK

    if( !gpodderPodcastsConfig().exists() )
        return;

    QStringList podcastsUrlsToAdd;
    QStringList podcastsUrlsToRemove;

    podcastsUrlsToAdd = gpodderPodcastsConfig().readEntry( "addList" ).split( ',' );
    podcastsUrlsToRemove = gpodderPodcastsConfig().readEntry( "removeList" ).split( ',' );

    foreach( QString podcastUrl, podcastsUrlsToAdd )
    {
        debug() << QString( "New channel to subscribe: %1" ).arg( podcastUrl );

        m_addList.append( QUrl( podcastUrl ) );
    }

    foreach( QString podcastUrl, podcastsUrlsToRemove )
    {
        debug() << QString( "New channel to unsubscribe: %1 action." ).arg( podcastUrl );

        m_removeList.append( QUrl( podcastUrl ) );
    }

    //We should delete cached podcasts changes, since we already loaded them
    gpodderPodcastsConfig().deleteGroup();

    synchronizeSubscriptions();
}

void GpodderProvider::saveCachedPodcastsChanges()
{
    DEBUG_BLOCK

    if( !m_addList.isEmpty() )
    {
        QStringList podcastUrlsToAdd;

        foreach( QUrl podcastUrl, m_addList )
            podcastUrlsToAdd.append( podcastUrl.toString() );

        gpodderPodcastsConfig().writeEntry( "addList", podcastUrlsToAdd );
    }

    if( !m_removeList.isEmpty() )
    {
        QStringList podcastsUrlsToRemove;

        foreach( QUrl podcastUrl, m_removeList )
            podcastsUrlsToRemove.append( podcastUrl.toString() );

        gpodderPodcastsConfig().writeEntry( "removeList", podcastsUrlsToRemove );
    }
}

QUrl GpodderProvider::resolvedPodcastUrl( const PodcastEpisodePtr episode )
{
    QUrl podcastUrl = episode->channel()->url();

    if( m_redirectionUrlMap.contains( podcastUrl ) )
        podcastUrl = m_redirectionUrlMap.value( podcastUrl );

    return podcastUrl;
}
