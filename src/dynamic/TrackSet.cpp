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

Dynamic::TrackCollection::TrackCollection( const QStringList& uids )
{
    m_uids = uids;
    for( int i = 0; i < m_uids.count(); i++ )
        m_ids.insert( m_uids[i], i );
}

int
Dynamic::TrackCollection::count() const
{
    return m_uids.count();
}

Dynamic::TrackSet::TrackSet()
    : m_bits()
    , m_collection( 0 )
{ }

Dynamic::TrackSet::TrackSet( const Dynamic::TrackCollectionPtr collection, bool value )
    : m_bits( collection->count(), value )
    , m_collection( collection )
{}

void
Dynamic::TrackSet::reset( bool value )
{
    m_bits.fill( value );
}

bool
Dynamic::TrackSet::isOutstanding() const
{
    return !m_collection;
}

int
Dynamic::TrackSet::trackCount() const
{
    return m_bits.count(true);
}

bool
Dynamic::TrackSet::isEmpty() const
{
    return m_bits.isEmpty();
}

bool
Dynamic::TrackSet::isFull() const
{
    return m_bits.count(true) == m_bits.count();
}

QString
Dynamic::TrackSet::getRandomTrack() const
{
    if( !m_collection )
        return QString();

    int count = trackCount();
    if( count == 0 )
        return QString();

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
                return m_collection->m_uids.at(i);
            }
        }
    }

    return QString();
}

void
Dynamic::TrackSet::unite( const Dynamic::TrackSet& B )
{
    m_bits |= B.m_bits;
}

void
Dynamic::TrackSet::unite( const QStringList& B )
{
    if( !m_collection )
        return;

    foreach( const QString &str, B )
    {
        if( !m_collection->m_ids.contains( str ) )
            continue;

        int index = m_collection->m_ids.value( str );
        m_bits.setBit( index );
    }
}

void
Dynamic::TrackSet::intersect( const Dynamic::TrackSet& B )
{
    m_bits &= B.m_bits;
}

void
Dynamic::TrackSet::intersect( const QStringList& B )
{
    if( !m_collection )
        return;

    QBitArray bBits( m_bits.count() );
    foreach( const QString &str, B )
    {
        if( !m_collection->m_ids.contains( str ) )
            continue;

        int index = m_collection->m_ids.value( str );
        bBits.setBit( index );
    }

    m_bits &= bBits;
}

void
Dynamic::TrackSet::subtract( const Dynamic::TrackSet& B )
{
    m_bits |= B.m_bits;
    m_bits ^= B.m_bits;
}

void
Dynamic::TrackSet::subtract( const QStringList& B )
{
    if( !m_collection )
        return;

    foreach( const QString &str, B )
    {
        if( !m_collection->m_ids.contains( str ) )
            continue;

        int index = m_collection->m_ids.value( str );
        m_bits.clearBit( index );
    }
}




Dynamic::TrackSet&
Dynamic::TrackSet::operator=( const Dynamic::TrackSet& B )
{
    m_bits = B.m_bits;
    m_collection = B.m_collection;
    return *this;
}



