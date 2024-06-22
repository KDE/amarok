/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "core/meta/Meta.h"
#include "core/playlists/Playlist.h"
#include "core/playlists/PlaylistProvider.h"

using namespace Playlists;

PlaylistObserver::PlaylistObserver()
{
}

PlaylistObserver::~PlaylistObserver()
{
    QMutexLocker locker( &m_playlistSubscriptionsMutex );
    for( PlaylistPtr playlist : m_playlistSubscriptions )
    {
        playlist->unsubscribe( this );
    }
}

void
PlaylistObserver::subscribeTo( PlaylistPtr playlist )
{
    if( playlist )
    {
        QMutexLocker locker( &m_playlistSubscriptionsMutex );
        m_playlistSubscriptions.insert( playlist );
        playlist->subscribe( this );
    }
}

void
PlaylistObserver::unsubscribeFrom( PlaylistPtr playlist )
{
    if( playlist )
    {
        QMutexLocker locker( &m_playlistSubscriptionsMutex );
        m_playlistSubscriptions.remove( playlist );
        playlist->unsubscribe( this );
    }
}

void PlaylistObserver::metadataChanged( const PlaylistPtr &)
{
}

void PlaylistObserver::trackAdded( const PlaylistPtr&, const Meta::TrackPtr&, int )
{
}

void PlaylistObserver::trackRemoved( const PlaylistPtr&, int )
{
}

void PlaylistObserver::tracksLoaded( PlaylistPtr )
{
}

Playlist::Playlist()
    : m_async( true )
{
}

Playlist::~Playlist()
{
}

void
Playlist::setName( const QString & )
{
}

void
Playlist::triggerTrackLoad()
{
    notifyObserversTracksLoaded();
}

void Playlist::addTrack( const Meta::TrackPtr&, int )
{
}

void Playlist::removeTrack( int )
{
}

void
Playlist::syncTrackStatus( int, const Meta::TrackPtr &)
{
}

QStringList
Playlist::groups()
{
    return QStringList();
}

void
Playlist::setGroups( const QStringList & )
{
}

void
Playlist::notifyObserversMetadataChanged()
{
    QMutexLocker locker( &m_observersMutex );
    for( PlaylistObserver *observer : QSet<PlaylistObserver *>(m_observers) )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( PlaylistPtr( this ) );
    }
}

void
Playlist::notifyObserversTracksLoaded()
{
    QMutexLocker locker( &m_observersMutex );
    for( PlaylistObserver *observer : QSet<PlaylistObserver *>(m_observers) )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->tracksLoaded( PlaylistPtr( this ) );
    }
}

void
Playlist::notifyObserversTrackAdded( const Meta::TrackPtr &track, int position )
{
    Q_ASSERT( position >= 0 ); // notice bug 293295 early
    QMutexLocker locker( &m_observersMutex );
    for( PlaylistObserver *observer : QSet<PlaylistObserver *>(m_observers) )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->trackAdded( PlaylistPtr( this ), track, position );
    }
}

void
Playlist::notifyObserversTrackRemoved( int position )
{
    QMutexLocker locker( &m_observersMutex );
    for( PlaylistObserver *observer : QSet<PlaylistObserver *>(m_observers) )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->trackRemoved( PlaylistPtr( this ), position );
    }
}

void
Playlist::subscribe( PlaylistObserver* observer )
{
    if( observer )
    {
        QMutexLocker locker( &m_observersMutex );
        m_observers.insert( observer );
    }
}

void
Playlist::unsubscribe( PlaylistObserver* observer )
{
    QMutexLocker locker( &m_observersMutex );
    m_observers.remove( observer );
}
