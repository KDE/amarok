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

#include "UmsPodcastMeta.h"

#include "core/support/Debug.h"

#include "core-impl/playlists/types/file/PlaylistFileSupport.h"

#include "UmsPodcastProvider.h"

using namespace Podcasts;

UmsPodcastEpisodePtr
UmsPodcastEpisode::fromPodcastEpisodePtr( const PodcastEpisodePtr &episode )
{
    return UmsPodcastEpisodePtr::dynamicCast( episode );
}

UmsPodcastEpisodePtr
UmsPodcastEpisode::fromTrackPtr( const Meta::TrackPtr &track )
{
    return UmsPodcastEpisodePtr::dynamicCast( track );
}

PodcastEpisodePtr
UmsPodcastEpisode::toPodcastEpisodePtr( const UmsPodcastEpisodePtr &episode )
{
    return PodcastEpisodePtr::dynamicCast( episode );
}

PodcastEpisodeList
UmsPodcastEpisode::toPodcastEpisodeList( UmsPodcastEpisodeList episodes )
{
    PodcastEpisodeList list;
    for( UmsPodcastEpisodePtr e : episodes )
        list << toPodcastEpisodePtr( e );
    return list;
}

UmsPodcastEpisode::UmsPodcastEpisode( const UmsPodcastChannelPtr &channel )
        : Podcasts::PodcastEpisode( UmsPodcastChannel::toPodcastChannelPtr( channel ) )
{
}

UmsPodcastEpisode::~UmsPodcastEpisode()
{
}

void
UmsPodcastEpisode::setLocalUrl( const QUrl &localUrl )
{
    m_localUrl = localUrl;
    //TODO: load local file
}

QUrl
UmsPodcastEpisode::playableUrl() const
{
    if( m_localFile.isNull() )
        return m_url;

    return m_localFile->playableUrl();
}

QString
UmsPodcastEpisode::notPlayableReason() const
{
    if( m_localFile )
        return m_localFile->notPlayableReason();
    return PodcastEpisode::notPlayableReason();
}

void
UmsPodcastEpisode::setLocalFile( const MetaFile::TrackPtr &localFile )
{
    m_localFile = localFile;
}

QString
UmsPodcastEpisode::title() const
{
    if( m_localFile.isNull() )
        return m_title;

    return m_localFile->name();
}

QDateTime
UmsPodcastEpisode::createDate() const
{
    if( m_localFile )
        return m_localFile->createDate();
    return Meta::Track::createDate();
}

void
UmsPodcastEpisode::setTitle( const QString &title )
{
    if( !m_localFile.isNull() )
    {
        m_localFile->setTitle( title );
    }

    m_title = title;
}

Meta::AlbumPtr
UmsPodcastEpisode::album() const
{
    if( m_localFile.isNull() )
        return m_albumPtr;

    return m_localFile->album();
}

Meta::ArtistPtr
UmsPodcastEpisode::artist() const
{
    if( m_localFile.isNull() )
        return m_artistPtr;

    return m_localFile->artist();
}

Meta::ComposerPtr
UmsPodcastEpisode::composer() const
{
    if( m_localFile.isNull() )
        return m_composerPtr;

    return m_localFile->composer();
}

Meta::GenrePtr
UmsPodcastEpisode::genre() const
{
    if( m_localFile.isNull() )
        return m_genrePtr;

    return m_localFile->genre();
}

Meta::YearPtr
UmsPodcastEpisode::year() const
{
    if( m_localFile.isNull() )
        return m_yearPtr;

    return m_localFile->year();
}

UmsPodcastChannelPtr
UmsPodcastChannel::fromPodcastChannelPtr( const PodcastChannelPtr &channel )
{
    return UmsPodcastChannelPtr::dynamicCast( channel );
}

PodcastChannelPtr
UmsPodcastChannel::toPodcastChannelPtr( const UmsPodcastChannelPtr &channel )
{
    return PodcastChannelPtr::dynamicCast( channel );
}

PodcastChannelList
UmsPodcastChannel::toPodcastChannelList( UmsPodcastChannelList umsChannels )
{
    PodcastChannelList channels;
    for( UmsPodcastChannelPtr umsChannel : umsChannels )
        channels << UmsPodcastChannel::toPodcastChannelPtr(  umsChannel );
    return channels;
}

UmsPodcastChannel::UmsPodcastChannel( UmsPodcastProvider *provider )
        : Podcasts::PodcastChannel()
        , m_provider( provider )
{

}

UmsPodcastChannel::UmsPodcastChannel( PodcastChannelPtr channel,
                                      UmsPodcastProvider *provider )
        : Podcasts::PodcastChannel( channel )
        , m_provider( provider )
{
    //Since we need to copy the tracks, make sure it's loaded.
    //TODO: we might also need to subscribe to get trackAdded() when channel does async loading.
    channel->triggerTrackLoad();

    for( PodcastEpisodePtr episode : channel->episodes() )
        addEpisode( episode );
}

UmsPodcastChannel::~UmsPodcastChannel()
{

}

PodcastEpisodePtr
UmsPodcastChannel::addEpisode(const PodcastEpisodePtr &episode )
{
    DEBUG_BLOCK

    if( !episode->isNew() || !episode->playableUrl().isLocalFile() )
        return PodcastEpisodePtr(); //we don't care about these.

    if( !m_provider )
        return PodcastEpisodePtr();

    return m_provider->addEpisode( episode );
    //track adding is asynchronous, provider will call addUmsEpisode once done.
    //TODO: change this so track can show progress once playlist-inline-progress is implemented
}

void
UmsPodcastChannel::addUmsEpisode( UmsPodcastEpisodePtr umsEpisode )
{
    int i = 0;
    for( UmsPodcastEpisodePtr e : m_umsEpisodes )
    {
        if( umsEpisode->createDate() > e->createDate() )
        {
            i = m_umsEpisodes.indexOf( e );
            break;
        }
    }

    m_umsEpisodes.insert( i, umsEpisode );
    notifyObserversTrackAdded( Meta::TrackPtr::dynamicCast( umsEpisode ), i );
}

void
UmsPodcastChannel::setPlaylistFileSource( const QUrl &playlistFilePath )
{
    m_playlistFilePath = playlistFilePath;
    m_playlistFile = Playlists::loadPlaylistFile( playlistFilePath );

    //now parse the playlist and use it to create out episode list
}

Playlists::PlaylistProvider *
UmsPodcastChannel::provider() const
{
     return dynamic_cast<Playlists::PlaylistProvider *>( m_provider );
}

void
UmsPodcastChannel::removeEpisode( const UmsPodcastEpisodePtr &episode )
{
    int position = m_umsEpisodes.indexOf( episode );

    if( position == -1 )
    {
        error() << title() << " doesn't have this episode";
        return;
    }

    m_umsEpisodes.removeAt( position );
    notifyObserversTrackRemoved( position );
}
