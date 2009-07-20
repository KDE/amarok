/*
 *  Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ProxyCollection.h"

#include "ProxyCollectionMeta.h"
#include "ProxyCollectionQueryMaker.h"

ProxyCollection::Collection::Collection()
        : Amarok::Collection()
{
    //TODO
}

ProxyCollection::Collection::~Collection()
{
    //TODO
}

bool
ProxyCollection::Collection::possiblyContainsTrack( const KUrl &url ) const
{
    foreach( Amarok::Collection *collection, m_idCollectionMap )
    {
        if( collection->possiblyContainsTrack( url ) )
            return true;
    }
    return false;
}

Meta::TrackPtr
ProxyCollection::Collection::trackForUrl( const KUrl &url )
{
    foreach( Amarok::Collection *collection, m_idCollectionMap )
    {
        Meta::TrackPtr track = collection->trackForUrl( url );
        if( track )
        {
            //theoretically we should now query the other collections for the same track
            //not sure how to do that yet though...
            return Meta::TrackPtr( getTrack( track ) );
        }
    }
    return Meta::TrackPtr();
}

QueryMaker*
ProxyCollection::Collection::queryMaker()
{
    QList<QueryMaker*> list;
    foreach( Amarok::Collection *collection, m_idCollectionMap )
    {
        list.append( collection->queryMaker() );
    }
    return new ProxyCollection::ProxyQueryMaker( this, list );
}

QString
ProxyCollection::Collection::collectionId() const
{
    //do we need more than one proxycollection??
    return "ProxyCollection";
}

void
ProxyCollection::Collection::addCollection( Amarok::Collection *collection )
{
    if( !collection )
        return;

    m_idCollectionMap.insert( collection->collectionId(), collection );
    //TODO
}

void
ProxyCollection::Collection::removeCollection( const QString &collectionId )
{
    m_idCollectionMap.remove( collectionId );
}

void
ProxyCollection::Collection::slotUpdated()
{
    //TODO
    emit updated();
}

void
ProxyCollection::Collection::removeYear( const QString &name )
{
    m_yearLock.lockForWrite();
    m_yearMap.remove( name );
    m_yearLock.unlock();
}

ProxyCollection::Year*
ProxyCollection::Collection::getYear( Meta::YearPtr year )
{
    m_yearLock.lockForRead();
    if( m_yearMap.contains( year->name() ) )
    {
        KSharedPtr<ProxyCollection::Year> proxyYear = m_yearMap.value( year->name() );
        proxyYear->add( year );
        m_yearLock.unlock();
        return proxyYear.data();
    }
    else
    {
        m_yearLock.unlock();
        m_yearLock.lockForWrite();
        //we might create two year instances with the same name here,
        //which would show some weird behaviour in other places
        ProxyCollection::Year *proxyYear = new ProxyCollection::Year( this, year );
        m_yearMap.insert( year->name(), KSharedPtr<Year>( proxyYear ) );
        m_yearLock.unlock();
        return proxyYear;
    }
}

void
ProxyCollection::Collection::setYear( ProxyCollection::Year *year )
{
    m_yearLock.lockForWrite();
    m_yearMap.insert( year->name(), KSharedPtr<ProxyCollection::Year>( year ) );
    m_yearLock.unlock();
}

void
ProxyCollection::Collection::removeGenre( const QString &name )
{
    m_genreLock.lockForWrite();
    m_genreMap.remove( name );
    m_genreLock.unlock();
}

ProxyCollection::Genre*
ProxyCollection::Collection::getGenre( Meta::GenrePtr genre )
{
    m_genreLock.lockForRead();
    if( m_genreMap.contains( genre->name() ) )
    {
        KSharedPtr<ProxyCollection::Genre> proxy = m_genreMap.value( genre->name() );
        proxy->add( genre );
        m_genreLock.unlock();
        return proxy.data();
    }
    else
    {
        m_genreLock.unlock();
        m_genreLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        ProxyCollection::Genre *proxy = new ProxyCollection::Genre( this, genre );
        m_genreMap.insert( genre->name(), KSharedPtr<ProxyCollection::Genre>( proxy ) );
        m_genreLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::Collection::setGenre( ProxyCollection::Genre *genre )
{
    m_genreLock.lockForWrite();
    m_genreMap.insert( genre->name(), KSharedPtr<ProxyCollection::Genre>( genre ) );
    m_genreLock.unlock();
}

void
ProxyCollection::Collection::removeComposer( const QString &name )
{
    m_composerLock.lockForWrite();
    m_composerMap.remove( name );
    m_composerLock.unlock();
}

ProxyCollection::Composer*
ProxyCollection::Collection::getComposer( Meta::ComposerPtr composer )
{
    m_composerLock.lockForRead();
    if( m_composerMap.contains( composer->name() ) )
    {
        KSharedPtr<ProxyCollection::Composer> proxy = m_composerMap.value( composer->name() );
        proxy->add( composer );
        m_composerLock.unlock();
        return proxy.data();
    }
    else
    {
        m_composerLock.unlock();
        m_composerLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        ProxyCollection::Composer *proxy = new ProxyCollection::Composer( this, composer );
        m_composerMap.insert( composer->name(), KSharedPtr<ProxyCollection::Composer>( proxy ) );
        m_composerLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::Collection::setComposer( ProxyCollection::Composer *composer )
{
    m_composerLock.lockForWrite();
    m_composerMap.insert( composer->name(), KSharedPtr<ProxyCollection::Composer>( composer ) );
    m_composerLock.unlock();
}

void
ProxyCollection::Collection::removeArtist( const QString &name )
{
    m_artistLock.lockForWrite();
    m_artistMap.remove( name );
    m_artistLock.unlock();
}

ProxyCollection::Artist*
ProxyCollection::Collection::getArtist( Meta::ArtistPtr artist )
{
    m_artistLock.lockForRead();
    if( m_artistMap.contains( artist->name() ) )
    {
        KSharedPtr<ProxyCollection::Artist> proxy = m_artistMap.value( artist->name() );
        proxy->add( artist );
        m_artistLock.unlock();
        return proxy.data();
    }
    else
    {
        m_artistLock.unlock();
        m_artistLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        ProxyCollection::Artist *proxy = new ProxyCollection::Artist( this, artist );
        m_artistMap.insert( artist->name(), KSharedPtr<ProxyCollection::Artist>( proxy ) );
        m_artistLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::Collection::setArtist( ProxyCollection::Artist *artist )
{
    m_artistLock.lockForWrite();
    m_artistMap.insert( artist->name(), KSharedPtr<ProxyCollection::Artist>( artist ) );
    m_artistLock.unlock();
}

void
ProxyCollection::Collection::removeAlbum( const QString &album, const QString &albumartist )
{
    ProxyCollection::AlbumKey key;
    key.albumName = album;
    key.artistName = albumartist;
    m_albumLock.lockForWrite();
    m_albumMap.remove( key );
    m_albumLock.unlock();
}

ProxyCollection::Album*
ProxyCollection::Collection::getAlbum( Meta::AlbumPtr album )
{
    ProxyCollection::AlbumKey key;
    key.albumName = album->name();
    if( album->albumArtist() )
        key.artistName = album->albumArtist()->name();
    m_albumLock.lockForRead();
    if( m_albumMap.contains( key ) )
    {
        KSharedPtr<ProxyCollection::Album> proxy = m_albumMap.value( key );
        proxy->add( album );
        m_albumLock.unlock();
        return proxy.data();
    }
    else
    {
        m_albumLock.unlock();
        m_albumLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        ProxyCollection::Album *proxy = new ProxyCollection::Album( this, album );
        m_albumMap.insert( key, KSharedPtr<ProxyCollection::Album>( proxy ) );
        m_albumLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::Collection::setAlbum( ProxyCollection::Album *album )
{
    ProxyCollection::AlbumKey key;
    key.albumName = album->name();
    if( album->albumArtist() )
        key.artistName = album->albumArtist()->name();
    m_albumLock.lockForWrite();
    m_albumMap.insert( key, KSharedPtr<ProxyCollection::Album>( album ) );
    m_albumLock.unlock();
}

void
ProxyCollection::Collection::removeTrack( const TrackKey &key )
{
    m_trackLock.lockForWrite();
    m_trackMap.remove( key );
    m_trackLock.unlock();
}

ProxyCollection::Track*
ProxyCollection::Collection::getTrack( Meta::TrackPtr track )
{
    const ProxyCollection::TrackKey key = ProxyCollection::keyFromTrack( track );
    m_trackLock.lockForRead();
    if( m_trackMap.contains( key ) )
    {
        KSharedPtr<ProxyCollection::Track> proxy = m_trackMap.value( key );
        proxy->add( track );
        m_trackLock.unlock();
        return proxy.data();
    }
    else
    {
        m_trackLock.unlock();
        m_trackLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        ProxyCollection::Track *proxy = new ProxyCollection::Track( this, track );
        m_trackMap.insert( key, KSharedPtr<ProxyCollection::Track>( proxy ) );
        m_trackLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::Collection::setTrack( ProxyCollection::Track *track )
{
    Meta::TrackPtr ptr( track );
    const ProxyCollection::TrackKey key = ProxyCollection::keyFromTrack( ptr );
    m_trackLock.lockForWrite();
    m_trackMap.insert( key, KSharedPtr<ProxyCollection::Track>( track ) );
    m_trackLock.unlock();
}

ProxyCollection::TrackKey
ProxyCollection::keyFromTrack( const Meta::TrackPtr &track )
{
    ProxyCollection::TrackKey k;
    k.trackName = track->name();
    if( track->artist() )
        k.artistName = track->artist()->name();

    if( track->album() )
        k.albumName = track->album()->name();

    return k;
}
