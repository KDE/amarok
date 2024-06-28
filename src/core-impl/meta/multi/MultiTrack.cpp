/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "MultiTrack.h"

#include "core/meta/Statistics.h"
#include "core-impl/capabilities/multisource/MultiSourceCapabilityImpl.h"

using namespace Meta;

MultiTrack::MultiTrack( Playlists::PlaylistPtr playlist )
    : QObject()
    , Track()
    , m_playlist( playlist )
{
    Q_ASSERT( playlist );
    if( playlist->trackCount() < 0 )
    {
        PlaylistObserver::subscribeTo( playlist );
        playlist->triggerTrackLoad();
    }
    if( !playlist->tracks().isEmpty() )
        setSource( 0 );
}

MultiTrack::~MultiTrack()
{
}

QStringList
Meta::MultiTrack::sources() const
{
    QStringList trackNames;
    for ( TrackPtr track : m_playlist->tracks() )
    {
        trackNames << track->prettyUrl();
    }

    return trackNames;
}

void
MultiTrack::setSource( int source )
{
    QWriteLocker locker( &m_lock );
    setSourceImpl( source );
    locker.unlock();

    notifyObservers();
    Q_EMIT urlChanged( playableUrl() );
}

int
Meta::MultiTrack::current() const
{
    QReadLocker locker( &m_lock );
    return m_playlist->tracks().indexOf( m_currentTrack );
}

QUrl
MultiTrack::nextUrl() const
{
    int index = current() + 1;
    Meta::TrackPtr track = m_playlist->tracks().value( index );
    if( track )
    {
        track->prepareToPlay();
        return track->playableUrl();
    }
    return QUrl();
}

bool
MultiTrack::hasCapabilityInterface(Capabilities::Capability::Type type) const
{
    return type == Capabilities::Capability::MultiSource;
}

Capabilities::Capability *
MultiTrack::createCapabilityInterface(Capabilities::Capability::Type type)
{
    switch( type )
    {
        case Capabilities::Capability::MultiSource:
            return new Capabilities::MultiSourceCapabilityImpl( this );
        default:
            return nullptr;
    }
}

void
MultiTrack::prepareToPlay()
{
    QReadLocker locker( &m_lock );
    if( m_currentTrack )
        m_currentTrack->prepareToPlay();
}

Meta::StatisticsPtr
Meta::MultiTrack::statistics()
{
    QReadLocker locker( &m_lock );
    return m_currentTrack ? m_currentTrack->statistics() : Track::statistics();
}

void
Meta::MultiTrack::metadataChanged(const TrackPtr &track )
{
    Q_UNUSED( track )
    // forward changes from active tracks
    notifyObservers();
}

void
MultiTrack::trackAdded(const Playlists::PlaylistPtr &, const TrackPtr &, int )
{
    PlaylistObserver::unsubscribeFrom( m_playlist );

    QWriteLocker locker( &m_lock );
    if( !m_currentTrack )
    {
        setSourceImpl( 0 );
        locker.unlock();

        notifyObservers();
        Q_EMIT urlChanged( playableUrl() );
    }
}

void
MultiTrack::setSourceImpl( int source )
{
    if( source < 0 || source >= m_playlist->tracks().count() )
        return;

    if( m_currentTrack )
        Observer::unsubscribeFrom( m_currentTrack );

    m_currentTrack = m_playlist->tracks().at( source );
    Observer::subscribeTo( m_currentTrack );
}
