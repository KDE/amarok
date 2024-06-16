/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#define DEBUG_PREFIX "AggregateCollection"

#include "AggregateCollection.h"

#include "core/support/Debug.h"
#include "core-impl/collections/aggregate/AggregateMeta.h"
#include "core-impl/collections/aggregate/AggregateQueryMaker.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <QIcon>

#include <QReadLocker>
#include <QTimer>

using namespace Collections;

AggregateCollection::AggregateCollection()
        : Collections::Collection()
{
    QTimer *timer = new QTimer( this );
    timer->setSingleShot( false );
    timer->setInterval( 60000 ); //clean up every 60 seconds
    connect( timer, &QTimer::timeout, this, &AggregateCollection::emptyCache );
    timer->start();
}

AggregateCollection::~AggregateCollection()
{
}

QString
AggregateCollection::prettyName() const
{
    return i18nc( "Name of the virtual collection that merges tracks from all collections",
                  "Aggregate Collection" );
}

QIcon
AggregateCollection::icon() const
{
    return QIcon::fromTheme(QStringLiteral("drive-harddisk"));
}

bool
AggregateCollection::possiblyContainsTrack( const QUrl &url ) const
{
    for( Collections::Collection *collection : m_idCollectionMap )
    {
        if( collection->possiblyContainsTrack( url ) )
            return true;
    }
    return false;
}

Meta::TrackPtr
AggregateCollection::trackForUrl( const QUrl &url )
{
    for( Collections::Collection *collection : m_idCollectionMap )
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
AggregateCollection::queryMaker()
{
    QList<QueryMaker*> list;
    for( Collections::Collection *collection : m_idCollectionMap )
    {
        list.append( collection->queryMaker() );
    }
    return new Collections::AggregateQueryMaker( this, list );
}

QString
AggregateCollection::collectionId() const
{
    // do we need more than one AggregateCollection?
    return QStringLiteral( "AggregateCollection" );
}

void
AggregateCollection::addCollection( Collections::Collection *collection, CollectionManager::CollectionStatus status )
{
    if( !collection )
        return;

    if( !( status & CollectionManager::CollectionViewable ) )
        return;

    m_idCollectionMap.insert( collection->collectionId(), collection );
    //TODO
    Q_EMIT updated();
}

void
AggregateCollection::removeCollectionById( const QString &collectionId )
{
    m_idCollectionMap.remove( collectionId );
    Q_EMIT updated();
}

void
AggregateCollection::removeCollection( Collections::Collection *collection )
{
    m_idCollectionMap.remove( collection->collectionId() );
    Q_EMIT updated();
}

void
AggregateCollection::slotUpdated()
{
    //TODO
    Q_EMIT updated();
}

void
AggregateCollection::removeYear( const QString &name )
{
    m_yearLock.lockForWrite();
    m_yearMap.remove( name );
    m_yearLock.unlock();
}

Meta::AggreagateYear*
AggregateCollection::getYear( Meta::YearPtr year )
{
    m_yearLock.lockForRead();
    if( m_yearMap.contains( year->name() ) )
    {
        AmarokSharedPointer<Meta::AggreagateYear> aggregateYear = m_yearMap.value( year->name() );
        aggregateYear->add( year );
        m_yearLock.unlock();
        return aggregateYear.data();
    }
    else
    {
        m_yearLock.unlock();
        m_yearLock.lockForWrite();
        //we might create two year instances with the same name here,
        //which would show some weird behaviour in other places
        Meta::AggreagateYear *aggregateYear = new Meta::AggreagateYear( this, year );
        m_yearMap.insert( year->name(), AmarokSharedPointer<Meta::AggreagateYear>( aggregateYear ) );
        m_yearLock.unlock();
        return aggregateYear;
    }
}

void
AggregateCollection::setYear( Meta::AggreagateYear *year )
{
    m_yearLock.lockForWrite();
    m_yearMap.insert( year->name(), AmarokSharedPointer<Meta::AggreagateYear>( year ) );
    m_yearLock.unlock();
}

bool
AggregateCollection::hasYear( const QString &name )
{
    QReadLocker locker( &m_yearLock );
    return m_yearMap.contains( name );
}

void
AggregateCollection::removeGenre( const QString &name )
{
    m_genreLock.lockForWrite();
    m_genreMap.remove( name );
    m_genreLock.unlock();
}

Meta::AggregateGenre*
AggregateCollection::getGenre( Meta::GenrePtr genre )
{
    m_genreLock.lockForRead();
    if( m_genreMap.contains( genre->name() ) )
    {
        AmarokSharedPointer<Meta::AggregateGenre> aggregateGenre = m_genreMap.value( genre->name() );
        aggregateGenre->add( genre );
        m_genreLock.unlock();
        return aggregateGenre.data();
    }
    else
    {
        m_genreLock.unlock();
        m_genreLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        Meta::AggregateGenre *aggregateGenre = new Meta::AggregateGenre( this, genre );
        m_genreMap.insert( genre->name(), AmarokSharedPointer<Meta::AggregateGenre>( aggregateGenre ) );
        m_genreLock.unlock();
        return aggregateGenre;
    }
}

void
AggregateCollection::setGenre( Meta::AggregateGenre *genre )
{
    m_genreLock.lockForWrite();
    m_genreMap.insert( genre->name(), AmarokSharedPointer<Meta::AggregateGenre>( genre ) );
    m_genreLock.unlock();
}

bool
AggregateCollection::hasGenre( const QString &genre )
{
    QReadLocker locker( &m_genreLock );
    return m_genreMap.contains( genre );
}

void
AggregateCollection::removeComposer( const QString &name )
{
    m_composerLock.lockForWrite();
    m_composerMap.remove( name );
    m_composerLock.unlock();
}

Meta::AggregateComposer*
AggregateCollection::getComposer( Meta::ComposerPtr composer )
{
    m_composerLock.lockForRead();
    if( m_composerMap.contains( composer->name() ) )
    {
        AmarokSharedPointer<Meta::AggregateComposer> aggregateComposer = m_composerMap.value( composer->name() );
        aggregateComposer->add( composer );
        m_composerLock.unlock();
        return aggregateComposer.data();
    }
    else
    {
        m_composerLock.unlock();
        m_composerLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        Meta::AggregateComposer *aggregateComposer = new Meta::AggregateComposer( this, composer );
        m_composerMap.insert( composer->name(), AmarokSharedPointer<Meta::AggregateComposer>( aggregateComposer ) );
        m_composerLock.unlock();
        return aggregateComposer;
    }
}

void
AggregateCollection::setComposer( Meta::AggregateComposer *composer )
{
    m_composerLock.lockForWrite();
    m_composerMap.insert( composer->name(), AmarokSharedPointer<Meta::AggregateComposer>( composer ) );
    m_composerLock.unlock();
}

bool
AggregateCollection::hasComposer( const QString &name )
{
    QReadLocker locker( &m_composerLock );
    return m_composerMap.contains( name );
}

void
AggregateCollection::removeArtist( const QString &name )
{
    m_artistLock.lockForWrite();
    m_artistMap.remove( name );
    m_artistLock.unlock();
}

Meta::AggregateArtist*
AggregateCollection::getArtist( Meta::ArtistPtr artist )
{
    m_artistLock.lockForRead();
    if( m_artistMap.contains( artist->name() ) )
    {
        AmarokSharedPointer<Meta::AggregateArtist> aggregateArtist = m_artistMap.value( artist->name() );
        aggregateArtist->add( artist );
        m_artistLock.unlock();
        return aggregateArtist.data();
    }
    else
    {
        m_artistLock.unlock();
        m_artistLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        Meta::AggregateArtist *aggregateArtist = new Meta::AggregateArtist( this, artist );
        m_artistMap.insert( artist->name(), AmarokSharedPointer<Meta::AggregateArtist>( aggregateArtist ) );
        m_artistLock.unlock();
        return aggregateArtist;
    }
}

void
AggregateCollection::setArtist( Meta::AggregateArtist *artist )
{
    m_artistLock.lockForWrite();
    m_artistMap.insert( artist->name(), AmarokSharedPointer<Meta::AggregateArtist>( artist ) );
    m_artistLock.unlock();
}

bool
AggregateCollection::hasArtist( const QString &artist )
{
    QReadLocker locker( &m_artistLock );
    return m_artistMap.contains( artist );
}

void
AggregateCollection::removeAlbum( const QString &album, const QString &albumartist )
{
    Meta::AlbumKey key( album, albumartist );
    m_albumLock.lockForWrite();
    m_albumMap.remove( key );
    m_albumLock.unlock();
}

Meta::AggregateAlbum*
AggregateCollection::getAlbum( const Meta::AlbumPtr &album )
{
    Meta::AlbumKey key( album );
    m_albumLock.lockForRead();
    if( m_albumMap.contains( key ) )
    {
        AmarokSharedPointer<Meta::AggregateAlbum> aggregateAlbum = m_albumMap.value( key );
        aggregateAlbum->add( album );
        m_albumLock.unlock();
        return aggregateAlbum.data();
    }
    else
    {
        m_albumLock.unlock();
        m_albumLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        Meta::AggregateAlbum *aggregateAlbum = new Meta::AggregateAlbum( this, album );
        m_albumMap.insert( key, AmarokSharedPointer<Meta::AggregateAlbum>( aggregateAlbum ) );
        m_albumLock.unlock();
        return aggregateAlbum;
    }
}

void
AggregateCollection::setAlbum( Meta::AggregateAlbum *album )
{
    m_albumLock.lockForWrite();
    m_albumMap.insert( Meta::AlbumKey( Meta::AlbumPtr( album ) ),
                       AmarokSharedPointer<Meta::AggregateAlbum>( album ) );
    m_albumLock.unlock();
}

bool
AggregateCollection::hasAlbum( const QString &album, const QString &albumArtist )
{
    QReadLocker locker( &m_albumLock );
    return m_albumMap.contains( Meta::AlbumKey( album, albumArtist ) );
}

void
AggregateCollection::removeTrack( const Meta::TrackKey &key )
{
    m_trackLock.lockForWrite();
    m_trackMap.remove( key );
    m_trackLock.unlock();
}

Meta::AggregateTrack*
AggregateCollection::getTrack( const Meta::TrackPtr &track )
{
    const Meta::TrackKey key( track );
    m_trackLock.lockForRead();
    if( m_trackMap.contains( key ) )
    {
        AmarokSharedPointer<Meta::AggregateTrack> aggregateTrack = m_trackMap.value( key );
        aggregateTrack->add( track );
        m_trackLock.unlock();
        return aggregateTrack.data();
    }
    else
    {
        m_trackLock.unlock();
        m_trackLock.lockForWrite();
        //we might create two instances with the same name here,
        //which would show some weird behaviour in other places
        Meta::AggregateTrack *aggregateTrack = new Meta::AggregateTrack( this, track );
        m_trackMap.insert( key, AmarokSharedPointer<Meta::AggregateTrack>( aggregateTrack ) );
        m_trackLock.unlock();
        return aggregateTrack;
    }
}

void
AggregateCollection::setTrack( Meta::AggregateTrack *track )
{
    Meta::TrackPtr ptr( track );
    const Meta::TrackKey key( ptr );
    m_trackLock.lockForWrite();
    m_trackMap.insert( key, AmarokSharedPointer<Meta::AggregateTrack>( track ) );
    m_trackLock.unlock();
}

bool
AggregateCollection::hasTrack( const Meta::TrackKey &key )
{
    QReadLocker locker( &m_trackLock );
    return m_trackMap.contains( key );
}

bool
AggregateCollection::hasLabel( const QString &name )
{
    QReadLocker locker( &m_labelLock );
    return m_labelMap.contains( name );
}

void
AggregateCollection::removeLabel( const QString &name )
{
    QWriteLocker locker( &m_labelLock );
    m_labelMap.remove( name );
}

Meta::AggregateLabel*
AggregateCollection::getLabel( Meta::LabelPtr label )
{
    m_labelLock.lockForRead();
    if( m_labelMap.contains( label->name() ) )
    {
        AmarokSharedPointer<Meta::AggregateLabel> aggregateLabel = m_labelMap.value( label->name() );
        aggregateLabel->add( label );
        m_labelLock.unlock();
        return aggregateLabel.data();
    }
    else
    {
        m_labelLock.unlock();
        m_labelLock.lockForWrite();
        //we might create two year instances with the same name here,
        //which would show some weird behaviour in other places
        Meta::AggregateLabel *aggregateLabel = new Meta::AggregateLabel( this, label );
        m_labelMap.insert( label->name(), AmarokSharedPointer<Meta::AggregateLabel>( aggregateLabel ) );
        m_labelLock.unlock();
        return aggregateLabel;
    }
}

void
AggregateCollection::setLabel( Meta::AggregateLabel *label )
{
    QWriteLocker locker( &m_labelLock );
    m_labelMap.insert( label->name(), AmarokSharedPointer<Meta::AggregateLabel>( label ) );
}

void
AggregateCollection::emptyCache()
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
        //another reference is stored in m_uidMap
        #define foreachCollectGarbage( Key, Type, RefCount, x ) \
        for( QMutableHashIterator<Key,Type > iter(x); iter.hasNext(); ) \
        { \
            Type elem = iter.next().value(); \
            if( elem.count() == RefCount ) \
                iter.remove(); \
        }

        foreachCollectGarbage( Meta::TrackKey, AmarokSharedPointer<Meta::AggregateTrack>, 2, m_trackMap )
        //run before artist so that album artist pointers can be garbage collected
        foreachCollectGarbage( Meta::AlbumKey, AmarokSharedPointer<Meta::AggregateAlbum>, 2, m_albumMap )
        foreachCollectGarbage( QString, AmarokSharedPointer<Meta::AggregateArtist>, 2, m_artistMap )
        foreachCollectGarbage( QString, AmarokSharedPointer<Meta::AggregateGenre>, 2, m_genreMap )
        foreachCollectGarbage( QString, AmarokSharedPointer<Meta::AggregateComposer>, 2, m_composerMap )
        foreachCollectGarbage( QString, AmarokSharedPointer<Meta::AggreagateYear>, 2, m_yearMap )
        foreachCollectGarbage( QString, AmarokSharedPointer<Meta::AggregateLabel>, 2, m_labelMap )
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
