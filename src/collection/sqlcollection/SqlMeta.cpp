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

#include "SqlMeta.h"

#include "Amarok.h"
#include "BlockingQuery.h"
#include "Debug.h"
#include "MetaUtility.h"
#include "SqlCollection.h"
#include "SqlRegistry.h"
#include "browsers/servicebrowser/lastfm/SimilarArtistsAction.h"
#include "context/popupdropper/PopupDropperAction.h"
#include "covermanager/CoverFetcher.h"
#include "covermanager/CoverFetchingActions.h"
#include "meta/CustomActionsCapability.h"
#include "meta/EditCapability.h"
#include "meta/OrganiseCapability.h"
#include "meta/UpdateCapability.h"
#include "mountpointmanager.h"
//#include "ScriptManager.h"
//#include "mediadevice/CopyToDeviceAction.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QListIterator>
#include <QMutexLocker>
#include <QPointer>

#include <KAction>
#include <kcodecs.h>
#include <KFileMetaInfo>
#include <klocale.h>
#include <KSharedPtr>

using namespace Meta;

class EditCapabilityImpl : public Meta::EditCapability
{
    Q_OBJECT
    public:
        EditCapabilityImpl( SqlTrack *track )
            : Meta::EditCapability()
            , m_track( track ) {}

        virtual bool isEditable() const { return m_track->isEditable(); }
        virtual void setAlbum( const QString &newAlbum ) { m_track->setAlbum( newAlbum ); }
        virtual void setArtist( const QString &newArtist ) { m_track->setArtist( newArtist ); }
        virtual void setComposer( const QString &newComposer ) { m_track->setComposer( newComposer ); }
        virtual void setGenre( const QString &newGenre ) { m_track->setGenre( newGenre ); }
        virtual void setYear( const QString &newYear ) { m_track->setYear( newYear ); }
        virtual void setTitle( const QString &newTitle ) { m_track->setTitle( newTitle ); }
        virtual void setComment( const QString &newComment ) { m_track->setComment( newComment ); }
        virtual void setTrackNumber( int newTrackNumber ) { m_track->setTrackNumber( newTrackNumber ); }
        virtual void setDiscNumber( int newDiscNumber ) { m_track->setDiscNumber( newDiscNumber ); }
        virtual void beginMetaDataUpdate() { m_track->beginMetaDataUpdate(); }
        virtual void endMetaDataUpdate() { m_track->endMetaDataUpdate(); }
        virtual void abortMetaDataUpdate() { m_track->abortMetaDataUpdate(); }

    private:
        KSharedPtr<SqlTrack> m_track;
};

class OrganiseCapabilityImpl : public Meta::OrganiseCapability
{
    Q_OBJECT
    public:
        OrganiseCapabilityImpl( SqlTrack *track )
            : Meta::OrganiseCapability()
            , m_track( track ) {}

        virtual void deleteTrack()
        {
            if( QFile::remove( m_track->playableUrl().path() ) )
            {
                QString sql = QString( "DELETE FROM tracks WHERE id = %1;" ).arg( m_track->trackId() );
                m_track->sqlCollection()->query( sql );
            }
        }

    private:
        KSharedPtr<SqlTrack> m_track;
};

class UpdateCapabilitySql : public Meta::UpdateCapability
{
    Q_OBJECT
    public:
        UpdateCapabilitySql( SqlTrack *track )
    : Meta::UpdateCapability()
                , m_track( track ) {}

        virtual void collectionUpdated() const { m_track->collection()->collectionUpdated(); }


    private:
        KSharedPtr<SqlTrack> m_track;
};

QString
SqlTrack::getTrackReturnValues()
{
    //do not use any weird column names that contains commas: this will break getTrackReturnValuesCount()
    return "urls.deviceid, urls.rpath, urls.uniqueid, "
           "tracks.id, tracks.title, tracks.comment, "
           "tracks.tracknumber, tracks.discnumber, "
           "statistics.score, statistics.rating, "
           "tracks.bitrate, tracks.length, "
           "tracks.filesize, tracks.samplerate, "
           "statistics.createdate, statistics.accessdate, "
           "statistics.playcount, tracks.filetype, tracks.bpm, "
           "artists.name, artists.id, "
           "albums.name, albums.id, albums.artist, "
           "genres.name, genres.id, "
           "composers.name, composers.id, "
           "years.name, years.id";
}

int
SqlTrack::getTrackReturnValueCount()
{
    static int count = getTrackReturnValues().split( ',' ).count();
    return count;
}

TrackPtr
SqlTrack::getTrack( int deviceid, const QString &rpath, SqlCollection *collection )
{
    QString query = "SELECT %1 FROM urls "
                    "LEFT JOIN tracks ON urls.id = tracks.url "
                    "LEFT JOIN statistics ON urls.id = statistics.url "
                    "LEFT JOIN artists ON tracks.artist = artists.id "
                    "LEFT JOIN albums ON tracks.album = albums.id "
                    "LEFT JOIN genres ON tracks.genre = genres.id "
                    "LEFT JOIN composers ON tracks.composer = composers.id "
                    "LEFT JOIN years ON tracks.year = years.id "
                    "WHERE urls.deviceid = %2 AND urls.rpath = '%3';";
    query = query.arg( SqlTrack::getTrackReturnValues(), QString::number( deviceid ), collection->escape( rpath ) );
    QStringList result = collection->query( query );
    if( result.isEmpty() )
        return TrackPtr();
    else
        return TrackPtr( new SqlTrack( collection, result ) );
}

TrackPtr
SqlTrack::getTrackFromUid( const QString &uid, SqlCollection* collection )
{
    QString query = "SELECT urls.deviceid, urls.rpath FROM urls "
                    "WHERE urls.uniqueid = '%1';";
    query = query.arg( collection->escape( uid ) );
    QStringList result = collection->query( query );
    if( result.isEmpty() )
        return TrackPtr();
    else
        return getTrack( result[0].toInt(), result[1], collection );    
}

SqlTrack::SqlTrack( SqlCollection* collection, const QStringList &result )
    : Track()
    , m_collection( QPointer<SqlCollection>( collection ) )
    , m_batchUpdate( false )
{
    QStringList::ConstIterator iter = result.constBegin();
    m_deviceid = (*(iter++)).toInt();
    m_rpath = *(iter++);
    m_uid = *(iter++);
    m_url = KUrl( MountPointManager::instance()->getAbsolutePath( m_deviceid, m_rpath ) );
    m_trackId = (*(iter++)).toInt();
    m_title = *(iter++);
    m_comment = *(iter++);
    m_trackNumber = (*(iter++)).toInt();
    m_discNumber = (*(iter++)).toInt();
    m_score = (*(iter++)).toDouble();
    m_rating = (*(iter++)).toInt();
    m_bitrate = (*(iter++)).toInt();
    m_length = (*(iter++)).toInt();
    m_filesize = (*(iter++)).toInt();
    m_sampleRate = (*(iter++)).toInt();
    m_firstPlayed = (*(iter++)).toInt();
    m_lastPlayed = (*(iter++)).toUInt();
    m_playCount = (*(iter++)).toInt();
    ++iter; //file type
    ++iter; //BPM

    SqlRegistry* registry = m_collection->registry();
    QString artist = *(iter++);
    int artistId = (*(iter++)).toInt();
    m_artist = registry->getArtist( artist, artistId  );
    QString album = *(iter++);
    int albumId =(*(iter++)).toInt();
    int albumArtistId = (*(iter++)).toInt();
    m_album = registry->getAlbum( album, albumId, albumArtistId );
    QString genre = *(iter++);
    int genreId = (*(iter++)).toInt();
    m_genre = registry->getGenre( genre, genreId );
    QString composer = *(iter++);
    int composerId = (*(iter++)).toInt();
    m_composer = registry->getComposer( composer, composerId );
    QString year = *(iter++);
    int yearId = (*(iter++)).toInt();
    m_year = registry->getYear( year, yearId );
    Q_ASSERT_X( iter == result.constEnd(), "SqlTrack( SqlCollection*, QStringList )", "number of expected fields did not match number of actual fields" );
}

bool
SqlTrack::isPlayable() const
{
    //a song is not playable anymore if the collection was removed
    return m_collection && QFile::exists( m_url.path() );
}

bool
SqlTrack::isEditable() const
{
    return m_collection && QFile::exists( m_url.path() ) && QFile::permissions( m_url.path() ) & QFile::WriteUser;
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
    if ( !name().isEmpty() )
        return name();
    else
        return  prettyTitle( m_url.fileName() );
}

void
SqlTrack::setUrl( const QString &url )
{
    m_deviceid = MountPointManager::instance()->getIdForUrl( url );
    m_rpath = MountPointManager::instance()->getRelativePath( m_deviceid, url );
    if( m_batchUpdate )
        m_cache.insert( Meta::Field::URL, MountPointManager::instance()->getAbsolutePath( m_deviceid, m_rpath ) ); 
    else
    {
        m_url = url;
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setUrl( const int deviceid, const QString &rpath )
{
    m_deviceid = deviceid;
    m_rpath = rpath;
    if( m_batchUpdate )
        m_cache.insert( Meta::Field::URL, KUrl( MountPointManager::instance()->getAbsolutePath( deviceid, rpath ) ) );
    else
    {
        m_url = KUrl( MountPointManager::instance()->getAbsolutePath( m_deviceid, m_rpath ) );
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setArtist( const QString &newArtist )
{
    if( m_batchUpdate )
        m_cache.insert( Meta::Field::ARTIST, newArtist );
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
        m_cache.insert( Meta::Field::GENRE, newGenre );
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
        m_cache.insert( Meta::Field::COMPOSER, newComposer );
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
        m_cache.insert( Meta::Field::YEAR, newYear );
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
        m_cache.insert( Meta::Field::ALBUM, newAlbum );
    else
    {
        KSharedPtr<SqlAlbum>::staticCast( m_album )->invalidateCache();
        int id = -1;
        SqlArtist *artist = dynamic_cast<SqlArtist*>(m_artist.data());
        if( artist )
            id = artist->id();
        m_album = m_collection->registry()->getAlbum( newAlbum, -1, id );
        KSharedPtr<SqlAlbum>::staticCast( m_album )->invalidateCache();
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setScore( double newScore )
{
    if( m_batchUpdate )
        m_cache.insert( Meta::Field::SCORE, newScore );
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
        m_cache.insert( Meta::Field::RATING, newRating );
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
        m_cache.insert( Meta::Field::TRACKNUMBER, newTrackNumber );
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
        m_cache.insert( Meta::Field::DISCNUMBER, newDiscNumber );
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
    if( m_batchUpdate )
        m_cache.insert( Meta::Field::COMMENT, newComment );
    else
    {
        m_comment = newComment;
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setTitle( const QString &newTitle )
{
    if( m_batchUpdate )
        m_cache.insert( Meta::Field::TITLE, newTitle );
    else
    {
        m_title = newTitle;
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
}

void
SqlTrack::setUidUrl( const QString &uid )
{
    if( m_batchUpdate )
        m_cache.insert( Meta::Field::UNIQUEID, uid );
    else
    {
        m_uid = uid;
        notifyObservers();
    }
}

void
SqlTrack::beginMetaDataUpdate()
{
    m_batchUpdate = true;
    //init cache with current values
    /*m_cache->title = m_title;
    m_cache->artist = m_artist->name();
    m_cache->album = m_album->name();
    m_cache->composer = m_composer->name();
    m_cache->genre = m_genre->name();
    m_cache->year = m_year->name();
    m_cache->comment = m_comment;
    m_cache->score = m_score;
    m_cache->rating = m_rating;
    m_cache->trackNumber = m_trackNumber;
    m_cache->discNumber = m_discNumber;*/
}

void
SqlTrack::endMetaDataUpdate()
{
    commitMetaDataChanges();
    m_batchUpdate = false;
    m_cache.clear();
    notifyObservers();
}

void
SqlTrack::abortMetaDataUpdate()
{
    m_batchUpdate = false;
    m_cache.clear();
}


void
SqlTrack::writeMetaDataToFile()
{
    Meta::Field::writeFields( m_url.path(), m_cache );
    //updating the fields might have changed the filesize
    //read the current filesize so that we can update the db
    QFile file( m_url.path() );
    if( file.exists() )
    {
        m_filesize = file.size();
    }
}

void
SqlTrack::commitMetaDataChanges()
{
    if( m_batchUpdate )
    {
        if( m_cache.contains( Meta::Field::TITLE ) )
            m_title = m_cache.value( Meta::Field::TITLE ).toString();
        if( m_cache.contains( Meta::Field::COMMENT ) )
            m_comment = m_cache.value( Meta::Field::COMMENT ).toString();
        if( m_cache.contains( Meta::Field::SCORE ) )
            m_score = m_cache.value( Meta::Field::SCORE ).toDouble();
        if( m_cache.contains( Meta::Field::RATING ) )
            m_rating = m_cache.value( Meta::Field::RATING ).toInt();
        if( m_cache.contains( Meta::Field::TRACKNUMBER ) )
            m_trackNumber = m_cache.value( Meta::Field::TRACKNUMBER ).toInt();
        if( m_cache.contains( Meta::Field::DISCNUMBER ) )
            m_discNumber = m_cache.value( Meta::Field::DISCNUMBER ).toInt();
        if( m_cache.contains( Meta::Field::UNIQUEID ) )
            m_uid = m_cache.value( Meta::Field::UNIQUEID ).toString();
        if( m_cache.contains( Meta::Field::URL ) )
            m_url = m_cache.value( Meta::Field::URL ).toString();

        //invalidate the cache of both the old and the new object
        if( m_cache.contains( Meta::Field::ARTIST ) )
        {
            KSharedPtr<SqlArtist>::staticCast( m_artist )->invalidateCache();
            m_artist = m_collection->registry()->getArtist( m_cache.value( Meta::Field::ARTIST ).toString() );
            KSharedPtr<SqlArtist>::staticCast( m_artist )->invalidateCache();
        }
        if( m_cache.contains( Meta::Field::ALBUM ) )
        {
            KSharedPtr<SqlAlbum>::staticCast( m_album )->invalidateCache();
            int artistId = KSharedPtr<SqlArtist>::staticCast( m_artist )->id();
            m_album = m_collection->registry()->getAlbum( m_cache.value( Meta::Field::ALBUM ).toString(), -1, artistId );
            KSharedPtr<SqlAlbum>::staticCast( m_album )->invalidateCache();
        }
        if( m_cache.contains( Meta::Field::COMPOSER ) )
        {
            KSharedPtr<SqlComposer>::staticCast( m_composer )->invalidateCache();
            m_composer = m_collection->registry()->getComposer( m_cache.value( Meta::Field::COMPOSER ).toString() );
            KSharedPtr<SqlComposer>::staticCast( m_composer )->invalidateCache();
        }
        if( m_cache.contains( Meta::Field::GENRE ) )
        {
            KSharedPtr<SqlGenre>::staticCast( m_genre )->invalidateCache();
            m_genre = m_collection->registry()->getGenre( m_cache.value( Meta::Field::GENRE ).toString() );
            KSharedPtr<SqlGenre>::staticCast( m_genre )->invalidateCache();
        }
        if( m_cache.contains( Meta::Field::YEAR ) )
        {
            KSharedPtr<SqlYear>::staticCast( m_year )->invalidateCache();
            m_year = m_collection->registry()->getYear( m_cache.value( Meta::Field::YEAR ).toString() );
            KSharedPtr<SqlYear>::staticCast( m_year )->invalidateCache();
        }

        //updating the tags of the file might change the filesize
        //therefore write the tag to the file first, and update the db
        //with the new filesize
        writeMetaDataToFile();
        writeMetaDataToDb();
        updateStatisticsInDb();
    }
}

void
SqlTrack::writeMetaDataToDb()
{
    //TODO store the tracks id in SqlTrack
    QString query = "SELECT tracks.id FROM tracks LEFT JOIN urls ON tracks.url = urls.id WHERE urls.uniqueid = '%1';";
    query = query.arg( m_collection->escape( m_uid ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        debug() << "Could not perform update in writeMetaDataToDb";
        return;
    }
    int id = res[0].toInt();
    QString update = "UPDATE tracks SET %1 WHERE id = %2;";
    QString tags = "title='%1',comment='%2',tracknumber=%3,discnumber=%4, artist=%5,album=%6,genre=%7,composer=%8,year=%9";
    QString artist = QString::number( KSharedPtr<SqlArtist>::staticCast( m_artist )->id() );
    QString album = QString::number( KSharedPtr<SqlAlbum>::staticCast( m_album )->id() );
    QString genre = QString::number( KSharedPtr<SqlGenre>::staticCast( m_genre )->id() );
    QString composer = QString::number( KSharedPtr<SqlComposer>::staticCast( m_composer )->id() );
    QString year = QString::number( KSharedPtr<SqlYear>::staticCast( m_year )->id() );
    tags = tags.arg( m_collection->escape( m_title ), m_collection->escape( m_comment ),
              QString::number( m_trackNumber ), QString::number( m_discNumber ),
              artist, album, genre, composer, year );
    tags += QString( ",filesize=%1" ).arg( m_filesize );
    update = update.arg( tags, QString::number( id ) );
    m_collection->query( update );
}

void
SqlTrack::updateStatisticsInDb()
{
    QString query = "SELECT urls.id FROM urls WHERE urls.deviceid = %1 AND urls.rpath = '%2';";
    query = query.arg( QString::number( m_deviceid ), m_collection->escape( m_rpath ) );
    QStringList res = m_collection->query( query );
    int urlId = res[0].toInt();
    QStringList count = m_collection->query( QString( "SELECT count(*) FROM statistics WHERE url = %1;" ).arg( urlId ) );
    if( count[0].toInt() == 0 )
    {
        m_firstPlayed = QDateTime::currentDateTime().toTime_t();
        QString insert = "INSERT INTO statistics(url,rating,score,playcount,accessdate,createdate,uniqueid) VALUES ( %1 );";
        QString data = "%1,%2,%3,%4,%5,%6,'%7'";
        data = data.arg( QString::number( urlId )
                , QString::number( m_rating )
                , QString::number( m_score )
                , QString::number( m_playCount )
                , QString::number( m_lastPlayed )
                , QString::number( m_firstPlayed )
                , m_collection->escape( m_uid ) );
        insert = insert.arg( data );
        m_collection->insert( insert, "statistics" );
    }
    else
    {
        QString update = "UPDATE statistics SET %1 WHERE url = %2;";
        QString data = "rating=%1, score=%2, playcount=%3, accessdate=%4";
        data = data.arg( QString::number( m_rating )
                , QString::number( m_score )
                , QString::number( m_playCount )
                , QString::number( m_lastPlayed ) );
        update = update.arg( data, QString::number( urlId ) );
        m_collection->query( update );
    }
}

void
SqlTrack::finishedPlaying( double playedFraction )
{
    // Only increment playcount if we've played more than half of the song... It seems like the sanest comprimise.
    // and only update the other stats (if we did assume that is not played we should not update the last played date
    if ( playedFraction >= 0.5 )
    {
        m_lastPlayed = QDateTime::currentDateTime().toTime_t();
        m_playCount++;
        if( !m_firstPlayed )
        {
            m_firstPlayed = m_lastPlayed;
        }
    }
    //    ScriptManager::instance()->requestNewScore( url(), score(), playCount(), length(), playedFraction * 100 /*scripts expect it as a percent, not a fraction*/, QString() );
    updateStatisticsInDb();
    notifyObservers();
}

bool
SqlTrack::inCollection() const
{
    return true;
}

Collection*
SqlTrack::collection() const
{
    return m_collection;
}

QString
SqlTrack::cachedLyrics() const
{
//     QString query = QString( "SELECT lyrics FROM lyrics WHERE deviceid = %1 AND url = '%2';" )
//                         .arg( QString::number( m_deviceid ), m_collection->escape( m_rpath ) );
    QString query = QString( "SELECT lyrics FROM lyrics WHERE url = '%1'" )
                        .arg( m_collection->escape( m_rpath ) );
    QStringList result = m_collection->query( query );
    if( result.isEmpty() )
        return QString();
    else
        return result[0];
}

void
SqlTrack::setCachedLyrics( const QString &lyrics )
{
//     QString query = QString( "SELECT count(*) FROM lyrics WHERE deviceid = %1 AND url = '%2';" )
//                         .arg( QString::number( m_deviceid ), m_collection->escape( m_rpath ) );
    QString query = QString( "SELECT count(*) FROM lyrics WHERE url = '%1'")
                        .arg( m_collection->escape(m_rpath) );
    QStringList queryResult = m_collection->query( query );
    if( queryResult[0].toInt() == 0 )
    {
        QString insert = QString( "INSERT INTO lyrics( url, lyrics, uniqueid ) VALUES ( '%1', '%2', '%3' );" )
                            .arg( m_collection->escape( m_rpath ),
                                  m_collection->escape( lyrics ),
                                  m_collection->escape( m_uid ) );
        m_collection->insert( insert, "lyrics" );
    }
    else
    {
        QString update = QString( "UPDATE lyrics SET lyrics = '%3' WHERE url = '%1';" )
                            .arg( m_collection->escape( m_rpath ),
                                  m_collection->escape( lyrics ) );
        m_collection->query( update );
    }
}

bool
SqlTrack::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    switch( type )
    {
        case Meta::Capability::CustomActions:
        case Meta::Capability::Organisable:
            return true;
        case Meta::Capability::Updatable:
            return true;
        case Meta::Capability::Editable:
            return QFile::permissions( playableUrl().path() ) & QFile::WriteUser;

        default:
            return false;
    }
}

Meta::Capability*
SqlTrack::asCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::Editable:
            return new EditCapabilityImpl( this );

        case Meta::Capability::CustomActions:
        {
            QList<PopupDropperAction*> actions;
            //TODO These actions will hang around until m_collection is destructed.
            // Find a better parent to avoid this memory leak.
            //actions.append( new CopyToDeviceAction( m_collection, this ) );

            return new CustomActionsCapability( actions );
        }

        case Meta::Capability::Organisable:
        {
            return new OrganiseCapabilityImpl( this );
        }

        case Meta::Capability::Updatable:
            return new UpdateCapabilitySql( this );

        default:
            return 0;
    }
}

//---------------------- class Artist --------------------------

SqlArtist::SqlArtist( SqlCollection* collection, int id, const QString &name ) : Artist()
    ,m_collection( QPointer<SqlCollection>( collection ) )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_albumsLoaded( false )
    ,m_mutex( QMutex::Recursive )
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
    else if( m_collection )
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->setQueryType( QueryMaker::Track );
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

AlbumList
SqlArtist::albums()
{
    QMutexLocker locker( &m_mutex );
    if( m_albumsLoaded )
    {
        return m_albums;
    }
    else if( m_collection )
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->setQueryType( QueryMaker::Album );
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_albums = bq.albums( m_collection->collectionId() );
        m_albumsLoaded = true;
        return m_albums;
    }
    else
    {
        return AlbumList();
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

bool
SqlArtist::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    switch( type )
    {
        case Meta::Capability::CustomActions:
            return true;

        default:
            return false;
    }
}

Meta::Capability*
SqlArtist::asCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::CustomActions:
        {
            QList<PopupDropperAction *> actions;
            //actions.append( new CopyToDeviceAction( m_collection, this ) );
            actions.append( new SimilarArtistsAction( m_collection, this ) );
            return new CustomActionsCapability( actions );
        }

        default:
            return 0;
    }
}

/*void
SqlArtist::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
}*/

//---------------Album compilation management actions-----

class CompilationAction : public PopupDropperAction
{
    Q_OBJECT
    public:
        CompilationAction( QObject* parent, SqlAlbum *album )
            : PopupDropperAction( parent )
            , m_album( album )
            , m_isCompilation( album->isCompilation() )
            {
                connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
                if( m_isCompilation )
                {
                    setText( i18n( "Do not show under Various Artists" ) );
                }
                else
                {
                    setText( i18n( "Show under Various Artists" ) );
                }
            }

    private slots:
        void slotTriggered()
        {
            m_album->setCompilation( !m_isCompilation );
        }
    private:
        KSharedPtr<SqlAlbum> m_album;
        bool m_isCompilation;
};


//---------------SqlAlbum---------------------------------

SqlAlbum::SqlAlbum( SqlCollection* collection, int id, const QString &name, int artist ) : Album()
    ,m_collection( QPointer<SqlCollection>( collection ) )
    ,m_name( name )
    ,m_id( id )
    ,m_artistId( artist )
    ,m_hasImage( false )
    ,m_hasImageChecked( false )
    ,m_tracksLoaded( false )
    ,m_artist()
    ,m_mutex( QMutex::Recursive )
{
    //nothing to do
}

void
SqlAlbum::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_hasImage = false;
    m_hasImageChecked = false;
    m_images.clear();
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
    else if( m_collection )
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->setQueryType( QueryMaker::Track );
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

bool
SqlAlbum::hasImage( int size ) const
{
    if( !m_hasImageChecked )
        m_hasImage = ! const_cast<SqlAlbum*>( this )->image( size ).isNull();
    return m_hasImage;
}

QPixmap
SqlAlbum::image( int size, bool withShadow )
{
    if( m_hasImageChecked && !m_hasImage )
        return Meta::Album::image( size, withShadow );

    m_hasImageChecked = true;

    //FIXME this cache doesn't differentiate between shadowed/unshadowed
    if( m_images.contains( size ) )
        return QPixmap( m_images.value( size ) );

    QString result;

    // findCachedImage looks for a scaled version of the fullsize image
    // which may have been saved on a previous lookup
    QString cachedImage = findCachedImage( size );
    if( !cachedImage.isEmpty() )
        result = cachedImage;

    // findImage will lookup the original cover and create a scaled version
    // of the cover if required (returning the path of that scaled image)
    else
    {
        QString image = findImage( size );
        if( !image.isEmpty() && size < 1000 )
            result = image;
    }

    if( !result.isEmpty() )
    {
        m_hasImage = true;
        m_images.insert( size, result );
        return QPixmap( result );
    }

    // Cover fetching runs in another thread. If there is a retrieved cover
    // then updateImage() gets called which updates the cache and alerts the
    // subscribers. We use queueAlbum() because this runs the fetch as a
    // background job and doesn't give an intruding popup asking for confirmation
    if( !m_name.isEmpty() )
        CoverFetcher::instance()->queueAlbum( KSharedPtr<Meta::Album>(this) );

    // If the result image is empty then we didn't find any cached image, nor 
    // could we find the original cover to scale to the appropriate size. Hence,
    // the album cannot have a cover, for any size. In this case, we return the
    // default image

    m_hasImage = false;

    return Meta::Album::image( size, withShadow );
}

void
SqlAlbum::setImage( const QImage &image )
{
    if( image.isNull() )
        return;

    QByteArray widthKey = QString::number( image.width() ).toLocal8Bit() + '@';
    QString album = m_name;
    QString artist = hasAlbumArtist() ? albumArtist()->name() : QString();

    if( artist.isEmpty() && album.isEmpty() )
        return;

    // removeImage() will destroy all scaled cached versions of the artwork 
    // and remove references from the database if required.
    if( hasImage( -1 ) ) // -1 is a dummy
        removeImage();

    QByteArray key = md5sum( artist, album, QString() );
    QString path = Amarok::saveLocation( "albumcovers/large/" ) + key;
    image.save( path, "JPG" );

    updateImage( path );

    notifyObservers();
}

void
SqlAlbum::removeImage()
{
    QString album = m_name;
    QString artist = hasAlbumArtist() ? albumArtist()->name() : QString();

    if( artist.isEmpty() && album.isEmpty() )
        return;

    QByteArray key = md5sum( artist, album, QString() );

    // remove the large covers
    QFile::remove( Amarok::saveLocation( "albumcovers/large/" ) + key );

    // remove all cache images
    QDir        cacheDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    QStringList cacheFilter;
    cacheFilter << QString( '*' + key );
    QStringList cachedImages = cacheDir.entryList( cacheFilter );

    foreach( const QString &image, cachedImages )
    {
        bool r = QFile::remove( cacheDir.filePath( image ) );
        debug() << "deleting cached image: " << image << " : " + ( r ? QString("ok") : QString("fail") );
    }

    // TODO: remove directory image ??
    
    // Update the database image path
    
    QString query = "SELECT images.id, count( images.id ) FROM images, albums "
                    "WHERE albums.image = images.id AND albums.id = %1 "
                    "GROUP BY images.id";
    QStringList res = m_collection->query( query.arg( QString::number( m_id ) ) );
    if( !res.isEmpty() )
    {
        int imageId    = res[0].toInt();
        int references = res[1].toInt();

        // Set the album image to empty
        query = "UPDATE albums SET image = NULL WHERE id = %1";
        m_collection->query( query.arg( QString::number( m_id ) ) );

        // We've just removed a references to that imageid
        references--;

        // From here on we check if there are any remaining references to that particular image in the database
        // If there aren't, then we should remove the image path from the database ( and possibly delete the file? )
        // If there are, we need to leave it since other albums will reference this particular image path.
        //
        // TODO: Should this cleanup be handled by the Collection at some other point with a DB wide cleanup, with a
        // vaccuum analyze? Since the use case will most likely be a 1:1 mapping of album-image by moving the cleanup
        // elsewhere we could remove 2 of the queries for this method - ie, it would be just one UPDATE statement.

        // If there are no more references to this particular image, then we should clean up
        if( references <= 0 )
        {
            query = "DELETE FROM images WHERE id = %1";
            m_collection->query( query.arg( QString::number( imageId ) ) );
        }
    }

    m_images.clear();

    notifyObservers();
}

bool
SqlAlbum::isCompilation() const
{
    return !hasAlbumArtist();
}

bool
SqlAlbum::hasAlbumArtist() const
{
    DEBUG_BLOCK
    debug() << "AID: " << m_artistId;
    return m_artistId != 0;
}

Meta::ArtistPtr
SqlAlbum::albumArtist() const
{
    DEBUG_BLOCK
    if( m_artistId != 0 && !m_artist )
    {
        QString query = QString( "SELECT artists.name FROM artists WHERE artists.id = %1;" ).arg( m_artistId );
        debug() << "QUERY: " << query;
        QStringList result = m_collection->query( query );
        if( result.isEmpty() )
            return Meta::ArtistPtr();
        const_cast<SqlAlbum*>( this )->m_artist =
            m_collection->registry()->getArtist( result.first(), m_artistId );
    }
    return m_artist;
}

QByteArray
SqlAlbum::md5sum( const QString& artist, const QString& album, const QString& file ) const
{
    KMD5 context( artist.toLower().toLocal8Bit() + album.toLower().toLocal8Bit() + file.toLocal8Bit() );
    return context.hexDigest();
}

QString
SqlAlbum::findCachedImage( int size ) const
{
    QByteArray widthKey = QString::number( size ).toLocal8Bit() + '@';
    QString album = m_name;
    QString artist = hasAlbumArtist() ? albumArtist()->name() : QString();

    if ( artist.isEmpty() && album.isEmpty() )
        return QString();

    QByteArray key = md5sum( artist, album, QString() );

    QDir cacheCoverDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    // check cache for existing cover
    if ( cacheCoverDir.exists( widthKey + key ) )
        return cacheCoverDir.filePath( widthKey + key );
    return QString();
}

QString
SqlAlbum::createScaledImage( QString path, int size ) const
{
    if( size <= 1 )
        return QString();

    QByteArray widthKey = QString::number( size ).toLocal8Bit() + '@';
    QString album = m_name;
    QString artist = hasAlbumArtist() ? albumArtist()->name() : QString();

    if( artist.isEmpty() && album.isEmpty() )
        return QString();

    QByteArray key = md5sum( artist, album, QString() );

    QDir cacheCoverDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    QString cachedImagePath = cacheCoverDir.filePath( widthKey + key );

    if( QFile::exists( path ) )
    {
        // Don't overwrite if it already exists
        if( !QFile::exists( cachedImagePath ) )
        {
            QImage img( path );
            if( img.isNull() )
                return QString();
           
            // resize and save the image
            img.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation ).save( cachedImagePath, "JPG" );
        }
        return cachedImagePath;
    }

    return QString();
}

QString
SqlAlbum::findImage( int size )
{
    if( m_images.contains( size ) )
        return m_images.value( size );

    QString fullsize;
    
    // get the full size path from the cache if we have it
    if( m_images.contains( 0 ) ) 
    {
        fullsize = m_images.value( 0 );
    }
    // if we don't have it, retrieve it from the database
    else
    {
        QString query = "SELECT path FROM images, albums WHERE albums.image = images.id AND albums.id = %1;";
        QStringList res = m_collection->query( query.arg( m_id ) );
        if( !res.isEmpty() )
        {
            fullsize = res.first();
            if( !fullsize.isEmpty() )
                m_images.insert( 0, fullsize ); // store the full size version
        }
    }

    if( QFile::exists( fullsize ) )
    {
        if( size > 1 )
            return createScaledImage( fullsize, size );
        return fullsize;
    }

    return QString();
}

void
SqlAlbum::updateImage( const QString path ) const
{
    DEBUG_BLOCK
    QString query = "SELECT id FROM images WHERE path = '%1'";
    query = query.arg( m_collection->escape( path ) );
    QStringList res = m_collection->query( query );

    int imageid = -1;

    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO images( path ) VALUES ( '%1' )" )
                            .arg( m_collection->escape( path ) );
        imageid = m_collection->insert( insert, "images" );
    }
    else
        imageid = res[0].toInt();

    if( imageid >= 0 )
    {
        query = QString("UPDATE albums SET image = %1 WHERE albums.id = %2" )
                    .arg( QString::number( imageid ), QString::number( m_id ) );
        m_hasImage = true;
        m_hasImageChecked = true;
        m_images.insert( 0, path );
        m_collection->query( query );
    }
}

void
SqlAlbum::setCompilation( bool compilation )
{
    DEBUG_BLOCK
    AMAROK_NOTIMPLEMENTED
    if( isCompilation() == compilation )
    {
        return;
    }
    else
    {
        if( compilation )
        {
            debug() << "User selected album as compilation";
            m_artistId = 0;
            m_artist = Meta::ArtistPtr();

            QString update = "UPDATE albums SET artist = NULL WHERE id = %1;";
            m_collection->query( update.arg( m_id ) );
        }
        else
        {
            debug() << "User selected album as non-compilation";
            //TODO find album artist
        }
        notifyObservers();
        m_collection->sendChangedSignal();
    }
}

bool
SqlAlbum::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    switch( type )
    {
        case Meta::Capability::CustomActions:
            return true;

        default:
            return false;
    }
}

Meta::Capability*
SqlAlbum::asCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::CustomActions:
        {
            QList<PopupDropperAction*> actions;
            //actions.append( new CopyToDeviceAction( m_collection, this ) );
            actions.append( new CompilationAction( m_collection, this ) );
            PopupDropperAction* separator = new PopupDropperAction( m_collection );
            separator->setSeparator( true );
            actions.append( separator );
            actions.append( new FetchCoverAction( m_collection, this ) );
            actions.append( new SetCustomCoverAction( m_collection, this ) );
            PopupDropperAction *displayCoverAction = new DisplayCoverAction( m_collection, this );
            PopupDropperAction *unsetCoverAction   = new UnsetCoverAction( m_collection, this );
            if( !hasImage() )
            {
                displayCoverAction->setEnabled( false );
                unsetCoverAction->setEnabled( false );
            }
            actions.append( displayCoverAction );
            actions.append( unsetCoverAction );
            return new CustomActionsCapability( actions );
        }

        default:
            return 0;
    }
}

//---------------SqlComposer---------------------------------

SqlComposer::SqlComposer( SqlCollection* collection, int id, const QString &name ) : Composer()
    ,m_collection( QPointer<SqlCollection>( collection ) )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_mutex( QMutex::Recursive )
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
    else if( m_collection )
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->setQueryType( QueryMaker::Track );
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

//---------------SqlGenre---------------------------------

SqlGenre::SqlGenre( SqlCollection* collection, int id, const QString &name ) : Genre()
    ,m_collection( QPointer<SqlCollection>( collection ) )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_mutex( QMutex::Recursive )
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
    else if( m_collection )
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->setQueryType( QueryMaker::Track );
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

//---------------SqlYear---------------------------------

SqlYear::SqlYear( SqlCollection* collection, int id, const QString &name ) : Year()
    ,m_collection( QPointer<SqlCollection>( collection ) )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_mutex( QMutex::Recursive )
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
    else if( m_collection )
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->setQueryType( QueryMaker::Track );
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

#include "sqlmeta.moc"
