/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#include "blockingquery.h"

#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <QWaitCondition>

struct BlockingQuery::Private
{
    QueryMaker *qm;
    QMutex mutex;
    QWaitCondition wait;
    QStringList collectionIds;
    QMutex dataMutex;
    QHash<QString, DataList> data;
    QHash<QString, TrackList> track;
    QHash<QString, AlbumList> album;
    QHash<QString, ArtistList> artist;
    QHash<QString, GenreList> genre;
    QHash<QString, ComposerList> composer;
    QHash<QString, YearList> year;
    QHash<QString, QStringList> custom;
};

BlockingQuery::BlockingQuery( QueryMaker *qm )
    : QObject()
    , d( new Private )
{
    d->qm = qm;
}

BlockingQuery::~BlockingQuery()
{
    delete d->qm;
    delete d;
}

void
BlockingQuery::startQuery()
{
    connect( d->qm, SIGNAL( newResultReady( QString, DataList ) ), SLOT( result( QString, DataList ) ), Qt::DirectConnection );
    connect( d->qm, SIGNAL( newResultReady( QString, TrackList ) ), SLOT( result( QString, TrackList ) ), Qt::DirectConnection );
    connect( d->qm, SIGNAL( newResultReady( QString, AlbumList ) ), SLOT( result( QString, AlbumList ) ), Qt::DirectConnection );
    connect( d->qm, SIGNAL( newResultReady( QString, ArtistList ) ), SLOT( result( QString, ArtistList ) ), Qt::DirectConnection );
    connect( d->qm, SIGNAL( newResultReady( QString, ComposerList ) ), SLOT( result( QString, ComposerList ) ), Qt::DirectConnection );
    connect( d->qm, SIGNAL( newResultReady( QString, YearList ) ), SLOT( result( QString, YearList ) ), Qt::DirectConnection );
    connect( d->qm, SIGNAL( newResultReady( QString, QStringList ) ), SLOT( result( QString, QStringList ) ), Qt::DirectConnection );
    connect( d->qm, SIGNAL( queryDone() ), SLOT( queryDone() ), Qt::DirectConnection );

    d->mutex.lock();
    d->qm->run();
    d->wait.wait( &d->mutex );
    d->mutex.unlock();
}

QStringList
BlockingQuery::collectionIds()
{
    QMutexLocker locker( &d->dataMutex );
    return d->collectionIds;
}

DataList
BlockingQuery::data( const QString &id )
{
    QMutexLocker locker( &d->dataMutex );
    return d->data.value( id );
}

TrackList
BlockingQuery::tracks( const QString &id )
{
    QMutexLocker locker( &d->dataMutex );
    return d->track.value( id );
}

AlbumList
BlockingQuery::albums( const QString &id )
{
    QMutexLocker locker( &d->dataMutex );
    return d->album.value( id );
}

ArtistList
BlockingQuery::artists( const QString &id )
{
    QMutexLocker locker( &d->dataMutex );
    return d->artist.value( id );
}

GenreList
BlockingQuery::genres( const QString &id )
{
    QMutexLocker locker( &d->dataMutex );
    return d->genre.value( id );
}

ComposerList
BlockingQuery::composers( const QString &id )
{
    QMutexLocker locker( &d->dataMutex );
    return d->composer.value( id );
}

YearList
BlockingQuery::years( const QString &id )
{
    QMutexLocker locker( &d->dataMutex );
    return d->year.value( id );
}

QStringList
BlockingQuery::customData( const QString &id )
{
    QMutexLocker locker( &d->dataMutex );
    return d->custom.value( id );
}

QHash<QString, DataList>
BlockingQuery::data()
{
    QMutexLocker locker( &d->dataMutex );
    return d->data;
}

QHash<QString, TrackList>
BlockingQuery::tracks()
{
    QMutexLocker locker( &d->dataMutex );
    return d->track;
}

QHash<QString, AlbumList>
BlockingQuery::albums()
{
    QMutexLocker locker( &d->dataMutex );
    return d->album;
}

QHash<QString, ArtistList>
BlockingQuery::artists()
{
    QMutexLocker locker( &d->dataMutex );
    return d->artist;
}

QHash<QString, GenreList>
BlockingQuery::genres()
{
    QMutexLocker locker( &d->dataMutex );
    return d->genre;
}

QHash<QString, ComposerList>
BlockingQuery::composers()
{
    QMutexLocker locker( &d->dataMutex );
    return d->composer;
}

QHash<QString, YearList>
BlockingQuery::years()
{
    QMutexLocker locker( &d->dataMutex );
    return d->year;
}

QHash<QString, QStringList>
BlockingQuery::customData()
{
    QMutexLocker locker( &d->dataMutex );
    return d->custom;
}

void
BlockingQuery::queryDone()
{
    //lock the mutex so we can be sure that we wait in run()
    d->mutex.lock();
    d->mutex.unlock();
    d->wait.wakeOne();
}

void
BlockingQuery::result( const QString &collectionId, DataList data )
{
    d->dataMutex.lock();
    d->collectionIds.append( collectionId );
    d->data.insert( collectionId, data );
    d->dataMutex.unlock();
}

void
BlockingQuery::result( const QString &collectionId, TrackList tracks )
{
    d->dataMutex.lock();
    d->collectionIds.append( collectionId );
    d->track.insert( collectionId, tracks );
    d->dataMutex.unlock();
}

void
BlockingQuery::result( const QString &collectionId, ArtistList artists )
{
    d->dataMutex.lock();
    d->collectionIds.append( collectionId );
    d->artist.insert( collectionId, artists );
    d->dataMutex.unlock();
}

void
BlockingQuery::result( const QString &collectionId, AlbumList albums )
{
    d->dataMutex.lock();
    d->collectionIds.append( collectionId );
    d->album.insert( collectionId, albums );
    d->dataMutex.unlock();
}

void
BlockingQuery::result( const QString &collectionId, GenreList genres )
{
    d->dataMutex.lock();
    d->collectionIds.append( collectionId );
    d->genre.insert( collectionId, genres );
    d->dataMutex.unlock();
}

void
BlockingQuery::result( const QString &collectionId, ComposerList composers )
{
    d->dataMutex.lock();
    d->collectionIds.append( collectionId );
    d->composer.insert( collectionId, composers );
    d->dataMutex.unlock();
}

void
BlockingQuery::result( const QString &collectionId, YearList years )
{
    d->dataMutex.lock();
    d->collectionIds.append( collectionId );
    d->year.insert( collectionId, years );
    d->dataMutex.unlock();
}

void
BlockingQuery::result( const QString &collectionId, const QStringList &list )
{
    d->dataMutex.lock();
    d->collectionIds.append( collectionId );
    d->custom.insert( collectionId, list );
    d->dataMutex.unlock();
}

#include "blockingquery.moc"
