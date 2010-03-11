/****************************************************************************************
 * Copyright (c) 2009 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#include "CoverFetchQueue.h"
#include "Debug.h"

#define DEBUG_PREFIX "CoverFetchQueue"

CoverFetchQueue::CoverFetchQueue( QObject *parent )
    : QObject( parent )
{
}

CoverFetchQueue::~CoverFetchQueue()
{
}

void
CoverFetchQueue::add( const CoverFetchUnit::Ptr unit )
{
    DEBUG_BLOCK
    m_queue.append( unit );
    emit fetchUnitAdded( unit );
}

void
CoverFetchQueue::add( const Meta::AlbumPtr album, const CoverFetch::Options opt, const QByteArray &xml )
{
    CoverFetchPayload *payload;
    if( xml.isEmpty() )
    {
        payload = new CoverFetchInfoPayload( album );
    }
    else
    {
        CoverFetchArtPayload *art = new CoverFetchArtPayload( album );
        const bool wild = ( opt == CoverFetch::WildInteractive ) ? true : false;
        art->setWildMode( wild );
        art->setSize( CoverFetch::NormalSize );
        art->setXml( xml );
        payload = art;
    }
    add( KSharedPtr< CoverFetchUnit >( new CoverFetchUnit( album, payload, opt ) ) );
}

void
CoverFetchQueue::add( const CoverFetch::Options opt, const QByteArray &xml )
{
    CoverFetchArtPayload *art = new CoverFetchArtPayload();
    const bool wild = ( opt == CoverFetch::WildInteractive ) ? true : false;
    art->setWildMode( wild );
    art->setXml( xml );
    add( KSharedPtr< CoverFetchUnit >( new CoverFetchUnit( Meta::AlbumPtr( 0 ), art, opt ) ) );
}

void
CoverFetchQueue::addQuery( const QString &query )
{
    CoverFetchSearchPayload *payload = new CoverFetchSearchPayload( query );
    add( KSharedPtr< CoverFetchUnit >( new CoverFetchUnit( payload ) ) );
}

int
CoverFetchQueue::size() const
{
    return m_queue.size();
}

bool
CoverFetchQueue::isEmpty() const
{
    return m_queue.isEmpty();
}

void
CoverFetchQueue::clear()
{
    m_queue.clear();
}

void
CoverFetchQueue::remove( const CoverFetchUnit::Ptr unit )
{
    m_queue.removeAll( unit );
}

void
CoverFetchQueue::remove( const Meta::AlbumPtr album )
{
    m_queue.removeAt( index( album ) );
}

bool
CoverFetchQueue::contains( const Meta::AlbumPtr album ) const
{
    typedef CoverFetchUnitList::const_iterator ListIter;
    ListIter it   = m_queue.constBegin();
    ListIter last = m_queue.constEnd();
    while( it != last )
    {
        Meta::AlbumPtr t_album = (*it)->album();
        if( t_album == album )
            return true;
        ++it;
    }
    return false;
}

int
CoverFetchQueue::index( const Meta::AlbumPtr album ) const
{
    for( int i = 0, len = m_queue.size(); i < len; ++i )
    {
        if( m_queue.at( i )->album() == album )
            return i;
    }
    return -1;
}

const CoverFetchUnit::Ptr
CoverFetchQueue::take( const Meta::AlbumPtr album )
{
    for( int i = 0, end = this->size(); i < end; ++i )
    {
        const CoverFetchUnit::Ptr unit = m_queue.at( i );
        if( unit->album() == album )
        {
            m_queue.removeAt( i );
            return unit;
        }
    }
    // need to test if album exists with contains() first
    return KSharedPtr< CoverFetchUnit >();
}

#include "CoverFetchQueue.moc"
