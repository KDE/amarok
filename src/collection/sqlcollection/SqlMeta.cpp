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

#include "amarok.h"
#include "BlockingQuery.h"
#include "debug.h"
#include "MainWindow.h"
#include "mediadevice/CopyToDeviceAction.h"
#include "meta/CustomActionsCapability.h"
#include "meta/EditCapability.h"
#include "SqlRegistry.h"
#include "SqlCollection.h"

#include "mountpointmanager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QListIterator>
#include <QMutexLocker>
#include <QPointer>

#include <KAction>
#include <kcodecs.h>
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

QString
SqlTrack::getTrackReturnValues()
{
    return "urls.deviceid, urls.rpath, "
           "tracks.title, tracks.comment, "
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

SqlTrack::SqlTrack( SqlCollection* collection, const QStringList &result )
    : Track()
    , m_collection( QPointer<SqlCollection>( collection ) )
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
    m_album = registry->getAlbum( result[19], result[20].toInt(), result[21].toInt() );
    m_genre = registry->getGenre( result[22], result[23].toInt() );
    m_composer = registry->getComposer( result[24], result[25].toInt() );
    m_year = registry->getYear( result[26], result[27].toInt() );
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
SqlTrack::setTitle( const QString &newTitle )
{
    if( !m_batchUpdate )
    {
        m_title = newTitle;
        writeMetaDataToFile();
        writeMetaDataToDb();
        notifyObservers();
    }
    else
        m_cache->title = newTitle;
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
    //TODO store the tracks id in SqlTrack
    QString query = "SELECT tracks.id FROM tracks LEFT JOIN urls ON tracks.url = urls.id WHERE urls.deviceid = %1 AND urls.rpath = '%2';";
    query = query.arg( QString::number( m_deviceid ), m_collection->escape( m_rpath ) );
    QStringList res = m_collection->query( query );
    int id = res[0].toInt();
    QString update = "UPDATE tracks SET %1 WHERE id = %2;";
    QString tags = "title='%1',comment='%2',tracknumber=%3,discnumber=%4, artist=%5,album=%6,genre=%7,composer=%8,year=%9";
    QString artist = QString::number( KSharedPtr<SqlArtist>::staticCast( m_artist )->id() );
    QString album = QString::number( KSharedPtr<SqlAlbum>::staticCast( m_album )->id() );
    QString genre = QString::number( KSharedPtr<SqlGenre>::staticCast( m_genre )->id() );
    QString composer = QString::number( KSharedPtr<SqlComposer>::staticCast( m_composer )->id() );
    QString year = QString::number( KSharedPtr<SqlYear>::staticCast( m_year )->id() );
    tags.arg( m_collection->escape( m_title ), m_collection->escape( m_comment ),
              QString::number( m_trackNumber ), QString::number( m_discNumber ),
              artist, album, genre, composer, year );
    update.arg( tags, QString::number( id ) );
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
        QString insert = "INSERT INTO statistics(url,rating,score,playcount,accessdate,createdate) VALUES ( %1 );";
        QString data = "%1,%2,%3,%4,%5,%6";
        data = data.arg( count[0] ).arg( m_rating ).arg( m_score );
        data = data.arg( m_playCount ).arg( m_lastPlayed ).arg( m_firstPlayed );
        insert = insert.arg( data );
    }
    else
    {
        QString update = "UPDATE statistics SET %1 WHERE url = %2;";
        QString data = "rating=%1, score=%2, playcount=%3, accessdate=%4";
        data = data.arg( m_rating ).arg( m_score ).arg( m_playCount ).arg( m_lastPlayed );
        update = update.arg( data, count[0] );
        m_collection->query( update );
    }
}

void
SqlTrack::finishedPlaying( double playedFraction )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( playedFraction );
    m_lastPlayed = QDateTime::currentDateTime().toTime_t();
    m_playCount++;
    //TODO get new rating
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
    QString query = QString( "SELECT lyrics FROM lyrics WHERE deviceid = %1 AND url = '%2';" )
                        .arg( QString::number( m_deviceid ), m_collection->escape( m_rpath ) );
    QStringList result = m_collection->query( query );
    if( result.isEmpty() )
        return QString();
    else
        return result[0];
}

void
SqlTrack::setCachedLyrics( const QString &lyrics )
{
    QString query = QString( "SELECT count(*) FROM lyrics WHERE deviceid = %1 AND url = '%2';" )
                        .arg( QString::number( m_deviceid ), m_collection->escape( m_rpath ) );
    QStringList queryResult = m_collection->query( query );
    if( queryResult[0].toInt() == 0 )
    {
        QString insert = QString( "INSERT INTO lyrics( deviceid, url, lyrics ) VALUES ( %1, '%2', '%3' );" )
                            .arg( QString::number( m_deviceid ),
                                  m_collection->escape( m_rpath ),
                                  m_collection->escape( lyrics ) );
        m_collection->insert( insert, "lyrics" );
    }
    else
    {
        QString update = QString( "UPDATE lyrics SET lyrics = '%3' WHERE deviceid = %1 AND url = '%2';" )
                            .arg( QString::number( m_deviceid ),
                                  m_collection->escape( m_rpath ),
                                  m_collection->escape( lyrics ) );
        m_collection->query( update );
    }
}

bool
SqlTrack::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    switch( type )
    {
        case Meta::Capability::Editable:
        case Meta::Capability::CustomActions:
            return true;

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
            QList<QAction*> actions;
            //TODO These actions will hang around until m_collection is destructed.
            // Find a better parent to avoid this memory leak.
            actions.append( new CopyToDeviceAction( m_collection, this ) );
            return new CustomActionsCapability( actions );
        }

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
        qm->startTrackQuery();
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
        qm->startAlbumQuery();
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
            QList<QAction*> actions;
            actions.append( new CopyToDeviceAction( m_collection, this ) );
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

class CompilationAction : public KAction
{
    Q_OBJECT
    public:
        CompilationAction( QObject* parent, SqlAlbum *album )
            : KAction( parent )
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
    ,m_tracksLoaded( false )
    ,m_artist()
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
    else if( m_collection )
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
    else
        return TrackList();
}
bool
SqlAlbum::hasImage( int size ) const
{
    QByteArray widthKey = QString::number( size ).toLocal8Bit() + '@';
    QString album = m_name;
    QString artist = hasAlbumArtist() ? albumArtist()->name() : QString();

    if ( artist.isEmpty() && album.isEmpty() )
        return false;

    QByteArray key = md5sum( artist, album, QString() );

    QDir imageDir( Amarok::saveLocation( "albumcovers/large/" ) );
    if ( imageDir.exists( key ) ) {
        return true;
    }
    return false;
}
QPixmap
SqlAlbum::image( int size, bool withShadow )
{
    QString amazonImage = findAmazonImage( size );
    if( !amazonImage.isEmpty() && size < 1000 )
    {
        return QPixmap( amazonImage );
    }
    else
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

    QByteArray key = md5sum( artist, album, QString() );
    image.save( Amarok::saveLocation( "albumcovers/large/" ) + key, "JPG" );
    
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
    cacheFilter << QString( "*" + key );
    QStringList cachedImages = cacheDir.entryList( cacheFilter );

    foreach( QString image, cachedImages )
    {
        bool r = QFile::remove( cacheDir.filePath( image ) );
        debug() << "deleting cached image: " << image << " : " + ( r ? QString("ok") : QString("fail") );
    }

    // TODO: remove directory image ??
    
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
    return m_artistId != 0;
}

Meta::ArtistPtr
SqlAlbum::albumArtist() const
{
    if( m_artistId != 0 && !m_artist )
    {
        QString query = QString( "SELECT artists.name FROM artists WHERE artists.id = %1;" ).arg( m_artistId );
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
SqlAlbum::findAmazonImage( int size ) const
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

    // we need to create a scaled version of this cover
    QDir imageDir( Amarok::saveLocation( "albumcovers/large/" ) );
    if ( imageDir.exists( key ) )
    {
        if ( size > 1 )
        {
            QImage img( imageDir.filePath( key ) );
            img.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation ).save( cacheCoverDir.filePath( widthKey + key ), "PNG" );

            return cacheCoverDir.filePath( widthKey + key );
        }
        else
            return imageDir.filePath( key );
    }

    return QString();
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
            QList<QAction*> actions;
            actions.append( new CopyToDeviceAction( m_collection, this ) );
            actions.append( new CompilationAction( m_collection, this ) );
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
        qm->startTrackQuery();
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
        qm->startTrackQuery();
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
        qm->startTrackQuery();
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
