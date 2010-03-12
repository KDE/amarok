/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::RepeatAlbumNavigator"

#include "RepeatAlbumNavigator.h"

#include "Debug.h"
#include "Meta.h"
#include "playlist/PlaylistModelStack.h"

Playlist::RepeatAlbumNavigator::RepeatAlbumNavigator()
{
    DEBUG_BLOCK
    m_model = Playlist::ModelStack::instance()->top();
    connect( model(), SIGNAL( insertedIds( const QList<quint64>& ) ),
             this, SLOT( recvInsertedIds( const QList<quint64>& ) ) );
    connect( model(), SIGNAL( removedIds( const QList<quint64>& ) ),
             this, SLOT( recvRemovedIds( const QList<quint64>& ) ) );
    connect( model(), SIGNAL( activeTrackChanged( const quint64 ) ),
             this, SLOT( recvActiveTrackChanged( const quint64 ) ) );

    for ( int i = 0; i < m_model->rowCount(); i++ )
    {
        Meta::AlbumPtr album = m_model->trackAt( i )->album();
        m_albumGroups[album->name()].append( m_model->idAt( i ) ); // conveniently creates an empty list if none exists
    }

    Meta::TrackPtr activeTrack = m_model->activeTrack();
    m_currentAlbum = activeTrack ? activeTrack->album()->name() : QString();
    m_currentTrack = m_model->activeId();

    dump();
}

void
Playlist::RepeatAlbumNavigator::recvInsertedIds( const QList<quint64>& list )
{
    DEBUG_BLOCK

    foreach( quint64 id, list )
    {
        Meta::AlbumPtr album = m_model->trackForId( id )->album();
        m_albumGroups[album->name()].append( id ); // conveniently creates an empty list if none exists
    }
}

void
Playlist::RepeatAlbumNavigator::recvRemovedIds( const QList<quint64>& list )
{
    DEBUG_BLOCK
    QList<quint64>::const_iterator id_iter;
    for ( id_iter = list.begin(); id_iter != list.end(); ++id_iter )
    {
        quint64 id = *id_iter;
        debug() << "removing" << id;
        QHash<QString, ItemList>::iterator alb_iter = m_albumGroups.begin();

        while ( alb_iter != m_albumGroups.end() )
        {
            if ( alb_iter->contains( id ) )
            {

                if( !alb_iter.key().isEmpty() &&  alb_iter.key() != "Unknown" )
                    debug() << "    from" << alb_iter.key();
                else
                    debug() << "    which is not in any album";

                QString album = alb_iter.key();
                ItemList atl = alb_iter.value();
                if ( m_currentTrack == id )
                {
                    int idx = atl.indexOf( id );
                    m_currentTrack = ( idx < ( atl.size() - 1 ) ) ? atl.at( idx + 1 ) : 0;
                }
                atl.removeAll( id );
                if ( atl.isEmpty() )
                {
                    if( !album.isEmpty() )
                        debug() << album << "is now empty";
                    alb_iter = m_albumGroups.erase( alb_iter );
                    if ( album == m_currentAlbum )
                    {
                        m_currentAlbum = QString();
                        m_currentTrack = 0;
                    }
                }
                else
                {
                    ++alb_iter;
                    m_albumGroups.insert( album, atl );
                }
                break;
            }
            else
                ++alb_iter;
        }
    }
}

void
Playlist::RepeatAlbumNavigator::recvActiveTrackChanged( const quint64 id )
{
    DEBUG_BLOCK
    if ( id == m_currentTrack )
        return;

    if ( m_model->containsId( id ) )
    {
        if( m_model->trackForId( id )->album() )
            m_currentAlbum = m_model->trackForId( id )->album()->name();
        else m_currentAlbum = QString();

    }
    else
    {
        m_currentAlbum = QString();
    }
    m_currentTrack = id;
}


quint64
Playlist::RepeatAlbumNavigator::likelyNextTrack()
{
    DEBUG_BLOCK
    if ( !m_currentAlbum.isEmpty() )
    {

        debug() << "current album is " << m_currentAlbum;
        ItemList atl = m_albumGroups.value( m_currentAlbum );
        debug() << "it has " << atl.count() << " tracks in it";
        int row = atl.indexOf( m_currentTrack ) + 1;
        row = ( row < atl.size() ) ? row : 0;
        debug() << "row of next track is " << row;
        return atl.at( row );
    }
    return 0;
}

quint64
Playlist::RepeatAlbumNavigator::likelyLastTrack()
{
    DEBUG_BLOCK
    if ( !m_currentAlbum.isEmpty() )
    {
        ItemList atl = m_albumGroups.value( m_currentAlbum );
        int row = atl.indexOf( m_currentTrack ) - 1;
        row = ( row >= 0 ) ? row : atl.size() - 1;
        return atl.at( row );
    }
    return 0;
}

quint64
Playlist::RepeatAlbumNavigator::requestNextTrack()
{
    quint64 track = likelyNextTrack();
    if ( track )
        m_currentTrack = track;
    return track;
}

quint64
Playlist::RepeatAlbumNavigator::requestLastTrack()
{
    quint64 track = likelyLastTrack();
    if ( track )
        m_currentTrack = track;
    return track;
}

void
Playlist::RepeatAlbumNavigator::dump()
{
    debug() << "album groups are as follows:";
    foreach( QString album, m_albumGroups.keys() )
    {
        if( !album.isEmpty() )
        {
            debug() << "   in" << album;
            ItemList atl = m_albumGroups.value( album );
            foreach( quint64 id, atl )
            {
                Meta::TrackPtr track = m_model->trackForId( id );
                debug() << "      " << track->trackNumber() << track->prettyName() << id;
            }
        }
    }
}

