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
#include "core/support/Debug.h"
#include "core-impl/capabilities/multisource/MultiSourceCapabilityImpl.h"

using namespace Meta;

MultiTrack::MultiTrack( Playlists::PlaylistPtr playlist )
    : QObject()
    , Track()
    , m_playlist( playlist )
    , m_index( 0 )
{
    DEBUG_BLOCK
    debug() << "playlist size: " << m_playlist->tracks().count();
    setSource( 0 );
}


MultiTrack::~MultiTrack()
{
}

QStringList
Meta::MultiTrack::sources() const
{
    QStringList trackNames;
    foreach ( TrackPtr track, m_playlist->tracks() )
    {
        trackNames << track->prettyUrl();
    }

    return trackNames;
}

void
MultiTrack::setSource( int source )
{
    if( source < 0 || source >= m_playlist->tracks().count() )
        return;

    if( m_currentTrack )
        unsubscribeFrom( m_currentTrack );

    m_index = source;
    m_currentTrack = m_playlist->tracks().at( m_index );
    subscribeTo( m_currentTrack );

    notifyObservers();
    emit urlChanged( playableUrl() );
}

int
Meta::MultiTrack::current() const
{
    return m_index;
}

KUrl
MultiTrack::nextUrl() const
{
    int index = m_index + 1;
    Meta::TrackPtr track = m_playlist->tracks().value( index );
    if( track )
    {
        track->prepareToPlay();
        return track->playableUrl();
    }
    return KUrl();
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
            return 0;
    }
}

Meta::StatisticsPtr Meta::MultiTrack::statistics()
{
    return m_currentTrack->statistics();
}

void
Meta::MultiTrack::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    //forward changes from active tracks
    notifyObservers();
}
