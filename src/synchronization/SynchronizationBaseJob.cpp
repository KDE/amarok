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

#include "collection/Collection.h"
#include "collection/QueryMaker.h"
#include "Debug.h"
#include "meta/MetaConstants.h"

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
        , m_collectionA( 0 )
        , m_collectionB( 0 )
{
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( timeout() ) );
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
SynchronizationBaseJob::setCollectionA( Amarok::Collection *collection )
{
    m_collectionA = collection;
}

void
SynchronizationBaseJob::setCollectionB( Amarok::Collection *collection )
{
    m_collectionB = collection;
}

void
SynchronizationBaseJob::setFilter( const QString &filter )
{
    Q_UNUSED( filter )
}

QueryMaker*
SynchronizationBaseJob::createQueryMaker( Amarok::Collection *collection )
{
    //TODO: apply filters. This allows us to only sync a subset of a collection
    QueryMaker *qm = collection->queryMaker();
    qm->setAutoDelete( true );
    return qm;
}

void
SynchronizationBaseJob::synchronize()
{
    DEBUG_BLOCK
    if( !m_collectionA || !m_collectionB )
    {
        debug() << "aborting synchronization, at least one collecton is missing";
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
    DEBUG_BLOCK
    if( m_currentResultCount != 2 )
    {
        return;
    }

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

QueryMaker*
SynchronizationBaseJob::setupArtistQuery( Amarok::Collection *coll )
{
    QueryMaker *qm = createQueryMaker( coll );
    qm->setQueryType( QueryMaker::Artist );
    connect( qm, SIGNAL( queryDone() ), this, SLOT( slotQueryDone() ), Qt::QueuedConnection );
    connect( qm, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), this, SLOT( slotResultReady(QString,Meta::ArtistList) ), Qt::QueuedConnection );
    return qm;
}

QueryMaker*
SynchronizationBaseJob::setupAlbumQuery( Amarok::Collection *coll )
{
    QueryMaker *qm = createQueryMaker( coll );
    qm->setQueryType( QueryMaker::Album );
    connect( qm, SIGNAL( queryDone() ), this, SLOT( slotQueryDone() ), Qt::QueuedConnection );
    connect( qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), this, SLOT( slotResultReady(QString,Meta::AlbumList) ), Qt::QueuedConnection );
    return qm;
}

QueryMaker*
SynchronizationBaseJob::setupTrackQuery( Amarok::Collection *coll )
{
    QueryMaker *qm = createQueryMaker( coll );
    qm->setQueryType( QueryMaker::Track );
    connect( qm, SIGNAL( queryDone() ), this, SLOT( slotQueryDone() ), Qt::QueuedConnection );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( slotResultReady(QString,Meta::TrackList) ), Qt::QueuedConnection );
    return qm;
}

void
SynchronizationBaseJob::slotResultReady( const QString &id, const Meta::ArtistList &artists )
{
    DEBUG_BLOCK
    QSet<QString> artistSet;
    foreach( const Meta::ArtistPtr &artist, artists )
    {
        if( artist )
            artistSet.insert( artist->name() );
    }
    if( id == m_collectionA->collectionId() )
    {
        m_currentResultCount += 1;
        m_artistsA = artistSet;
    }
    else if( id == m_collectionB->collectionId() )
    {
        m_currentResultCount += 1;
        m_artistsB = artistSet;
    }
    else
    {
        //huh? how did we get here?
    }
}

void
SynchronizationBaseJob::slotResultReady( const QString &id, const Meta::AlbumList &albums )
{
    DEBUG_BLOCK
    QSet<AlbumKey> albumSet;
    foreach( const Meta::AlbumPtr &albumPtr, albums )
    {
        QString album;
        QString albumArtist;
        if( albumPtr )
            album = albumPtr->name();
        if( albumPtr && albumPtr->hasAlbumArtist() )
            albumArtist = albumPtr->albumArtist()->name();

        albumSet.insert( AlbumKey( albumArtist, album ) );
    }
    if( id == m_collectionA->collectionId() )
    {
        m_currentResultCount += 1;
        m_albumsA = albumSet;
    }
    else if( id == m_collectionB->collectionId() )
    {
        m_currentResultCount += 1;
        m_albumsB = albumSet;
    }
    else
    {
        //huh? how did we get here?
    }
}

void
SynchronizationBaseJob::slotResultReady( const QString &id, const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    if( id == m_collectionA->collectionId() )
    {

        foreach( const Meta::TrackPtr &track, tracks )
        {
            TrackKey key = Meta::keyFromTrack( track );
            m_tracksA.insert( key );
            m_keyToTrackA.insert( key, track );
        }
        m_currentResultCount += 1;
    }
    else if( id == m_collectionB->collectionId() )
    {
        foreach( const Meta::TrackPtr &track, tracks )
        {
            TrackKey key = Meta::keyFromTrack( track );
            m_tracksB.insert( key );
            m_keyToTrackB.insert( key, track );
        }
        m_currentResultCount += 1;
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
    foreach( const QString &artist, artistsOnlyInA )
    {
        m_artistResult.insert( artist, OnlyInA );
    }
    foreach( const QString &artist, artistsOnlyInB )
    {
        m_artistResult.insert( artist, OnlyInB );
    }
    foreach( const QString &artist, artistsInBoth )
    {
        m_artistResult.insert( artist, InBoth );
    }
    QueryMaker *qmA = setupAlbumQuery( m_collectionA );
    QueryMaker *qmB = setupAlbumQuery( m_collectionB );
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
    QSet<AlbumKey> albumsOnlyInA = m_albumsA - m_albumsB;
    QSet<AlbumKey> albumsOnlyInB = m_albumsB - m_albumsA;
    QSet<AlbumKey> albumsInBoth = m_albumsA & m_albumsB;

    foreach( const AlbumKey &album, albumsOnlyInA )
    {
        m_albumResult.insert( album, OnlyInA );
    }
    foreach( const AlbumKey &album, albumsOnlyInB )
    {
        m_albumResult.insert( album, OnlyInB );
    }
    foreach( const AlbumKey &album, albumsInBoth )
    {
        m_albumResult.insert( album, InBoth );
    }
    QueryMaker *qmA = setupTrackQuery( m_collectionA );
    QueryMaker *qmB = setupTrackQuery( m_collectionB );
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
        QHashIterator<AlbumKey, InSet> iter( m_albumResult );
        while( iter.hasNext() )
        {
            iter.next();
            AlbumKey album = iter.key();
            InSet currentStatus = iter.value();
            if( currentStatus == OnlyInA )
            {
                qmA->beginOr();
                qmA->excludeFilter( Meta::valAlbum, album.albumName, true, true );
                qmA->excludeFilter( Meta::valAlbumArtist, album.artistName, true, true );
                qmA->endAndOr();
            }
            if( currentStatus == OnlyInB )
            {
                qmB->beginOr();
                qmB->excludeFilter( Meta::valAlbum, album.albumName, true, true );
                qmB->excludeFilter( Meta::valAlbumArtist, album.artistName, true, true );
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
    QSet<TrackKey> tracksOnlyInA = m_tracksA - m_tracksB;
    QSet<TrackKey> tracksOnlyInB = m_tracksB - m_tracksA;

    foreach( const TrackKey &key, tracksOnlyInA )
    {
        m_trackResultOnlyInA << m_keyToTrackA.value( key );
    }
    foreach( const TrackKey &key, tracksOnlyInB )
    {
        m_trackResultOnlyInB << m_keyToTrackB.value( key );
    }

    //we have to make sure that we do not start queries that will return *all* tracks of the collection
    //because we did not add any filter to it
    bool haveToStartQueryA = false;
    bool haveToStartQueryB = false;

    //we do not care about tracks in both collections
    QueryMaker *qmA = createQueryMaker( m_collectionA );
    QueryMaker *qmB = createQueryMaker( m_collectionB );
    qmA->setQueryType( QueryMaker::Track );
    qmB->setQueryType( QueryMaker::Track );
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
        QHashIterator<AlbumKey, InSet> iter( m_albumResult );
        while( iter.hasNext() )
        {
            iter.next();
            AlbumKey album = iter.key();
            InSet currentStatus = iter.value();
            if( currentStatus == OnlyInA )
            {
                qmA->beginAnd();
                qmA->addFilter( Meta::valAlbum, album.albumName, true, true );
                qmA->addFilter( Meta::valAlbumArtist, album.artistName, true, true );
                qmA->endAndOr();
                haveToStartQueryA = true;
            }
            if( currentStatus == OnlyInB )
            {
                qmB->beginAnd();
                qmB->addFilter( Meta::valAlbum, album.albumName, true, true );
                qmB->addFilter( Meta::valAlbumArtist, album.artistName, true, true );
                qmB->endAndOr();
                haveToStartQueryB = true;
            }
        }
    }
    qmA->endAndOr();
    qmB->endAndOr();
    connect( qmA, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( slotSyncTracks( QString, Meta::TrackList ) ) );
    connect( qmB, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( slotSyncTracks( QString, Meta::TrackList ) ) );
    connect( qmA, SIGNAL( queryDone() ), this, SLOT( slotSyncQueryDone() ) );
    connect( qmB, SIGNAL( queryDone() ), this, SLOT( slotSyncQueryDone() ) );
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
SynchronizationBaseJob::slotSyncTracks( const QString &id, const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    if( id == m_collectionA->collectionId() )
    {
        m_currentResultCount += 1;
        m_trackResultOnlyInA << tracks;
    }
    else if( id == m_collectionB->collectionId() )
    {
        m_currentResultCount += 1;
        m_trackResultOnlyInB << tracks;
    }
    else
    {
        //huh?
        debug() << "received data from unknown collection: " << id;
    }
}

void
SynchronizationBaseJob::slotSyncQueryDone()
{
    DEBUG_BLOCK
    if( m_currentResultCount != 2 )
    {
        return;
    }

    m_currentResultCount = 0;
    m_timer.stop();
    if( m_state == Syncing )
    {
        doSynchronization( m_trackResultOnlyInA, OnlyInA, m_collectionA, m_collectionB );
        doSynchronization( m_trackResultOnlyInB, OnlyInB, m_collectionA, m_collectionB );
    }
    else
    {
        const QMetaObject *mo = metaObject();
        QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "State" ) );
        debug() << "detected state " << me.valueToKey( m_state ) << " in slotSyncQueryDone(), do not know how to handle this. Aborting";
        deleteLater();
    }
}
