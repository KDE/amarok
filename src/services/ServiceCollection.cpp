/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#define DEBUG_PREFIX "ServiceCollection"

#include "ServiceCollection.h"

#include "ServiceCollectionLocation.h"
#include "amarokconfig.h"
#include "ServiceMetaBase.h"
#include "core/support/Debug.h"
#include "support/MemoryQueryMaker.h"
//#include "reader.h"


using namespace Collections;


//ServiceCollection

ServiceCollection::ServiceCollection( ServiceBase * service )
    : Collection()
    , m_service( service )
    , m_mc( new MemoryCollection() )
{
}

ServiceCollection::ServiceCollection( ServiceBase * service, const QString &id, const QString &prettyName )
    : Collection()
    , m_service( service )
    , m_mc( new MemoryCollection() )
    , m_collectionId( id )
    , m_prettyName( prettyName )
{
}

ServiceCollection::~ServiceCollection()
{
}

void
ServiceCollection::startFullScan()
{
    //ignore
}

Collections::QueryMaker*
ServiceCollection::queryMaker()
{
    return new Collections::MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString
ServiceCollection::collectionId() const
{
    return m_collectionId;
}

QString
ServiceCollection::prettyName() const
{
    return m_prettyName;
}

CollectionLocation*
ServiceCollection::location() const
{
    return new ServiceCollectionLocation( this );
}


ServiceBase * ServiceCollection::service()
{
    return m_service;
}

void ServiceCollection::emitUpdated()
{
    emit( updated() );
}


void ServiceCollection::addTrack( Meta::TrackPtr trackPtr )
{
    m_mc->addTrack( trackPtr );
    const Meta::ServiceTrackPtr track = Meta::ServiceTrackPtr::dynamicCast( trackPtr );

    if ( track && track->id() != 0 )
        m_trackIdMap.insert( track->id(), trackPtr );
}

void ServiceCollection::addArtist( Meta::ArtistPtr artistPtr )
{
    m_mc->addArtist( artistPtr );
    const Meta::ServiceArtistPtr artist = Meta::ServiceArtistPtr::dynamicCast( artistPtr );

    if ( artist && artist->id() != 0 )
        m_artistIdMap.insert( artist->id(), artistPtr );
}

void ServiceCollection::addAlbum( Meta::AlbumPtr albumPtr )
{
    m_mc->addAlbum( albumPtr );
    const Meta::ServiceAlbumPtr album = Meta::ServiceAlbumPtr::dynamicCast( albumPtr );

    if ( album && album->id() != 0 )
        m_albumIdMap.insert( album->id(), albumPtr );
}

void ServiceCollection::addGenre( Meta::GenrePtr genrePtr )
{
    m_mc->addGenre( genrePtr );
    const Meta::ServiceGenrePtr genre = Meta::ServiceGenrePtr::dynamicCast( genrePtr );

    if ( genre && genre->id() != 0 )
        m_genreIdMap.insert( genre->id(), genrePtr );
}

Meta::TrackPtr ServiceCollection::trackById( int id )
{
    return m_trackIdMap.value( id );
}

Meta::AlbumPtr ServiceCollection::albumById( int id )
{
    return m_albumIdMap.value( id );
}

Meta::ArtistPtr ServiceCollection::artistById( int id )
{
    return m_artistIdMap.value( id );
}

Meta::GenrePtr ServiceCollection::genreById( int id )
{
    return m_genreIdMap.value( id );
}


#include "ServiceCollection.moc"

