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

#include "SynchronizationBaseJob.h"

#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"

#include <QMetaEnum>
#include <QMetaObject>
#include <QHashIterator>

//this class might cause SqlQueryMaker to generate very large SQL statements
//see http://dev.mysql.com/doc/refman/5.0/en/packet-too-large.html
//if it turns out that the generated SQL query is too large

SynchronizationBaseJob::SynchronizationBaseJob()
        : QObject()
        , m_state( NotStarted )
        , m_currentResultCount( 0 )
        , m_collectionA( nullptr )
        , m_collectionB( nullptr )
{
    connect( &m_timer, &QTimer::timeout, this, &SynchronizationBaseJob::timeout );
    //abort syncing if both queries have not returned within 30 seconds for a given state
    //probably needs to be adjusted during testing
    m_timer.setInterval( 30000 );
    m_timer.setSingleShot( true );
}

SynchronizationBaseJob::~SynchronizationBaseJob()
{
    //nothing to do
}

void
SynchronizationBaseJob::setCollectionA( Collections::Collection *collection )
{
    m_collectionA = collection;
}

void
SynchronizationBaseJob::setCollectionB( Collections::Collection *collection )
{
    m_collectionB = collection;
}

void
SynchronizationBaseJob::setFilter( const QString &filter )
{
    Q_UNUSED( filter )
}

Collections::QueryMaker*
SynchronizationBaseJob::createQueryMaker( Collections::Collection *collection )
{
    //TODO: apply filters. This allows us to only sync a subset of a collection
    Collections::QueryMaker *qm = collection->queryMaker();
    qm->setAutoDelete( true );
    m_queryMakers.insert( qm, collection );
    return qm;
}

void
SynchronizationBaseJob::synchronize()
{
    DEBUG_BLOCK
    if( !m_collectionA || !m_collectionB )
    {
        debug() << "aborting synchronization, at least one collection is missing";
        deleteLater();
        return;
    }
    m_state = ComparingArtists;
    setupArtistQuery( m_collectionA )->run();
    setupArtistQuery( m_collectionB )->run();
    m_timer.start();
}

void
SynchronizationBaseJob::timeout()
{
    const QMetaObject *mo = metaObject();
    QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "State" ) );
    debug() << "syncing aborted due to a timeout in state " << me.valueToKey( m_state );
    deleteLater();
}

void
SynchronizationBaseJob::slotQueryDone()
{
    DEBUG_BLOCK;
    m_currentResultCount += 1;
    if( m_currentResultCount < 2 )
        return;
    m_currentResultCount = 0;

    m_timer.stop();
    switch( m_state )
    {
        case ComparingArtists:
        {
            m_state = ComparingAlbums;
            handleArtistResult();
            break;
        }
        case ComparingAlbums:
        {
            m_state = ComparingTracks;
            handleAlbumResult();
            break;
        }
        case ComparingTracks:
        {
            m_state = Syncing;
            handleTrackResult();
            break;
        }
        default:
        {
            const QMetaObject *mo = metaObject();
            QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "State" ) );
            debug() << "detected state " << me.valueToKey( m_state ) << " in slotQueryDone(), do not know how to handle this. Aborting";
            deleteLater();
            break;
        }
    }
}

Collections::QueryMaker*
SynchronizationBaseJob::setupArtistQuery( Collections::Collection *coll )
{
    Collections::QueryMaker *qm = createQueryMaker( coll );
    qm->setQueryType( Collections::QueryMaker::Artist );
    connect( qm, &Collections::QueryMaker::queryDone, this, &SynchronizationBaseJob::slotQueryDone, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::newArtistsReady, this, &SynchronizationBaseJob::slotArtistsReady, Qt::QueuedConnection );
    return qm;
}

Collections::QueryMaker*
SynchronizationBaseJob::setupAlbumQuery( Collections::Collection *coll )
{
    Collections::QueryMaker *qm = createQueryMaker( coll );
    qm->setQueryType( Collections::QueryMaker::Album );
    connect( qm, &Collections::QueryMaker::queryDone, this, &SynchronizationBaseJob::slotQueryDone, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::newAlbumsReady, this, &SynchronizationBaseJob::slotAlbumsReady, Qt::QueuedConnection );
    return qm;
}

Collections::QueryMaker*
SynchronizationBaseJob::setupTrackQuery( Collections::Collection *coll )
{
    Collections::QueryMaker *qm = createQueryMaker( coll );
    qm->setQueryType( Collections::QueryMaker::Track );
    connect( qm, &Collections::QueryMaker::queryDone, this, &SynchronizationBaseJob::slotQueryDone, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::newTracksReady, this, &SynchronizationBaseJob::slotTracksReady, Qt::QueuedConnection );
    return qm;
}

void
SynchronizationBaseJob::slotArtistsReady( const Meta::ArtistList &artists )
{
    DEBUG_BLOCK;
    Collections::Collection *senderColl = m_queryMakers.value( qobject_cast<Collections::QueryMaker*>(sender()) );
    QSet<QString> artistSet;
    for( const Meta::ArtistPtr &artist : artists )
    {
        if( artist )
            artistSet.insert( artist->name() );
    }
    if( senderColl == m_collectionA )
    {
        m_artistsA.unite( artistSet );
    }
    else if( senderColl == m_collectionB )
    {
        m_artistsB.unite( artistSet );
    }
    else
    {
        //huh? how did we get here?
    }
}

void
SynchronizationBaseJob::slotAlbumsReady( const Meta::AlbumList &albums )
{
    DEBUG_BLOCK
    Collections::Collection *senderColl = m_queryMakers.value( qobject_cast<Collections::QueryMaker*>(sender()) );
    QSet<Meta::AlbumKey> albumSet;
    for( const Meta::AlbumPtr &albumPtr : albums )
    {
        albumSet.insert( Meta::AlbumKey( albumPtr ) );
    }
    if( senderColl == m_collectionA )
    {
        m_albumsA.unite( albumSet );
    }
    else if( senderColl == m_collectionB )
    {
        m_albumsB.unite( albumSet );
    }
    else
    {
        //huh? how did we get here?
    }
}

void
SynchronizationBaseJob::slotTracksReady( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    Collections::Collection *senderColl = m_queryMakers.value( qobject_cast<Collections::QueryMaker*>(sender()) );
    if( senderColl == m_collectionA )
    {

        for( const Meta::TrackPtr &track : tracks )
        {
            Meta::TrackKey key( track );
            m_tracksA.insert( key );
            m_keyToTrackA.insert( key, track );
        }
    }
    else if( senderColl == m_collectionB )
    {
        for( const Meta::TrackPtr &track : tracks )
        {
            Meta::TrackKey key( track );
            m_tracksB.insert( key );
            m_keyToTrackB.insert( key, track );
        }
    }
    else
    {
        //huh? how did we get here?
    }
}

void
SynchronizationBaseJob::handleArtistResult()
{
    DEBUG_BLOCK
    QSet<QString> artistsOnlyInA = m_artistsA - m_artistsB;
    QSet<QString> artistsOnlyInB = m_artistsB - m_artistsA;
    QSet<QString> artistsInBoth = m_artistsA & m_artistsB;
    for( const QString &artist : artistsOnlyInA )
    {
        m_artistResult.insert( artist, OnlyInA );
    }
    for( const QString &artist : artistsOnlyInB )
    {
        m_artistResult.insert( artist, OnlyInB );
    }
    for( const QString &artist : artistsInBoth )
    {
        m_artistResult.insert( artist, InBoth );
    }
    Collections::QueryMaker *qmA = setupAlbumQuery( m_collectionA );
    Collections::QueryMaker *qmB = setupAlbumQuery( m_collectionB );
    //we are going to exclude artists below, so make sure we exclude all of them by setting the QMs to And mode
    qmA->beginAnd();
    qmB->beginAnd();
    QHashIterator<QString, InSet> iter( m_artistResult );
    while( iter.hasNext() )
    {
        iter.next();
        QString artist = iter.key();
        InSet currentStatus = iter.value();
        if( currentStatus == OnlyInA )
        {
            qmA->excludeFilter( Meta::valArtist, artist, true, true );
        }
        if( currentStatus == OnlyInB )
        {
            qmB->excludeFilter( Meta::valArtist, artist, true, true );
        }
    }
    qmA->endAndOr();
    qmB->endAndOr();
    m_timer.start();
    qmA->run();
    qmB->run();
}

void
SynchronizationBaseJob::handleAlbumResult()
{
    DEBUG_BLOCK
    QSet<Meta::AlbumKey> albumsOnlyInA = m_albumsA - m_albumsB;
    QSet<Meta::AlbumKey> albumsOnlyInB = m_albumsB - m_albumsA;
    QSet<Meta::AlbumKey> albumsInBoth = m_albumsA & m_albumsB;

    for( const Meta::AlbumKey &album : albumsOnlyInA )
    {
        m_albumResult.insert( album, OnlyInA );
    }
    for( const Meta::AlbumKey &album : albumsOnlyInB )
    {
        m_albumResult.insert( album, OnlyInB );
    }
    for( const Meta::AlbumKey &album : albumsInBoth )
    {
        m_albumResult.insert( album, InBoth );
    }
    Collections::QueryMaker *qmA = setupTrackQuery( m_collectionA );
    Collections::QueryMaker *qmB = setupTrackQuery( m_collectionB );
    qmA->beginAnd();
    qmB->beginAnd();
    {
        QHashIterator<QString, InSet> iter( m_artistResult );
        while( iter.hasNext() )
        {
            iter.next();
            QString artist = iter.key();
            InSet currentStatus = iter.value();
            if( currentStatus == OnlyInA )
            {
                qmA->excludeFilter( Meta::valArtist, artist, true, true );
            }
            if( currentStatus == OnlyInB )
            {
                qmB->excludeFilter( Meta::valArtist, artist, true, true );
            }
        }
    }
    {
        QHashIterator<Meta::AlbumKey, InSet> iter( m_albumResult );
        while( iter.hasNext() )
        {
            iter.next();
            Meta::AlbumKey album = iter.key();
            InSet currentStatus = iter.value();
            if( currentStatus == OnlyInA )
            {
                qmA->beginOr();
                qmA->excludeFilter( Meta::valAlbum, album.albumName(), true, true );
                qmA->excludeFilter( Meta::valAlbumArtist, album.artistName(), true, true );
                qmA->endAndOr();
            }
            if( currentStatus == OnlyInB )
            {
                qmB->beginOr();
                qmB->excludeFilter( Meta::valAlbum, album.albumName(), true, true );
                qmB->excludeFilter( Meta::valAlbumArtist, album.artistName(), true, true );
                qmB->endAndOr();
            }
        }
    }
    qmA->endAndOr();
    qmB->endAndOr();
    m_timer.start();
    qmA->run();
    qmB->run();
}

void
SynchronizationBaseJob::handleTrackResult()
{
    DEBUG_BLOCK
    QSet<Meta::TrackKey> tracksOnlyInA = m_tracksA - m_tracksB;
    QSet<Meta::TrackKey> tracksOnlyInB = m_tracksB - m_tracksA;

    for( const Meta::TrackKey &key : tracksOnlyInA )
    {
        m_trackResultOnlyInA << m_keyToTrackA.value( key );
    }
    for( const Meta::TrackKey &key : tracksOnlyInB )
    {
        m_trackResultOnlyInB << m_keyToTrackB.value( key );
    }

    //we have to make sure that we do not start queries that will return *all* tracks of the collection
    //because we did not add any filter to it
    bool haveToStartQueryA = false;
    bool haveToStartQueryB = false;

    //we do not care about tracks in both collections
    Collections::QueryMaker *qmA = createQueryMaker( m_collectionA );
    Collections::QueryMaker *qmB = createQueryMaker( m_collectionB );
    qmA->setQueryType( Collections::QueryMaker::Track );
    qmB->setQueryType( Collections::QueryMaker::Track );
    qmA->beginOr();
    qmB->beginOr();
    {
        QHashIterator<QString, InSet> iter( m_artistResult );
        while( iter.hasNext() )
        {
            iter.next();
            QString artist = iter.key();
            InSet currentStatus = iter.value();
            if( currentStatus == OnlyInA )
            {
                qmA->addFilter( Meta::valArtist, artist, true, true );
                haveToStartQueryA = true;
            }
            if( currentStatus == OnlyInB )
            {
                qmB->addFilter( Meta::valArtist, artist, true, true );
                haveToStartQueryB = true;
            }
        }
    }
    {
        QHashIterator<Meta::AlbumKey, InSet> iter( m_albumResult );
        while( iter.hasNext() )
        {
            iter.next();
            Meta::AlbumKey album = iter.key();
            InSet currentStatus = iter.value();
            if( currentStatus == OnlyInA )
            {
                qmA->beginAnd();
                qmA->addFilter( Meta::valAlbum, album.albumName(), true, true );
                qmA->addFilter( Meta::valAlbumArtist, album.artistName(), true, true );
                qmA->endAndOr();
                haveToStartQueryA = true;
            }
            if( currentStatus == OnlyInB )
            {
                qmB->beginAnd();
                qmB->addFilter( Meta::valAlbum, album.albumName(), true, true );
                qmB->addFilter( Meta::valAlbumArtist, album.artistName(), true, true );
                qmB->endAndOr();
                haveToStartQueryB = true;
            }
        }
    }
    qmA->endAndOr();
    qmB->endAndOr();
    connect( qmA, &Collections::QueryMaker::newTracksReady, this, &SynchronizationBaseJob::slotSyncTracks );
    connect( qmB, &Collections::QueryMaker::newTracksReady, this, &SynchronizationBaseJob::slotSyncTracks );
    connect( qmA, &Collections::QueryMaker::queryDone, this, &SynchronizationBaseJob::slotSyncQueryDone );
    connect( qmB, &Collections::QueryMaker::queryDone, this, &SynchronizationBaseJob::slotSyncQueryDone );
    m_timer.start();
    if( haveToStartQueryA )
    {
        qmA->run();
    }
    else
    {
        delete qmA;
        m_currentResultCount += 1;
    }
    if( haveToStartQueryB )
    {
        qmB->run();
    }
    else
    {
        delete qmB;
        m_currentResultCount += 1;
    }
    if( !( haveToStartQueryA || haveToStartQueryB ) )
    {
        slotSyncQueryDone();
    }
}

void
SynchronizationBaseJob::slotSyncTracks( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    Collections::Collection *senderColl = m_queryMakers.value( qobject_cast<Collections::QueryMaker*>(sender()) );
    if( senderColl == m_collectionA )
    {
        m_trackResultOnlyInA << tracks;
    }
    else if( senderColl == m_collectionB )
    {
        m_trackResultOnlyInB << tracks;
    }
    else
    {
        //huh?
        debug() << "received data from unknown collection";
    }
}

void
SynchronizationBaseJob::slotSyncQueryDone()
{
    DEBUG_BLOCK;
    m_currentResultCount += 1;
    if( m_currentResultCount < 2 )
        return;
    m_currentResultCount = 0;

    m_timer.stop();
    if( m_state == Syncing )
    {
        doSynchronization( m_trackResultOnlyInA, OnlyInA, m_collectionA, m_collectionB );
        doSynchronization( m_trackResultOnlyInB, OnlyInB, m_collectionA, m_collectionB );
        deleteLater();
    }
    else
    {
        const QMetaObject *mo = metaObject();
        QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "State" ) );
        debug() << "detected state " << me.valueToKey( m_state ) << " in slotSyncQueryDone(), do not know how to handle this. Aborting";
        deleteLater();
    }
}
