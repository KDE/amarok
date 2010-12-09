/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TrackSet.h"

#include <KRandom>

Dynamic::TrackSet::TrackSet( const QList<QByteArray>& universe)
    : m_bits( universe.size() )
{
    m_bits.fill( true );
}

Dynamic::TrackSet::TrackSet( const QList<QByteArray>& universe,
                             const QList<QByteArray>& uidList )
    : m_bits( universe.size() )
{
    foreach( const QByteArray &t, uidList )
    {
        int i = universe.indexOf( t );
        if( i != -1 )
            m_bits.setBit( i );
    }
}

Dynamic::TrackSet::TrackSet( const QList<QByteArray>& universe,
                              const QSet<QByteArray>& uidSet )
    : m_bits( universe.size() )
{
    foreach( const QByteArray &t, uidSet )
    {
        int i = universe.indexOf( t );
        if( i != -1 )
            m_bits.setBit( i );
    }
}

void
Dynamic::TrackSet::reset()
{
    m_bits.clear();
}


int
Dynamic::TrackSet::trackCount() const
{
    return m_bits.count(true);
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

QByteArray
Dynamic::TrackSet::getRandomTrack( const QList<QByteArray>& universe ) const
{
    Q_ASSERT( universe.size() == m_bits.size() );

    int count = trackCount();
    if( count == 0 )
        return QByteArray();

    // stupid that I have to go through the set like this...
    int trackNr = KRandom::random() % count;
    for( int i = m_bits.size()-1; i>=0; i-- )
    {
        if( m_bits.at(i) )
        {
            if( trackNr )
                trackNr--;
            else
            {
                return universe.at(i);
            }
        }
    }

    return QByteArray();
}



Dynamic::TrackSet&
Dynamic::TrackSet::operator=( const Dynamic::TrackSet& B )
{
    m_bits = B.m_bits;
    return *this;
}



