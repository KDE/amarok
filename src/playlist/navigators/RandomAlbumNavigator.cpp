/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::RandomAlbumNavigator"

#include "RandomAlbumNavigator.h"

#include "Debug.h"
#include "Meta.h"
#include "playlist/PlaylistModelStack.h"

#include <algorithm> // STL

Playlist::RandomAlbumNavigator::RandomAlbumNavigator()
{
    m_model = Playlist::ModelStack::instance()->top();
    connect( model(), SIGNAL( insertedIds( const QList<quint64>& ) ),
             this, SLOT( recvInsertedIds( const QList<quint64>& ) ) );
    connect( model(), SIGNAL( removedIds( const QList<quint64>& ) ),
             this, SLOT( recvRemovedIds( const QList<quint64>& ) ) );
    connect( model(), SIGNAL( activeTrackChanged( const quint64 ) ),
             this, SLOT( recvActiveTrackChanged( const quint64 ) ) );

    reset();

    //dump();
}

void
Playlist::RandomAlbumNavigator::recvInsertedIds( const QList<quint64>& list )
{
    Meta::AlbumList modifiedAlbums;
    foreach( quint64 id, list )
    {
        Meta::AlbumPtr album = m_model->trackForId( id )->album();
        if ( !m_albumGroups.contains( album ) )
        {
            // TODO: handle already played albums
            m_unplayedAlbums.append( album );
        }
        if ( !modifiedAlbums.contains( album ) )
            modifiedAlbums.append( album );
        m_albumGroups[album].append( id ); // conveniently creates an empty list if none exists
    }

    sortTheseAlbums( modifiedAlbums );
    std::random_shuffle( m_unplayedAlbums.begin(), m_unplayedAlbums.end() );

    if ( m_currentAlbum == Meta::AlbumPtr() && !m_unplayedAlbums.isEmpty() )
    {
        m_currentAlbum = m_unplayedAlbums.takeFirst();
        m_currentTrack = 0;
    }

    //dump();
}

void
Playlist::RandomAlbumNavigator::recvRemovedIds( const QList<quint64>& list )
{
    QList<quint64>::const_iterator id_iter;
    for ( id_iter = list.begin(); id_iter != list.end(); ++id_iter ) {
        quint64 id = *id_iter;
        debug() << "removing" << id;
        QHash<Meta::AlbumPtr, ItemList>::iterator alb_iter = m_albumGroups.begin();
        while ( alb_iter != m_albumGroups.end() ) {
            if ( alb_iter->contains( id ) ) {
                debug() << "    from" << alb_iter.key()->prettyName();
                Meta::AlbumPtr album = alb_iter.key();
                ItemList atl = alb_iter.value();
                if ( m_currentTrack == id ) {
                    int idx = atl.indexOf( id );
                    m_currentTrack = ( idx > 0 ) ? atl.at( idx - 1 ) : 0;
                }
                atl.removeAll( id );
                if ( atl.isEmpty() ) {
                    debug() << album->prettyName() << "is now empty";
                    alb_iter = m_albumGroups.erase( alb_iter );
                    m_playedAlbums.removeAll( album );
                    m_unplayedAlbums.removeAll( album );
                    if ( album == m_currentAlbum )
                        m_currentAlbum = ( m_unplayedAlbums.isEmpty() ) ? Meta::AlbumPtr() : m_unplayedAlbums.takeFirst();
                } else {
                    ++alb_iter;
                    m_albumGroups.insert( album, atl );
                }
                break;
            } else {
                ++alb_iter;
            }
        }
    }
    //dump();
}

void
Playlist::RandomAlbumNavigator::recvActiveTrackChanged( const quint64 id )
{
    if ( id == m_currentTrack )
        return;

    if ( !m_albumGroups.value( m_currentAlbum ).contains( id ) )
    {
        if ( m_currentAlbum != Meta::AlbumPtr() )
            m_playedAlbums.prepend( m_currentAlbum );
        QHash<Meta::AlbumPtr, ItemList>::iterator alb_iter;
        for ( alb_iter = m_albumGroups.begin(); alb_iter != m_albumGroups.end(); ++alb_iter )
        {
            if ( alb_iter->contains( id ) )
            {
                Meta::AlbumPtr album = alb_iter.key();
                if ( m_playedAlbums.contains( album ) )
                {
                    m_currentAlbum = m_playedAlbums.takeAt( m_playedAlbums.indexOf( album ) );
                }
                else
                {
                    m_currentAlbum = m_unplayedAlbums.takeAt( m_unplayedAlbums.indexOf( album ) );
                }
            }
        }
    }
    m_currentTrack = id;

    //dump();
}

quint64
Playlist::RandomAlbumNavigator::requestNextTrack()
{
    if( !m_queue.isEmpty() )
        return m_queue.takeFirst();
    if ( m_unplayedAlbums.isEmpty() && m_currentAlbum == Meta::AlbumPtr() )
        return 0;
    if ( m_unplayedAlbums.isEmpty() && m_repeatPlaylist )
    {
        m_unplayedAlbums = m_playedAlbums;
        m_playedAlbums.clear();
    }

    if ( m_albumGroups.contains( m_currentAlbum ) )
    {
        ItemList atl = m_albumGroups.value( m_currentAlbum );
        int idx = atl.indexOf( m_currentTrack );
        if ( idx < ( atl.size() - 1 ) )
        {
            m_currentTrack = atl.at( idx + 1 );
        }
        else
        {
            m_playedAlbums.prepend( m_currentAlbum );
            if ( !m_unplayedAlbums.isEmpty() )
            {
                m_currentAlbum = m_unplayedAlbums.takeFirst();
                m_currentTrack = m_albumGroups.value( m_currentAlbum ).first();
            }
            else
            {
                m_currentAlbum = Meta::AlbumPtr();
                m_currentTrack = 0;
            }
        }
    }
    else
    {
        if ( !m_unplayedAlbums.isEmpty() )
        {
            m_currentAlbum = m_unplayedAlbums.takeFirst();
            m_currentTrack = m_albumGroups.value( m_currentAlbum ).first();
        }
    }
    return m_currentTrack;
}

quint64
Playlist::RandomAlbumNavigator::requestLastTrack()
{
    if ( m_unplayedAlbums.isEmpty() && m_currentAlbum == Meta::AlbumPtr() )
        return 0;

    if ( m_playedAlbums.isEmpty() && m_repeatPlaylist )
    {
        m_playedAlbums = m_unplayedAlbums;
        m_unplayedAlbums.clear();
    }

    if ( m_albumGroups.contains( m_currentAlbum ) )
    {
        ItemList atl = m_albumGroups.value( m_currentAlbum );
        int idx = atl.indexOf( m_currentTrack );
        if ( idx > 0 )
        {
            m_currentTrack = atl.at( idx - 1 );
        }
        else
        {
            m_unplayedAlbums.prepend( m_currentAlbum );
            if ( !m_playedAlbums.isEmpty() )
            {
                m_currentAlbum = m_playedAlbums.takeFirst();
                m_currentTrack = m_albumGroups.value( m_currentAlbum ).last();
            }
            else
            {
                m_currentAlbum = Meta::AlbumPtr();
                m_currentTrack = 0;
            }
        }
    }
    else
    {
        if ( !m_playedAlbums.isEmpty() )
        {
            m_currentAlbum = m_playedAlbums.takeFirst();
            m_currentTrack = m_albumGroups.value( m_currentAlbum ).last();
        }
    }
    return m_currentTrack;
}

bool
Playlist::RandomAlbumNavigator::idLessThan( const quint64& l, const quint64& r )
{
    Meta::TrackPtr left = Playlist::ModelStack::instance()->top()->trackForId( l );
    Meta::TrackPtr right = Playlist::ModelStack::instance()->top()->trackForId( r );

    return Meta::Track::lessThan( left, right );
}

void
Playlist::RandomAlbumNavigator::sortTheseAlbums( const Meta::AlbumList al )
{
    foreach( Meta::AlbumPtr a, al )
    {
        qStableSort( m_albumGroups[a].begin(), m_albumGroups[a].end(), idLessThan );
    }
}

void
Playlist::RandomAlbumNavigator::dump()
{
    debug() << "album groups are as follows:";
    debug() << "unplayed:";
    foreach( Meta::AlbumPtr album, m_unplayedAlbums )
    {
        debug() << "   in" << album->prettyName();
        ItemList atl = m_albumGroups.value( album );
        foreach( quint64 id, atl )
        {
            Meta::TrackPtr track = m_model->trackForId( id );
            debug() << "      " << track->trackNumber() << track->prettyName() << id;
        }
    }
    if ( m_currentAlbum != Meta::AlbumPtr() )
    {
        debug() << "current:";
        debug() << "   in" << m_currentAlbum->prettyName();
        ItemList atl = m_albumGroups.value( m_currentAlbum );
        foreach( quint64 id, atl )
        {
            Meta::TrackPtr track = m_model->trackForId( id );
            debug() << "      " << track->trackNumber() << track->prettyName() << id << (( id == m_currentTrack ) ? "***" : "" );
        }
    }
    debug() << "played:";
    foreach( Meta::AlbumPtr album, m_playedAlbums )
    {
        debug() << "   in" << album->prettyName();
        ItemList atl = m_albumGroups.value( album );
        foreach( quint64 id, atl )
        {
            Meta::TrackPtr track = m_model->trackForId( id );
            debug() << "      " << track->trackNumber() << track->prettyName() << id;
        }
    }
}

void Playlist::RandomAlbumNavigator::reset()
{
    m_albumGroups.clear();

    for ( int i = 0; i < m_model->rowCount(); i++ )
    {
        Meta::AlbumPtr album = m_model->trackAt( i )->album();
        m_albumGroups[album].append( m_model->idAt( i ) ); // conveniently creates an empty list if none exists
    }

    m_unplayedAlbums = m_albumGroups.uniqueKeys();
    std::random_shuffle( m_unplayedAlbums.begin(), m_unplayedAlbums.end() );

    if ( m_unplayedAlbums.size() )
        sortTheseAlbums( m_unplayedAlbums );

    m_currentAlbum = Meta::AlbumPtr();
    m_currentTrack = 0;
}
