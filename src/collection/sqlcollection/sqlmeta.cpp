/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "sqlmeta.h"

#include "amarok.h"
#include "blockingquery.h"
#include "sqlregistry.h"
#include "sqlcollection.h"

#include "mountpointmanager.h"

#include <QDateTime>
#include <QFile>
#include <QListIterator>
#include <QMutexLocker>

#include <klocale.h>

struct SqlTrack::MetaCache
{
    QString title;
    QString artist;
    QString album;
    QString composer;
    QString genre;
    QString year;
    QString comment;
    double score;
    int rating;
    int trackNumber;
    int discNumber;
};

SqlTrack::SqlTrack( SqlCollection* collection, const QStringList &result )
    : Track()
    , m_collection( collection )
    , m_batchUpdate( false )
    , m_cache( 0 )
{
    m_deviceid = result[0].toInt();
    m_rpath = result[1];
    m_url = KUrl( MountPointManager::instance()->getAbsolutePath( m_deviceid, m_rpath ) );
    m_title = result[2];
    m_comment = result[3];
    m_trackNumber = result[4].toInt();
    m_discNumber = result[5].toInt();
    m_score = result[6].toDouble();
    m_rating = result[7].toInt();
    m_bitrate = result[8].toInt();
    m_length = result[9].toInt();
    m_filesize = result[10].toInt();
    m_sampleRate = result[11].toInt();
    m_firstPlayed = result[12].toInt();
    m_lastPlayed = result[13].toUInt();
    m_playCount = result[14].toInt();
    //file type
    //BPM

    SqlRegistry* registry = m_collection->registry();
    m_artist = registry->getArtist( result[17], result[18].toInt() );
    m_album = registry->getAlbum( result[19], result[20].toInt() );
    //isCompilation
    m_genre = registry->getGenre( result[21], result[22].toInt() );
    m_composer = registry->getComposer( result[24], result[25].toInt() );
    m_year = registry->getYear( result[26], result[27].toInt() );
}

bool
SqlTrack::isPlayable() const
{
    return QFile::exists( m_url.path() );
}

bool
SqlTrack::isEditable() const
{
    return QFile::exists( m_url.path() ) && QFile::permissions( m_url.path() ) & QFile::WriteUser;
}

QString
SqlTrack::type() const
{
    return m_url.isLocalFile()
           ? m_url.fileName().mid( m_url.fileName().lastIndexOf( '.' ) + 1 )
           : i18n( "Stream" );
}

QString
SqlTrack::fullPrettyName() const
{
    QString s = m_artist->name();

    //FIXME doesn't work for resume playback

    if( s.isEmpty() )
        s = name();
    else
        s = i18n("%1 - %2", m_artist->name(), name() );

    //TODO
    if( s.isEmpty() ) s = prettyTitle( m_url.fileName() );

    return s;
}

QString
SqlTrack::prettyTitle( const QString &filename ) //static
{
    QString s = filename; //just so the code is more readable

    //remove .part extension if it exists
    if (s.endsWith( ".part" ))
        s = s.left( s.length() - 5 );

    //remove file extension, s/_/ /g and decode %2f-like sequences
    s = s.left( s.lastIndexOf( '.' ) ).replace( '_', ' ' );
    s = KUrl::fromPercentEncoding( s.toAscii() );

    return s;
}


QString
SqlTrack::prettyName() const
{
    //FIXME: This should handle cases when name() is empty!
    return name();
}

void
SqlTrack::setArtist( const QString &newArtist )
{
    if( m_batchUpdate )
        m_cache->artist = newArtist;
    else
    {
        //invalidate cache of the old artist...
        KSharedPtr<SqlArtist>::staticCast( m_artist )->invalidateCache();
        m_artist = m_collection->registry()->getArtist( newArtist );
        //and the new one
        KSharedPtr<SqlArtist>::staticCast( m_artist )->invalidateCache();
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setGenre( const QString &newGenre )
{
    if( m_batchUpdate )
        m_cache->genre = newGenre;
    else
    {
        KSharedPtr<SqlGenre>::staticCast( m_genre )->invalidateCache();
        m_genre = m_collection->registry()->getGenre( newGenre );
        KSharedPtr<SqlGenre>::staticCast( m_genre )->invalidateCache();
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setComposer( const QString &newComposer )
{
    if( m_batchUpdate )
        m_cache->composer = newComposer;
    else
    {
        KSharedPtr<SqlComposer>::staticCast( m_composer )->invalidateCache();
        m_composer = m_collection->registry()->getComposer( newComposer );
        KSharedPtr<SqlComposer>::staticCast( m_composer )->invalidateCache();
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setYear( const QString &newYear )
{
    if( m_batchUpdate )
        m_cache->year = newYear;
    else
    {
        KSharedPtr<SqlYear>::staticCast( m_year )->invalidateCache();
        m_year = m_collection->registry()->getYear( newYear );
        KSharedPtr<SqlYear>::staticCast( m_year )->invalidateCache();
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setAlbum( const QString &newAlbum )
{
    if( m_batchUpdate )
        m_cache->album = newAlbum;
    else
    {
        KSharedPtr<SqlAlbum>::staticCast( m_album )->invalidateCache();
        m_album = m_collection->registry()->getAlbum( newAlbum );
        KSharedPtr<SqlAlbum>::staticCast( m_album )->invalidateCache();
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::subscribe( TrackObserver *observer )
{
    if( !m_observers.contains( observer ) )
        m_observers.append( observer );
}

void
SqlTrack::unsubscribe( TrackObserver *observer )
{
    m_observers.removeAll( observer );
}

void
SqlTrack::notifyObservers()
{
    for( QListIterator<TrackObserver*> iter( m_observers ) ; iter.hasNext(); )
        iter.next()->metadataChanged( this );
}

void
SqlTrack::setScore( double newScore )
{
    if( m_batchUpdate )
        m_cache->score = newScore;
    else
    {
        m_score = newScore;
        updateStatisticsInDb();
        notifyObservers();
    }
}

void
SqlTrack::setRating( int newRating )
{
    if( m_batchUpdate )
        m_cache->rating = newRating;
    else
    {
        m_rating = newRating;
        updateStatisticsInDb();
        notifyObservers();
    }
}

void
SqlTrack::setTrackNumber( int newTrackNumber )
{
    if( m_batchUpdate )
        m_cache->trackNumber = newTrackNumber;
    else
    {
        m_trackNumber= newTrackNumber;
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setDiscNumber( int newDiscNumber )
{
    if( m_batchUpdate )
        m_cache->discNumber = newDiscNumber;
    else
    {
        m_discNumber = newDiscNumber;
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setComment( const QString &newComment )
{
    if( !m_batchUpdate )
    {
        m_comment = newComment;
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
    else
        m_cache->comment = newComment;
}

void
SqlTrack::beginMetaDataUpdate()
{
    m_batchUpdate = true;
    m_cache = new MetaCache;
    //init cache with current values
    m_cache->title = m_title;
    m_cache->artist = m_artist->name();
    m_cache->album = m_album->name();
    m_cache->composer = m_composer->name();
    m_cache->genre = m_genre->name();
    m_cache->year = m_year->name();
    m_cache->comment = m_comment;
    m_cache->score = m_score;
    m_cache->rating = m_rating;
    m_cache->trackNumber = m_trackNumber;
    m_cache->discNumber = m_discNumber;
}

void
SqlTrack::endMetaDataUpdate()
{
    commitMetaDataChanges();
    m_batchUpdate = false;
    delete m_cache;
    notifyObservers();
}

void
SqlTrack::abortMetaDataUpdate()
{
    //TODO method stub
    m_batchUpdate = false;
    delete m_cache;
}


void
SqlTrack::writeMetaDataToFile()
{
    //TODO method stub
}

void
SqlTrack::commitMetaDataChanges()
{
    if( m_cache )
    {
        m_title = m_cache->title;
        m_comment = m_cache->comment;
        m_score = m_cache->score;
        m_rating = m_cache->rating;
        m_trackNumber = m_cache->trackNumber;
        m_discNumber = m_cache->discNumber;
        m_artist = m_collection->registry()->getArtist( m_cache->artist );
        m_album = m_collection->registry()->getAlbum( m_cache->album );
        m_composer = m_collection->registry()->getComposer( m_cache->composer );
        m_genre = m_collection->registry()->getGenre( m_cache->genre );
        m_year = m_collection->registry()->getYear( m_cache->year );
        writeMetaDataToDb();
        writeMetaDataToFile();
        updateStatisticsInDb();
    }
}

void
SqlTrack::writeMetaDataToDb()
{
    QString update = "UPDATE tags SET %1 WHERE deviceid = %2 AND url ='%3';";
    QString tags = "title='%1',comment='%2',tracknumber=%3,discnumber=%4, artist=%5,album=%6,genre=%7,composer=%8,year=%9";
    QString artist = QString::number( KSharedPtr<SqlArtist>::staticCast( m_artist )->id() );
    QString album = QString::number( KSharedPtr<SqlAlbum>::staticCast( m_album )->id() );
    QString genre = QString::number( KSharedPtr<SqlGenre>::staticCast( m_genre )->id() );
    QString composer = QString::number( KSharedPtr<SqlComposer>::staticCast( m_composer )->id() );
    QString year = QString::number( KSharedPtr<SqlYear>::staticCast( m_year )->id() );
    tags.arg( m_collection->escape( m_title ), m_collection->escape( m_comment ),
              QString::number( m_trackNumber ), QString::number( m_discNumber ),
              artist, album, genre, composer, year );
    update.arg( tags, QString::number( m_deviceid ), m_collection->escape( m_rpath ) );
    m_collection->query( update );
}

void
SqlTrack::updateStatisticsInDb()
{
    QString update = "UPDATE statistics SET %1 WHERE deviceid = %2 AND url ='%3';";
    QString data = "rating=%1, percentage=%2, playcounter=%3, accessdate=%4, createdate=%5";
    data.arg( m_rating );
    data.arg( m_score );
    data.arg( m_playCount );
    data.arg( m_lastPlayed );
    data.arg( m_firstPlayed );
    update.arg( data, QString::number( m_deviceid ), m_collection->escape( m_rpath ) );
    m_collection->query( update );
}

void
SqlTrack::finishedPlaying( double playedFraction )
{
    m_lastPlayed = QDateTime::currentDateTime().toTime_t();
    m_playCount++;
    //TODO get new rating
    QString update = "UPDATE statistics SET playcounter = %1, accessdate = %2, rating = %3 "
                     "WHERE deviceid = %4 AND url = '%5';";
    update = update.arg( m_playCount ).arg( m_lastPlayed ).arg( m_rating ).arg( m_deviceid );
    update = update.arg( m_collection->escape( m_rpath ) );
    m_collection->query( update );
}


//---------------------- class Artist --------------------------

SqlArtist::SqlArtist( SqlCollection* collection, int id, const QString &name ) : Artist()
    ,m_collection( collection )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
{
    //nothing to do (yet)
}

void
SqlArtist::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
SqlArtist::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->startTrackQuery();
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
}


QString
SqlArtist::sortableName() const
{
    if ( m_modifiedName.isEmpty() && !m_name.isEmpty() ) {
        if ( m_name.startsWith( "the ", Qt::CaseInsensitive ) ) {
            QString begin = m_name.left( 3 );
            m_modifiedName = QString( "%1, %2" ).arg( m_name, begin );
            m_modifiedName = m_modifiedName.mid( 4 );
        }
        else {
            m_modifiedName = m_name;
        }
    }
    return m_modifiedName;
}

/*void
SqlArtist::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
}*/

//---------------SqlAlbum---------------------------------

SqlAlbum::SqlAlbum( SqlCollection* collection, int id, const QString &name ) : Album()
    ,m_collection( collection )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
{
    //nothing to do
}

void
SqlAlbum::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
SqlAlbum::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->startTrackQuery();
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlComposer---------------------------------

SqlComposer::SqlComposer( SqlCollection* collection, int id, const QString &name ) : Composer()
    ,m_collection( collection )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
{
    //nothing to do
}

void
SqlComposer::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
SqlComposer::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->startTrackQuery();
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlGenre---------------------------------

SqlGenre::SqlGenre( SqlCollection* collection, int id, const QString &name ) : Genre()
    ,m_collection( collection )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
{
    //nothing to do
}

void
SqlGenre::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
SqlGenre::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->startTrackQuery();
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlYear---------------------------------

SqlYear::SqlYear( SqlCollection* collection, int id, const QString &name ) : Year()
    ,m_collection( collection )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
{
    //nothing to do
}

void
SqlYear::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
SqlYear::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->startTrackQuery();
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
}

