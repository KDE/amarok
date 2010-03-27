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

#include "core-implementations/playlists/file/PlaylistFileSupport.h"
#include "UmsPodcastProvider.h"

using namespace Meta;

UmsPodcastEpisodePtr
UmsPodcastEpisode::fromPodcastEpisodePtr( PodcastEpisodePtr episode )
{
    return UmsPodcastEpisodePtr::dynamicCast( episode );
}

PodcastEpisodePtr
UmsPodcastEpisode::toPodcastEpisodePtr( UmsPodcastEpisodePtr episode )
{
    return PodcastEpisodePtr::dynamicCast( episode );
}

PodcastEpisodeList
UmsPodcastEpisode::toPodcastEpisodeList( UmsPodcastEpisodeList episodes )
{
    PodcastEpisodeList list;
    foreach( UmsPodcastEpisodePtr e, episodes )
        list << toPodcastEpisodePtr( e );
    return list;
}

UmsPodcastEpisode::UmsPodcastEpisode( UmsPodcastChannelPtr channel )
        : Meta::PodcastEpisode( UmsPodcastChannel::toPodcastChannelPtr( channel ) )
{
}

UmsPodcastEpisode::~UmsPodcastEpisode()
{
}

void
UmsPodcastEpisode::setLocalUrl( const KUrl &localUrl )
{
    m_localUrl = localUrl;
    //TODO: load local file
}

KUrl
UmsPodcastEpisode::playableUrl() const
{
    if( m_localFile.isNull() )
        return m_url;

    return m_localFile->playableUrl();
}

void
UmsPodcastEpisode::setLocalFile( MetaFile::TrackPtr localFile )
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

bool
UmsPodcastEpisode::isEditable() const
{
     if( m_localFile.isNull() )
         return false;

     return m_localFile->isEditable();
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

AlbumPtr
UmsPodcastEpisode::album() const
{
    if( m_localFile.isNull() )
        return m_albumPtr;

    return m_localFile->album();
}

ArtistPtr
UmsPodcastEpisode::artist() const
{
    if( m_localFile.isNull() )
        return m_artistPtr;

    return m_localFile->artist();
}

ComposerPtr
UmsPodcastEpisode::composer() const
{
    if( m_localFile.isNull() )
        return m_composerPtr;

    return m_localFile->composer();
}

GenrePtr
UmsPodcastEpisode::genre() const
{
    if( m_localFile.isNull() )
        return m_genrePtr;

    return m_localFile->genre();
}

YearPtr
UmsPodcastEpisode::year() const
{
    if( m_localFile.isNull() )
        return m_yearPtr;

    return m_localFile->year();
}

UmsPodcastChannelPtr
UmsPodcastChannel::fromPodcastChannelPtr( PodcastChannelPtr channel )
{
    return UmsPodcastChannelPtr::dynamicCast( channel );
}

PodcastChannelPtr
UmsPodcastChannel::toPodcastChannelPtr( UmsPodcastChannelPtr channel )
{
    return PodcastChannelPtr::dynamicCast( channel );
}

PodcastChannelList
UmsPodcastChannel::toPodcastChannelList( UmsPodcastChannelList umsChannels )
{
    PodcastChannelList channels;
    foreach( UmsPodcastChannelPtr umsChannel, umsChannels )
        channels << UmsPodcastChannel::toPodcastChannelPtr(  umsChannel );
    return channels;
}

UmsPodcastChannel::UmsPodcastChannel( UmsPodcastProvider *provider )
        : Meta::PodcastChannel()
        , m_provider( provider )
{

}

UmsPodcastChannel::UmsPodcastChannel( PodcastChannelPtr channel,
                                      UmsPodcastProvider *provider )
        : Meta::PodcastChannel( channel )
        , m_provider( provider )
{
    foreach( PodcastEpisodePtr episode, channel->episodes() )
        addEpisode( episode );
}

UmsPodcastChannel::~UmsPodcastChannel()
{

}

PodcastEpisodePtr
UmsPodcastChannel::addEpisode( PodcastEpisodePtr episode )
{
    DEBUG_BLOCK

    if( !episode->isNew() || !episode->playableUrl().isLocalFile() )
        return PodcastEpisodePtr(); //we don't care about these.

    if( !m_provider )
        return PodcastEpisodePtr();

    return m_provider->addEpisode( episode );
}

void
UmsPodcastChannel::addUmsEpisode( UmsPodcastEpisodePtr umsEpisode )
{
    int i = 0;
    foreach( UmsPodcastEpisodePtr e, m_umsEpisodes )
    {
        if( umsEpisode->createDate() > e->createDate() )
        {
            i = m_umsEpisodes.indexOf( e );
            break;
        }
    }

    m_umsEpisodes.insert( i, umsEpisode );
}

void
UmsPodcastChannel::setPlaylistFileSource( const KUrl &playlistFilePath )
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
UmsPodcastChannel::removeEpisode( UmsPodcastEpisodePtr episode )
{
    int position = m_umsEpisodes.indexOf( episode );

    if( position == -1 )
    {
        error() << title() << " does't have this episode";
        return;
    }

    m_umsEpisodes.removeAt( position );
    notifyObserversTrackRemoved( position );
}
