/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "scriptableservice.h"

#include "servicebrowser.h"
#include "ScriptableServiceInfoParser.h"
#include "amarok.h"
#include "debug.h"

using namespace Meta;

ScriptableService::ScriptableService( const QString &name )
        : ServiceBase( name )
        , m_trackIdCounter( 0 )
        , m_albumIdCounter( 0 )
        , m_artistIdCounter( 0 )
{
    m_infoParser = new ScriptableServiceInfoParser();

}

ServiceCollection * ScriptableService::collection()
{
    return m_collection;
}

void ScriptableService::setCollection( ServiceCollection * collection )
{
    m_collection = collection;

}

int ScriptableService::addTrack( ServiceTrack * track, int albumId )
{

    TrackPtr trackPtr = TrackPtr( track );
    m_collection->addTrack( track->name(), trackPtr );

    if ( albumIdMap.contains( albumId ) ) {

        AlbumPtr albumPtr = albumIdMap.value( albumId );
        ServiceAlbum * album = static_cast< ServiceAlbum * >( albumPtr.data() );
        track->setAlbum( albumPtr->name() );
        album->addTrack( trackPtr );

        m_trackIdCounter++;
        trackIdMap.insert( m_trackIdCounter, trackPtr );
        m_collection->acquireWriteLock();
        m_collection->addTrack( trackPtr->name(), trackPtr );
        m_collection->releaseLock();

        m_collection->emitUpdated();

        return m_trackIdCounter;

    }

    return -1;
}

int ScriptableService::addAlbum(ServiceAlbum * album)
{
    AlbumPtr albumPtr = AlbumPtr( album );
    m_albumIdCounter++;
    album->setId( m_albumIdCounter );
    albumIdMap.insert( m_albumIdCounter, albumPtr );
    m_collection->acquireWriteLock();
    m_collection->addAlbum( album->name(), albumPtr );
    m_collection->releaseLock();
    //m_collection->emitUpdated();
    return m_albumIdCounter;
}

int ScriptableService::addArtist(ServiceArtist * artist)
{
    Q_UNUSED( artist );
    return -1;
}

void ScriptableService::donePopulating( int parentId )
{
    DEBUG_BLOCK
    //m_collection->donePopulating( parentId );
}




#include "scriptableservice.moc"
