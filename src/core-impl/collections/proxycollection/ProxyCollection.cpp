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

#define DEBUG_PREFIX "ProxyCollection"

#include "ProxyCollection.h"

#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "ProxyCollectionMeta.h"
#include "ProxyCollectionQueryMaker.h"

#include <QReadLocker>
#include <QTimer>

#include <KIcon>

using namespace Collections;

ProxyCollection::ProxyCollection()
        : Collections::Collection()
{
    QTimer *timer = new QTimer( this );
    timer->setSingleShot( false );
    timer->setInterval( 60000 ); //clean up every 60 seconds
    connect( timer, SIGNAL( timeout() ), this, SLOT( emptyCache() ) );
    timer->start();
}

ProxyCollection::~ProxyCollection()
{
    //TODO
}

QString
ProxyCollection::prettyName() const
{
    return i18n( "Proxy Collection" );
}

KIcon
ProxyCollection::icon() const
{
    return KIcon("drive-harddisk");
}

bool
ProxyCollection::possiblyContainsTrack( const KUrl &url ) const
{
    foreach( Collections::Collection *collection, m_idCollectionMap )
    {
        if( collection->possiblyContainsTrack( url ) )
            return true;
    }
    return false;
}

Meta::TrackPtr
ProxyCollection::trackForUrl( const KUrl &url )
{
    foreach( Collections::Collection *collection, m_idCollectionMap )
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
ProxyCollection::queryMaker()
{
    QList<QueryMaker*> list;
    foreach( Collections::Collection *collection, m_idCollectionMap )
    {
        list.append( collection->queryMaker() );
    }
    return new Collections::ProxyQueryMaker( this, list );
}

QString
ProxyCollection::collectionId() const
{
    //do we need more than one proxycollection??
    return "ProxyCollection";
}

void
ProxyCollection::addCollection( Collections::Collection *collection, CollectionManager::CollectionStatus status )
{
    if( !collection )
        return;

    if( !( status & CollectionManager::CollectionViewable ) )
        return;

    m_idCollectionMap.insert( collection->collectionId(), collection );
    //TODO
    emit updated();
}

void
ProxyCollection::removeCollection( const QString &collectionId )
{
    m_idCollectionMap.remove( collectionId );
    emit updated();
}

void
ProxyCollection::removeCollection( Collections::Collection *collection )
{
    m_idCollectionMap.remove( collection->collectionId() );
    emit updated();
}

void
ProxyCollection::slotUpdated()
{
    //TODO
    emit updated();
}

void
ProxyCollection::removeYear( const QString &name )
{
    m_yearLock.lockForWrite();
    m_yearMap.remove( name );
    m_yearLock.unlock();
}

Meta::ProxyYear*
ProxyCollection::getYear( Meta::YearPtr year )
{
    m_yearLock.lockForRead();
    if( m_yearMap.contains( year->name() ) )
    {
        KSharedPtr<Meta::ProxyYear> proxyYear = m_yearMap.value( year->name() );
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
        Meta::ProxyYear *proxyYear = new Meta::ProxyYear( this, year );
        m_yearMap.insert( year->name(), KSharedPtr<Meta::ProxyYear>( proxyYear ) );
        m_yearLock.unlock();
        return proxyYear;
    }
}

void
ProxyCollection::setYear( Meta::ProxyYear *year )
{
    m_yearLock.lockForWrite();
    m_yearMap.insert( year->name(), KSharedPtr<Meta::ProxyYear>( year ) );
    m_yearLock.unlock();
}

bool
ProxyCollection::hasYear( const QString &name )
{
    QReadLocker locker( &m_yearLock );
    return m_yearMap.contains( name );
}

void
ProxyCollection::removeGenre( const QString &name )
{
    m_genreLock.lockForWrite();
    m_genreMap.remove( name );
    m_genreLock.unlock();
}

Meta::ProxyGenre*
ProxyCollection::getGenre( Meta::GenrePtr genre )
{
    m_genreLock.lockForRead();
    if( m_genreMap.contains( genre->name() ) )
    {
        KSharedPtr<Meta::ProxyGenre> proxy = m_genreMap.value( genre->name() );
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
        Meta::ProxyGenre *proxy = new Meta::ProxyGenre( this, genre );
        m_genreMap.insert( genre->name(), KSharedPtr<Meta::ProxyGenre>( proxy ) );
        m_genreLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::setGenre( Meta::ProxyGenre *genre )
{
    m_genreLock.lockForWrite();
    m_genreMap.insert( genre->name(), KSharedPtr<Meta::ProxyGenre>( genre ) );
    m_genreLock.unlock();
}

bool
ProxyCollection::hasGenre( const QString &genre )
{
    QReadLocker locker( &m_genreLock );
    return m_genreMap.contains( genre );
}

void
ProxyCollection::removeComposer( const QString &name )
{
    m_composerLock.lockForWrite();
    m_composerMap.remove( name );
    m_composerLock.unlock();
}

Meta::ProxyComposer*
ProxyCollection::getComposer( Meta::ComposerPtr composer )
{
    m_composerLock.lockForRead();
    if( m_composerMap.contains( composer->name() ) )
    {
        KSharedPtr<Meta::ProxyComposer> proxy = m_composerMap.value( composer->name() );
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
        Meta::ProxyComposer *proxy = new Meta::ProxyComposer( this, composer );
        m_composerMap.insert( composer->name(), KSharedPtr<Meta::ProxyComposer>( proxy ) );
        m_composerLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::setComposer( Meta::ProxyComposer *composer )
{
    m_composerLock.lockForWrite();
    m_composerMap.insert( composer->name(), KSharedPtr<Meta::ProxyComposer>( composer ) );
    m_composerLock.unlock();
}

bool
ProxyCollection::hasComposer( const QString &name )
{
    QReadLocker locker( &m_composerLock );
    return m_composerMap.contains( name );
}

void
ProxyCollection::removeArtist( const QString &name )
{
    m_artistLock.lockForWrite();
    m_artistMap.remove( name );
    m_artistLock.unlock();
}

Meta::ProxyArtist*
ProxyCollection::getArtist( Meta::ArtistPtr artist )
{
    m_artistLock.lockForRead();
    if( m_artistMap.contains( artist->name() ) )
    {
        KSharedPtr<Meta::ProxyArtist> proxy = m_artistMap.value( artist->name() );
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
        Meta::ProxyArtist *proxy = new Meta::ProxyArtist( this, artist );
        m_artistMap.insert( artist->name(), KSharedPtr<Meta::ProxyArtist>( proxy ) );
        m_artistLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::setArtist( Meta::ProxyArtist *artist )
{
    m_artistLock.lockForWrite();
    m_artistMap.insert( artist->name(), KSharedPtr<Meta::ProxyArtist>( artist ) );
    m_artistLock.unlock();
}

bool
ProxyCollection::hasArtist( const QString &artist )
{
    QReadLocker locker( &m_artistLock );
    return m_artistMap.contains( artist );
}

void
ProxyCollection::removeAlbum( const QString &album, const QString &albumartist )
{
    AlbumKey key;
    key.albumName = album;
    key.artistName = albumartist;
    m_albumLock.lockForWrite();
    m_albumMap.remove( key );
    m_albumLock.unlock();
}

Meta::ProxyAlbum*
ProxyCollection::getAlbum( Meta::AlbumPtr album )
{
    AlbumKey key;
    key.albumName = album->name();
    if( album->albumArtist() )
        key.artistName = album->albumArtist()->name();
    m_albumLock.lockForRead();
    if( m_albumMap.contains( key ) )
    {
        KSharedPtr<Meta::ProxyAlbum> proxy = m_albumMap.value( key );
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
        Meta::ProxyAlbum *proxy = new Meta::ProxyAlbum( this, album );
        m_albumMap.insert( key, KSharedPtr<Meta::ProxyAlbum>( proxy ) );
        m_albumLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::setAlbum( Meta::ProxyAlbum *album )
{
    AlbumKey key;
    key.albumName = album->name();
    if( album->albumArtist() )
        key.artistName = album->albumArtist()->name();
    m_albumLock.lockForWrite();
    m_albumMap.insert( key, KSharedPtr<Meta::ProxyAlbum>( album ) );
    m_albumLock.unlock();
}

bool
ProxyCollection::hasAlbum( const QString &album, const QString &albumArtist )
{
    AlbumKey key;
    key.albumName = album;
    key.artistName = albumArtist;
    QReadLocker locker( &m_albumLock );
    return m_albumMap.contains( key );
}

void
ProxyCollection::removeTrack( const TrackKey &key )
{
    m_trackLock.lockForWrite();
    m_trackMap.remove( key );
    m_trackLock.unlock();
}

Meta::ProxyTrack*
ProxyCollection::getTrack( Meta::TrackPtr track )
{
    const TrackKey key = Meta::keyFromTrack( track );
    m_trackLock.lockForRead();
    if( m_trackMap.contains( key ) )
    {
        KSharedPtr<Meta::ProxyTrack> proxy = m_trackMap.value( key );
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
        Meta::ProxyTrack *proxy = new Meta::ProxyTrack( this, track );
        m_trackMap.insert( key, KSharedPtr<Meta::ProxyTrack>( proxy ) );
        m_trackLock.unlock();
        return proxy;
    }
}

void
ProxyCollection::setTrack( Meta::ProxyTrack *track )
{
    Meta::TrackPtr ptr( track );
    const TrackKey key = Meta::keyFromTrack( ptr );
    m_trackLock.lockForWrite();
    m_trackMap.insert( key, KSharedPtr<Meta::ProxyTrack>( track ) );
    m_trackLock.unlock();
}

bool
ProxyCollection::hasTrack( const TrackKey &key )
{
    QReadLocker locker( &m_trackLock );
    return m_trackMap.contains( key );
}

bool
ProxyCollection::hasLabel( const QString &name )
{
    QReadLocker locker( &m_labelLock );
    return m_labelMap.contains( name );
}

void
ProxyCollection::removeLabel( const QString &name )
{
    QWriteLocker locker( &m_labelLock );
    m_labelMap.remove( name );
}

Meta::ProxyLabel*
ProxyCollection::getLabel( Meta::LabelPtr label )
{
    m_labelLock.lockForRead();
    if( m_labelMap.contains( label->name() ) )
    {
        KSharedPtr<Meta::ProxyLabel> proxyLabel = m_labelMap.value( label->name() );
        proxyLabel->add( label );
        m_labelLock.unlock();
        return proxyLabel.data();
    }
    else
    {
        m_labelLock.unlock();
        m_labelLock.lockForWrite();
        //we might create two year instances with the same name here,
        //which would show some weird behaviour in other places
        Meta::ProxyLabel *proxyLabel = new Meta::ProxyLabel( this, label );
        m_labelMap.insert( label->name(), KSharedPtr<Meta::ProxyLabel>( proxyLabel ) );
        m_labelLock.unlock();
        return proxyLabel;
    }
}

void
ProxyCollection::setLabel( Meta::ProxyLabel *label )
{
    QWriteLocker locker( &m_labelLock );
    m_labelMap.insert( label->name(), KSharedPtr<Meta::ProxyLabel>( label ) );
}

void
ProxyCollection::emptyCache()
{
    bool hasTrack, hasAlbum, hasArtist, hasYear, hasGenre, hasComposer, hasLabel;
    hasTrack = hasAlbum = hasArtist = hasYear = hasGenre = hasComposer = hasLabel = false;

    //try to avoid possible deadlocks by aborting when we can't get all locks
    if ( ( hasTrack = m_trackLock.tryLockForWrite() )
         && ( hasAlbum = m_albumLock.tryLockForWrite() )
         && ( hasArtist = m_artistLock.tryLockForWrite() )
         && ( hasYear = m_yearLock.tryLockForWrite() )
         && ( hasGenre = m_genreLock.tryLockForWrite() )
         && ( hasComposer = m_composerLock.tryLockForWrite() )
         && ( hasLabel = m_labelLock.tryLockForWrite() ) )
    {
        //this very simple garbage collector doesn't handle cyclic object graphs
        //so care has to be taken to make sure that we are not dealing with a cyclic graph
        //by invalidating the tracks cache on all objects
        #define foreachInvalidateCache( Type, RealType, x ) \
        for( QMutableHashIterator<int,Type > iter(x); iter.hasNext(); ) \
            RealType::staticCast( iter.next().value() )->invalidateCache()

        //elem.count() == 2 is correct because elem is one pointer to the object
        //and the other is stored in the hash map (except for m_trackMap, where
        //another refence is stored in m_uidMap
        #define foreachCollectGarbage( Key, Type, RefCount, x ) \
        for( QMutableHashIterator<Key,Type > iter(x); iter.hasNext(); ) \
        { \
            Type elem = iter.next().value(); \
            if( elem.count() == RefCount ) \
                iter.remove(); \
        }

        foreachCollectGarbage( TrackKey, KSharedPtr<Meta::ProxyTrack>, 2, m_trackMap )
        //run before artist so that album artist pointers can be garbage collected
        foreachCollectGarbage( AlbumKey, KSharedPtr<Meta::ProxyAlbum>, 2, m_albumMap )
        foreachCollectGarbage( QString, KSharedPtr<Meta::ProxyArtist>, 2, m_artistMap )
        foreachCollectGarbage( QString, KSharedPtr<Meta::ProxyGenre>, 2, m_genreMap )
        foreachCollectGarbage( QString, KSharedPtr<Meta::ProxyComposer>, 2, m_composerMap )
        foreachCollectGarbage( QString, KSharedPtr<Meta::ProxyYear>, 2, m_yearMap )
        foreachCollectGarbage( QString, KSharedPtr<Meta::ProxyLabel>, 2, m_labelMap )
    }

    //make sure to unlock all necessary locks
    //important: calling unlock() on an unlocked mutex gives an undefined result
    //unlocking a mutex locked by another thread results in an error, so be careful
    if( hasTrack ) m_trackLock.unlock();
    if( hasAlbum ) m_albumLock.unlock();
    if( hasArtist ) m_artistLock.unlock();
    if( hasYear ) m_yearLock.unlock();
    if( hasGenre ) m_genreLock.unlock();
    if( hasComposer ) m_composerLock.unlock();
    if( hasLabel ) m_labelLock.unlock();
}
