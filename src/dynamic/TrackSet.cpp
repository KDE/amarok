/***************************************************************************
 * copyright         : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "TrackSet.h"
#include "DynamicModel.h"


Dynamic::TrackSet::TrackSet()
    : m_bits( PlaylistBrowserNS::DynamicModel::instance()->universe().size() )
{
}


Dynamic::TrackSet::TrackSet( const Meta::TrackList& tracks )
    : m_bits( PlaylistBrowserNS::DynamicModel::instance()->universe().size() )
{
    addTracks( tracks );
}

Dynamic::TrackSet::TrackSet( const QSet<Meta::TrackPtr>& tracks )
    : m_bits( PlaylistBrowserNS::DynamicModel::instance()->universe().size() )
{
    addTracks( tracks );
}

void
Dynamic::TrackSet::reset()
{
    m_bits.resize( PlaylistBrowserNS::DynamicModel::instance()->universe().size() );
    m_bits.clear();
}


int
Dynamic::TrackSet::size() const
{
    return m_bits.count(true);
}


void
Dynamic::TrackSet::clear()
{
    m_bits.clear();
}

void
Dynamic::TrackSet::setUniverseSet()
{
    m_bits.fill( true );
}


void
Dynamic::TrackSet::setTracks( const Meta::TrackList& tracks )
{
    m_bits.clear();
    addTracks( tracks );
}

void
Dynamic::TrackSet::setTracks( const QSet<Meta::TrackPtr>& tracks )
{
    m_bits.clear();
    addTracks( tracks );
}

void
Dynamic::TrackSet::addTracks( const Meta::TrackList& tracks )
{
    const Meta::TrackList& U =
        PlaylistBrowserNS::DynamicModel::instance()->universe();

    foreach( Meta::TrackPtr t, tracks )
    {
        int i = U.indexOf( t );
        if( i != -1 )
            m_bits.setBit( i );
    }
}

void
Dynamic::TrackSet::addTracks( const QSet<Meta::TrackPtr>& tracks )
{
    const Meta::TrackList& U =
        PlaylistBrowserNS::DynamicModel::instance()->universe();

    foreach( Meta::TrackPtr t, tracks )
    {
        int i = U.indexOf( t );
        if( i != -1 )
            m_bits.setBit( i );
    }
}

Meta::TrackList
Dynamic::TrackSet::trackList() const
{
    const Meta::TrackList& U =
        PlaylistBrowserNS::DynamicModel::instance()->universe();

    Meta::TrackList tracks;

    int count = m_bits.count( true );
    for( int i = 0; count > 0 && i < m_bits.size(); ++i )
    {
        if( m_bits.testBit(i) )
        {
            tracks.append( U[i] );
            count--;
        }
    }

    return tracks;
}

void
Dynamic::TrackSet::unite( const Dynamic::TrackSet& B )
{
    m_bits |= B.m_bits;
}

void
Dynamic::TrackSet::intersect( const Dynamic::TrackSet& B )
{
    m_bits &= B.m_bits;
}

void
Dynamic::TrackSet::subtract( const Dynamic::TrackSet& B )
{
    m_bits |= B.m_bits;
    m_bits ^= B.m_bits;
}

Dynamic::TrackSet&
Dynamic::TrackSet::operator=( const Dynamic::TrackSet& B )
{
    m_bits = B.m_bits;
    return *this;
}



