/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
 * Copyright (c) 2010 Jeff Mitchell <mitchell@kde.org>                                  *
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

#include "DatabaseMeta.h"

#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "CapabilityDelegate.h"
#include <core/support/Debug.h>
#include <core/meta/support/MetaUtility.h>
#include <core-impl/meta/file/File.h> // for getting an embedded cover
#include <core-impl/meta/file/TagLibUtils.h>
#include <core-impl/collections/support/ArtistHelper.h>
#include "DatabaseCollection.h"
#include "DatabaseQueryMaker.h"
#include "DatabaseRegistry.h"
#include "covermanager/CoverFetcher.h"
#include "core/collections/support/DatabaseStorage.h"

#include "collectionscanner/AFTUtility.h"

#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMultiHash>
#include <QMutexLocker>
#include <QPointer>
#if QT_VERSION >= 0x040600
#include <QPixmapCache>
#endif

#include <KCodecs>
#include <KLocale>
#include <KSharedPtr>

// additional constants
namespace Meta
{
    static const qint64 valAlbumId  = valCustom + 1;
    static const qint64 valCreated  = valCustom + 2;
}

using namespace Meta;

QString
DatabaseTrack::getTrackReturnValues()
{
    //do not use any weird column names that contains commas: this will break getTrackReturnValuesCount()
    return "urls.id, urls.deviceid, urls.rpath, urls.uniqueid, "
           "tracks.id, tracks.title, tracks.comment, "
           "tracks.tracknumber, tracks.discnumber, "
           "statistics.score, statistics.rating, "
           "tracks.bitrate, tracks.length, "
           "tracks.filesize, tracks.samplerate, "
           "statistics.id, "
           "statistics.createdate, statistics.accessdate, "
           "statistics.playcount, tracks.filetype, tracks.bpm, "
           "tracks.createdate, tracks.albumgain, tracks.albumpeakgain, "
           "tracks.trackgain, tracks.trackpeakgain, "
           "artists.name, artists.id, " // TODO: just reading the id should be sufficient
           "albums.name, albums.id, albums.artist, " // TODO: again here
           "genres.name, genres.id, " // TODO: again here
           "composers.name, composers.id, " // TODO: again here
           "years.name, years.id"; // TODO: again here
}

QString
DatabaseTrack::getTrackJoinConditions()
{
    return "INNER JOIN tracks ON urls.id = tracks.url "
           "LEFT JOIN statistics ON urls.id = statistics.url "
           "LEFT JOIN artists ON tracks.artist = artists.id "
           "LEFT JOIN albums ON tracks.album = albums.id "
           "LEFT JOIN genres ON tracks.genre = genres.id "
           "LEFT JOIN composers ON tracks.composer = composers.id "
           "LEFT JOIN years ON tracks.year = years.id";
}

int
DatabaseTrack::getTrackReturnValueCount()
{
    static int count = getTrackReturnValues().split( ',' ).count();
    return count;
}

DatabaseTrack::DatabaseTrack( Collections::DatabaseCollection* collection, int deviceId,
                    const QString &rpath, int directoryId, const QString uidUrl )
    : Track()
    , m_collection( QPointer<Collections::DatabaseCollection>( collection ) )
    , m_createDate( QDateTime::currentDateTime().toTime_t() )
    , m_writeFile( true )
    , m_labelsInCache( false )
{
    m_batchUpdate = true; // I don't want commits yet

    m_urlId = -1; // this will be set with the first database write
    m_trackId = -1; // this will be set with the first database write
    m_statisticsId = -1;

    setUrl( deviceId, rpath, directoryId );
    setUidUrl( uidUrl );

    m_trackNumber = 0;
    m_discNumber = 0;
    m_score = 0;
    m_rating = 0;
    m_bitrate = 0;
    m_length = 0;
    m_filesize = 0;
    m_sampleRate = 0;
    m_firstPlayed = 0;
    m_lastPlayed = 0;
    m_playCount = 0;
    m_bpm = 0.0;
    m_createDate = QDateTime::currentDateTime().toTime_t();
    m_cache.insert( Meta::valCreateDate, m_createDate ); // ensure that the created date is written the next time

    m_trackGain = 0.0;
    m_trackPeakGain = 0.0;
    m_albumGain = 0.0;
    m_albumPeakGain = 0.0;

    m_batchUpdate = false;
}

DatabaseTrack::DatabaseTrack( Collections::DatabaseCollection* collection, const QStringList &result )
    : Track()
    , m_collection( QPointer<Collections::DatabaseCollection>( collection ) )
    , m_batchUpdate( false )
    , m_writeFile( true )
    , m_labelsInCache( false )
{
    QStringList::ConstIterator iter = result.constBegin();
    m_urlId = (*(iter++)).toInt();
    m_deviceId = (*(iter++)).toInt();
    m_rpath = *(iter++);
    m_directoryId = -1;
    m_url = KUrl( m_collection->mountPointManager()->getAbsolutePath( m_deviceId, m_rpath ) );
    m_uid = *(iter++);
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
    m_statisticsId = (*(iter++)).toInt();
    m_firstPlayed = (*(iter++)).toInt();
    m_lastPlayed = (*(iter++)).toUInt();
    m_playCount = (*(iter++)).toInt();
    ++iter; //file type
    m_bpm = (*(iter++)).toFloat();
    m_createDate = (*(iter++)).toUInt();

    // if there is no track gain, we assume a gain of 0
    // if there is no album gain, we use the track gain
    QString albumGain = *(iter++);
    QString albumPeakGain = *(iter++);
    m_trackGain = (*(iter++)).toDouble();
    m_trackPeakGain = (*(iter++)).toDouble();
    if ( albumGain.isEmpty() )
    {
        m_albumGain = m_trackGain;
        m_albumPeakGain = m_trackPeakGain;
    }
    else
    {
        m_albumGain = albumGain.toDouble();
        m_albumPeakGain = albumPeakGain.toDouble();
    }
    DatabaseRegistry* registry = m_collection->registry();

    QString artist = *(iter++);
    int artistId = (*(iter++)).toInt();
    if( artistId > 0 )
        m_artist = registry->getArtist( artistId, artist );

    QString album = *(iter++);
    int albumId =(*(iter++)).toInt();
    int albumArtistId = (*(iter++)).toInt();
    m_album = registry->getAlbum( albumId, album, albumArtistId );

    QString genre = *(iter++);
    int genreId = (*(iter++)).toInt();
    if( genreId > 0 )
        m_genre = registry->getGenre( genreId, genre );

    QString composer = *(iter++);
    int composerId = (*(iter++)).toInt();
    if( composerId > 0 )
        m_composer = registry->getComposer( composerId, composer );

    QString year = *(iter++);
    int yearId = (*(iter++)).toInt();
    if( yearId > 0 )
    m_year = registry->getYear( year.toInt(), yearId );
    //Q_ASSERT_X( iter == result.constEnd(), "DatabaseTrack( Collections::DatabaseCollection*, QStringList )", "number of expected fields did not match number of actual fields: expected " + result.size() );
}

DatabaseTrack::~DatabaseTrack()
{
}

bool
DatabaseTrack::isPlayable() const
{
    //a song is not playable anymore if the collection was removed
    return m_collection && QFile::exists( m_url.path() );
}

bool
DatabaseTrack::isEditable() const
{
    QFile::Permissions p = QFile::permissions( m_url.path() );
    const bool editable = ( p & QFile::WriteUser ) || ( p & QFile::WriteGroup ) || ( p & QFile::WriteOther );
    return m_collection && QFile::exists( m_url.path() ) && editable;
}

QString
DatabaseTrack::type() const
{
    return m_url.isLocalFile()
           ? Amarok::extension( m_url.fileName() )
           : i18n( "Stream" );
}

QString
DatabaseTrack::fullPrettyName() const
{
    QString s = m_artist->name();

    //FIXME doesn't work for resume playback

    if( s.isEmpty() )
        s = name();
    else
        s = i18n("%1 - %2", m_artist->name(), name() );

    //TODO
    if( s.isEmpty() )
        s = prettyTitle( m_url.fileName() );

    return s;
}

QString
DatabaseTrack::prettyTitle( const QString &filename ) //static
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
DatabaseTrack::prettyName() const
{
    if ( !name().isEmpty() )
        return name();
    return  prettyTitle( m_url.fileName() );
}

qreal
DatabaseTrack::replayGain( ReplayGainTag mode ) const
{
    switch( mode )
    {
    case Meta::ReplayGain_Track_Gain:
        return m_trackGain;
    case Meta::ReplayGain_Track_Peak:
        return m_trackPeakGain;
    case Meta::ReplayGain_Album_Gain:
        return m_albumGain;
    case Meta::ReplayGain_Album_Peak:
        return m_albumPeakGain;
    }
    return 0.0;
}

void
DatabaseTrack::setReplayGain( Meta::ReplayGainTag mode, qreal value )
{
    QMutexLocker locker( &m_mutex );

    switch( mode )
    {
    case Meta::ReplayGain_Track_Gain:
        m_cache.insert( Meta::valTrackGain, value );
        break;
    case Meta::ReplayGain_Track_Peak:
        m_cache.insert( Meta::valTrackGainPeak, value );
        break;
    case Meta::ReplayGain_Album_Gain:
        m_cache.insert( Meta::valAlbumGain, value );
        break;
    case Meta::ReplayGain_Album_Peak:
        m_cache.insert( Meta::valAlbumGainPeak, value );
        break;
    }

    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setUrl( int deviceId, const QString &rpath, int directoryId )
{
    QMutexLocker locker( &m_mutex );

    m_deviceId = deviceId;
    m_rpath = rpath;
    m_directoryId = directoryId;

    m_cache.insert( Meta::valUrl,
                    KUrl( m_collection->mountPointManager()->getAbsolutePath( m_deviceId, m_rpath ) ).path() );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setUidUrl( const QString &uid )
{
    QMutexLocker locker( &m_mutex );

    QString newid = uid;
    if( !newid.startsWith( "amarok-sqltrackuid" ) )
        newid.prepend( "amarok-sqltrackuid://" );

    m_cache.insert( Meta::valUniqueId, newid );
    if( !m_batchUpdate )
    {
        debug() << "setting uidUrl manually...did you really mean to do this?";
        commitMetaDataChanges();
    }
}

void
DatabaseTrack::setArtist( const QString &newArtist )
{
    QMutexLocker locker( &m_mutex );

    if ( m_artist && m_artist->name() == newArtist )
        return;

    m_cache.insert( Meta::valArtist, newArtist );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setGenre( const QString &newGenre )
{
    QMutexLocker locker( &m_mutex );

    if ( m_genre && m_genre->name() == newGenre )
        return;

    m_cache.insert( Meta::valGenre, newGenre );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setComposer( const QString &newComposer )
{
    QMutexLocker locker( &m_mutex );

    if ( m_composer && m_composer->name() == newComposer )
        return;

    m_cache.insert( Meta::valComposer, newComposer );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setYear( int newYear )
{
    QMutexLocker locker( &m_mutex );

    if ( m_year && m_year->year() == newYear )
        return;

    m_cache.insert( Meta::valYear, newYear );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setBpm( const qreal newBpm )
{
    QMutexLocker locker( &m_mutex );

    if ( m_bpm && m_bpm == newBpm )
        return;

    m_cache.insert( Meta::valBpm, newBpm );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setAlbum( const QString &newAlbum )
{
    QMutexLocker locker( &m_mutex );

    if( m_album && m_album->name() == newAlbum )
        return;

    m_cache.insert( Meta::valAlbum, newAlbum );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setAlbum( int albumId )
{
    QMutexLocker locker( &m_mutex );

    if( m_album )
    {
        Meta::DatabaseAlbum *oldAlbum = static_cast<DatabaseAlbum*>(m_album.data());
        if( oldAlbum->id() == albumId )
            return;
    }

    m_cache.insert( Meta::valAlbumId, albumId );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setScore( double newScore )
{
    QMutexLocker locker( &m_mutex );

    if( newScore == m_score )
        return;

    m_cache.insert( Meta::valScore, newScore );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setRating( int newRating )
{
    QMutexLocker locker( &m_mutex );

    if( newRating == m_rating )
        return;

    m_cache.insert( Meta::valRating, newRating );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setLength( qint64 newLength )
{
    QMutexLocker locker( &m_mutex );

    if( newLength == m_length )
        return;

    m_cache.insert( Meta::valLength, newLength );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setSampleRate( int newSampleRate )
{
    QMutexLocker locker( &m_mutex );

    if( newSampleRate == m_sampleRate )
        return;

    m_cache.insert( Meta::valSamplerate, newSampleRate );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setBitrate( int newBitrate )
{
    QMutexLocker locker( &m_mutex );

    if( newBitrate == m_bitrate )
        return;

    m_cache.insert( Meta::valBitrate, newBitrate );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setTrackNumber( int newTrackNumber )
{
    QMutexLocker locker( &m_mutex );

    if( newTrackNumber == m_trackNumber )
        return;

    m_cache.insert( Meta::valTrackNr, newTrackNumber );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setDiscNumber( int newDiscNumber )
{
    QMutexLocker locker( &m_mutex );

    m_cache.insert( Meta::valDiscNr, newDiscNumber );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setComment( const QString &newComment )
{
    QMutexLocker locker( &m_mutex );

    m_cache.insert( Meta::valComment, newComment );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setTitle( const QString &newTitle )
{
    QMutexLocker locker( &m_mutex );

    m_cache.insert( Meta::valTitle, newTitle );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::setLastPlayed( const uint newTime )
{
    QMutexLocker locker( &m_mutex );

    if( m_batchUpdate )
        m_cache.insert( Meta::valLastPlayed, newTime );
    else
    {
        m_lastPlayed = newTime;
        writeStatisticsToDb( Meta::valLastPlayed );
        notifyObservers();
    }
}

void
DatabaseTrack::setFirstPlayed( const uint newTime )
{
    QMutexLocker locker( &m_mutex );

    if( m_batchUpdate )
        m_cache.insert( Meta::valFirstPlayed, newTime );
    else
    {
        m_firstPlayed = newTime;
        writeStatisticsToDb( Meta::valFirstPlayed );
        notifyObservers();
    }
}

void
DatabaseTrack::setPlayCount( const int newCount )
{
    QMutexLocker locker( &m_mutex );

    if( newCount == m_playCount )
        return;

    m_cache.insert( Meta::valPlaycount, newCount );
    if( !m_batchUpdate )
        commitMetaDataChanges();
}

void
DatabaseTrack::beginMetaDataUpdate()
{
    QMutexLocker locker( &m_mutex );

    m_batchUpdate = true;
}

void
DatabaseTrack::endMetaDataUpdate()
{
    QMutexLocker locker( &m_mutex );

    commitMetaDataChanges();
    m_batchUpdate = false;
}

void
DatabaseTrack::commitMetaDataChanges()
{
    if( m_cache.isEmpty() )
        return; // nothing to do

    // debug() << "DatabaseTrack::commitMetaDataChanges " << m_cache;

    QString oldUid = m_uid;
    bool collectionChanged = false;

    // for all the following objects we need to invalidate the cache and
    // notify the observers after the update
    KSharedPtr<DatabaseArtist>   oldArtist;
    KSharedPtr<DatabaseArtist>   newArtist;
    KSharedPtr<DatabaseAlbum>    oldAlbum;
    KSharedPtr<DatabaseAlbum>    newAlbum;
    KSharedPtr<DatabaseComposer> oldComposer;
    KSharedPtr<DatabaseComposer> newComposer;
    KSharedPtr<DatabaseGenre>    oldGenre;
    KSharedPtr<DatabaseGenre>    newGenre;
    KSharedPtr<DatabaseYear>     oldYear;
    KSharedPtr<DatabaseYear>     newYear;

    if( m_cache.contains( Meta::valTitle ) )
        m_title = m_cache.value( Meta::valTitle ).toString();
    if( m_cache.contains( Meta::valComment ) )
        m_comment = m_cache.value( Meta::valComment ).toString();
    if( m_cache.contains( Meta::valScore ) )
        m_score = m_cache.value( Meta::valScore ).toDouble();
    if( m_cache.contains( Meta::valRating ) )
        m_rating = m_cache.value( Meta::valRating ).toInt();
    if( m_cache.contains( Meta::valLength ) )
        m_length = m_cache.value( Meta::valLength ).toLongLong();
    if( m_cache.contains( Meta::valSamplerate ) )
        m_sampleRate = m_cache.value( Meta::valSamplerate ).toInt();
    if( m_cache.contains( Meta::valBitrate ) )
        m_bitrate = m_cache.value( Meta::valBitrate ).toInt();
    if( m_cache.contains( Meta::valFirstPlayed ) )
        m_firstPlayed = m_cache.value( Meta::valFirstPlayed ).toUInt();
    if( m_cache.contains( Meta::valLastPlayed ) )
        m_lastPlayed = m_cache.value( Meta::valLastPlayed ).toUInt();
    if( m_cache.contains( Meta::valTrackNr ) )
        m_trackNumber = m_cache.value( Meta::valTrackNr ).toInt();
    if( m_cache.contains( Meta::valDiscNr ) )
        m_discNumber = m_cache.value( Meta::valDiscNr ).toInt();
    if( m_cache.contains( Meta::valPlaycount ) )
        m_playCount = m_cache.value( Meta::valPlaycount ).toInt();
    if( m_cache.contains( Meta::valCreateDate ) )
        m_createDate = m_cache.value( Meta::valCreateDate ).toUInt();
    if( m_cache.contains( Meta::valTrackGain ) )
        m_trackGain = m_cache.value( Meta::valTrackGain ).toDouble();
    if( m_cache.contains( Meta::valTrackGainPeak ) )
        m_trackPeakGain = m_cache.value( Meta::valTrackGainPeak ).toDouble();
    if( m_cache.contains( Meta::valAlbumGain ) )
        m_albumGain = m_cache.value( Meta::valAlbumGain ).toDouble();
    if( m_cache.contains( Meta::valAlbumGainPeak ) )
        m_albumPeakGain = m_cache.value( Meta::valAlbumGainPeak ).toDouble();

    if( m_cache.contains( Meta::valUrl ) )
    {
        // slight problem here: it is possible to set the url to the one of an already
        // existing track, which is forbidden by the database
        // At least the ScanResultProcessor handles this problem

        KUrl oldUrl = m_url;
        m_url = m_cache.value( Meta::valUrl ).toString();
        // debug() << "m_cache contains a new URL, setting m_url to " << m_url << " from " << oldUrl;
        m_collection->registry()->updateCachedUrl( oldUrl.path(), m_url.path() );
    }

    if( m_cache.contains( Meta::valArtist ) )
    {
        //invalidate cache of the old artist...
        oldArtist = static_cast<DatabaseArtist*>(m_artist.data());
        m_artist = m_collection->registry()->getArtist( m_cache.value( Meta::valArtist ).toString() );
        //and the new one
        newArtist = static_cast<DatabaseArtist*>(m_artist.data());

        // if the current album is no compilation and we aren't changing
        // the album anyway, then we need to create a new album with the
        // new artist.
        if( m_album &&
            m_album->hasAlbumArtist() &&
            m_album->albumArtist()->name() == oldArtist->name() &&
            !m_cache.contains( Meta::valAlbum ) )
            m_cache.insert( Meta::valAlbum, m_album->name() );

        collectionChanged = true;
    }

    if( m_cache.contains( Meta::valAlbum ) ||
        m_cache.contains( Meta::valAlbumId ) )
    {
        oldAlbum = static_cast<DatabaseAlbum*>(m_album.data());

        if( m_cache.contains( Meta::valAlbumId ) )
            m_album = m_collection->registry()->getAlbum( m_cache.value( Meta::valAlbumId ).toInt() );
        else
        {
            //the album should remain a compilation after renaming it
            QString newArtistName;
            if( oldAlbum && oldAlbum->isCompilation() )
                newArtistName = QString();
            else if( artist() )
                newArtistName = artist()->name();

            m_album = m_collection->registry()->getAlbum( m_cache.value( Meta::valAlbum ).toString(),
                                                          newArtistName );
        }

        newAlbum = static_cast<DatabaseAlbum*>(m_album.data());

        // copy the image BUG: 203211
        // IMPROVEMENT: use setImage(QString) to prevent the image being
        // physically copied.
        // This would also prevent warning messages during scanning when
        // a pixmap is handled from outside the UI thread
        /* TODO for database collection
        if( oldAlbum &&
            oldAlbum->hasImage(0) && !newAlbum->hasImage(0) )
            newAlbum->setImage( oldAlbum->imageLocation(0).path() );
            */

        collectionChanged = true;
    }

    if( m_cache.contains( Meta::valComposer ) )
    {
        oldComposer = static_cast<DatabaseComposer*>(m_composer.data());
        m_composer = m_collection->registry()->getComposer( m_cache.value( Meta::valComposer ).toString() );
        newComposer = static_cast<DatabaseComposer*>(m_composer.data());
        collectionChanged = true;
    }

    if( m_cache.contains( Meta::valGenre ) )
    {
        oldGenre = static_cast<DatabaseGenre*>(m_genre.data());
        m_genre = m_collection->registry()->getGenre( m_cache.value( Meta::valGenre ).toString() );
        newGenre = static_cast<DatabaseGenre*>(m_genre.data());
        collectionChanged = true;
    }

    if( m_cache.contains( Meta::valYear ) )
    {
        oldYear = static_cast<DatabaseYear*>(m_year.data());
        m_year = m_collection->registry()->getYear( m_cache.value( Meta::valYear ).toInt() );
        newYear = static_cast<DatabaseYear*>(m_year.data());
        collectionChanged = true;
    }

    if( m_cache.contains( Meta::valBpm ) )
        m_bpm = m_cache.value( Meta::valBpm ).toDouble();

    // --- write the file
    if( m_writeFile )
        writeMetaDataToFile();

    //updating the fields might have changed the filesize
    //read the current filesize so that we can update the db
    QFile file( m_url.path() );
    if( file.exists() )
    {
        if( m_filesize != file.size() )
        {
            m_cache.insert( Meta::valFilesize, file.size() );
            m_filesize = file.size();
        }

        AFTUtility aftutil;
        QString newUid = QString( "amarok-sqltrackuid://" ) + aftutil.readUniqueId( m_url.path() );
        if( newUid != m_uid || m_cache.contains( Meta::valUniqueId ) )
            m_cache.insert( Meta::valUniqueId, newUid );
    }

    // Use the latest uid here
    if( m_cache.contains( Meta::valUniqueId ) )
    {
        m_uid = m_cache.value( Meta::valUniqueId ).toString();
        m_collection->registry()->updateCachedUid( oldUid, m_uid );
    }

    // --- write the database
    writeUrlToDb( m_cache );
    writeMetaDataToDb( m_cache );
    writeStatisticsToDb( m_cache );
    writePlaylistsToDb( m_cache, oldUid );

    // --- clean up and notify everybody
    m_cache.clear();

    notifyObservers();

    // these calls must be made after the database has been updated
#define INVALIDATE_AND_UPDATE(X) if( X ) \
    { \
        X->invalidateCache(); \
        X->notifyObservers(); \
    }
    INVALIDATE_AND_UPDATE(oldArtist);
    INVALIDATE_AND_UPDATE(newArtist);
    INVALIDATE_AND_UPDATE(oldAlbum);
    INVALIDATE_AND_UPDATE(newAlbum);
    INVALIDATE_AND_UPDATE(oldComposer);
    INVALIDATE_AND_UPDATE(newComposer);
    INVALIDATE_AND_UPDATE(oldGenre);
    INVALIDATE_AND_UPDATE(newGenre);
    INVALIDATE_AND_UPDATE(oldYear);
    INVALIDATE_AND_UPDATE(newYear);
#undef INVALIDATE_AND_UPDATE

    if( collectionChanged )
        m_collection->collectionUpdated();
}

void
DatabaseTrack::writeMetaDataToFile()
{
    Meta::Field::writeFields( m_url.path(), m_cache );
}

void
DatabaseTrack::finishedPlaying( double playedFraction )
{
    beginMetaDataUpdate(); // Batch updates, so we only bother our observers once.

    bool doUpdate = false;

    if( m_length < 60000 ) // less than 1min
        doUpdate = playedFraction >= 0.9;
    else
        doUpdate = playedFraction >= 0.7;

    if( doUpdate )
    {
        setPlayCount( playCount() + 1 );
        if( firstPlayed() == 0 )
            setFirstPlayed( QDateTime::currentDateTime().toTime_t() );
        setLastPlayed( QDateTime::currentDateTime().toTime_t() );
    }

    setScore( Amarok::computeScore( score(), playCount(), playedFraction ) );

    endMetaDataUpdate();
}

bool
DatabaseTrack::inCollection() const
{
    return m_trackId > 0;
}

Collections::Collection*
DatabaseTrack::collection() const
{
    return m_collection;
}

bool
DatabaseTrack::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    return m_collection->trackCapabilityDelegate() &&
        m_collection->trackCapabilityDelegate()->hasCapabilityInterface( type, this );
}

Capabilities::Capability*
DatabaseTrack::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_collection->trackCapabilityDelegate() )
        return m_collection->trackCapabilityDelegate()->createCapabilityInterface( type, this );
    return 0;
}

void
DatabaseTrack::addLabel( const QString &label )
{
    Meta::LabelPtr realLabel = m_collection->registry()->getLabel( label );
    addLabel( realLabel );
}

Meta::LabelList
DatabaseTrack::labels() const
{
    if( m_labelsInCache )
    {
        return m_labelsCache;
    }
    else if( m_collection )
    {
        Collections::DatabaseQueryMaker *qm = static_cast< Collections::DatabaseQueryMaker* >( m_collection->queryMaker() );
        qm->setQueryType( Collections::QueryMaker::Label );
        const_cast<DatabaseTrack*>( this )->addMatchTo( qm );
        qm->setBlocking( true );
        qm->run();

        m_labelsInCache = true;
        m_labelsCache = qm->labels();

        delete qm;
        return m_labelsCache;
    }
    else
    {
        return Meta::LabelList();
    }
}

//---------------------- class Artist --------------------------

DatabaseArtist::DatabaseArtist( Collections::DatabaseCollection* collection, int id, const QString &name ) : Artist()
    ,m_collection( QPointer<Collections::DatabaseCollection>( collection ) )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_albumsLoaded( false )
    ,m_mutex( QMutex::Recursive )
{
    Q_ASSERT( m_collection );
    Q_ASSERT( m_id > 0 );
}

Meta::DatabaseArtist::~DatabaseArtist()
{
}

void
DatabaseArtist::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
DatabaseArtist::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
        return m_tracks;

    Collections::DatabaseQueryMaker *qm = static_cast< Collections::DatabaseQueryMaker* >( m_collection->queryMaker() );
    qm->setQueryType( Collections::QueryMaker::Track );
    addMatchTo( qm );
    qm->setBlocking( true );
    qm->run();
    m_tracks = qm->tracks( m_collection->collectionId() );
    delete qm;
    m_tracksLoaded = true;
    return m_tracks;
}

AlbumList
DatabaseArtist::albums()
{
    QMutexLocker locker( &m_mutex );
    if( m_albumsLoaded )
        return m_albums;

    Collections::DatabaseQueryMaker *qm = static_cast< Collections::DatabaseQueryMaker* >( m_collection->queryMaker() );
    qm->setQueryType( Collections::QueryMaker::Album );
    addMatchTo( qm );
    qm->setBlocking( true );
    qm->run();
    m_albums = qm->albums( m_collection->collectionId() );
    delete qm;
    m_albumsLoaded = true;
    return m_albums;
}

bool
DatabaseArtist::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    return m_collection->artistCapabilityDelegate() &&
        m_collection->artistCapabilityDelegate()->hasCapabilityInterface( type, this );
}

Capabilities::Capability*
DatabaseArtist::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_collection->artistCapabilityDelegate() )
        m_collection->artistCapabilityDelegate()->createCapabilityInterface( type, this );
    return 0;
}


//---------------DatabaseAlbum---------------------------------
const QString DatabaseAlbum::AMAROK_UNSET_MAGIC = QString( "AMAROK_UNSET_MAGIC" );

DatabaseAlbum::DatabaseAlbum( Collections::DatabaseCollection* collection, int id, const QString &name, int artist ) : Album()
    , m_collection( QPointer<Collections::DatabaseCollection>( collection ) )
    , m_name( name )
    , m_id( id )
    , m_artistId( artist )
    , m_imageId( -1 )
    , m_hasImage( false )
    , m_hasImageChecked( false )
    , m_unsetImageId( -1 )
    , m_tracksLoaded( false )
    , m_suppressAutoFetch( false )
    , m_mutex( QMutex::Recursive )
{
    Q_ASSERT( m_collection );
    Q_ASSERT( m_id > 0 );
}

Meta::DatabaseAlbum::~DatabaseAlbum()
{
}

void
DatabaseAlbum::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_hasImage = false;
    m_hasImageChecked = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
DatabaseAlbum::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
        return m_tracks;

    Collections::DatabaseQueryMaker *qm = static_cast< Collections::DatabaseQueryMaker* >( m_collection->queryMaker() );
    qm->setQueryType( Collections::QueryMaker::Track );
    addMatchTo( qm );
    qm->orderBy( Meta::valDiscNr );
    qm->orderBy( Meta::valTrackNr );
    qm->orderBy( Meta::valTitle );
    qm->setBlocking( true );
    qm->run();
    m_tracks = qm->tracks( m_collection->collectionId() );
    delete qm;
    m_tracksLoaded = true;
    return m_tracks;
}

bool
DatabaseAlbum::isCompilation() const
{
    return !hasAlbumArtist();
}

bool
DatabaseAlbum::hasAlbumArtist() const
{
    return albumArtist();
}

Meta::ArtistPtr
DatabaseAlbum::albumArtist() const
{
    if( m_artistId > 0 && !m_artist )
    {
        const_cast<DatabaseAlbum*>( this )->m_artist =
            m_collection->registry()->getArtist( m_artistId );
    }
    return m_artist;
}


void
DatabaseAlbum::setCompilation( bool compilation )
{
    if( isCompilation() == compilation )
    {
        return;
    }
    else
    {
        m_collection->blockUpdatedSignal();

        if( compilation )
        {
            // get the new compilation album
            Meta::AlbumPtr metaAlbum = m_collection->registry()->getAlbum( name(), QString() );
            KSharedPtr<DatabaseAlbum> databaseAlbum = KSharedPtr<DatabaseAlbum>::dynamicCast( metaAlbum );

            Meta::FieldHash changes;
            changes.insert( Meta::valCompilation, 1);

            Meta::TrackList myTracks = tracks();
            foreach( Meta::TrackPtr metaTrack, myTracks )
            {
                KSharedPtr<DatabaseTrack> databaseTrack = KSharedPtr<DatabaseTrack>::dynamicCast( metaTrack );

                // copy over the cover image
                // TODO

                // move the track
                databaseTrack->setAlbum( databaseAlbum->id() );
                Meta::Field::writeFields( databaseTrack->playableUrl().path(), changes );
            }
            /* TODO: delete all old tracks albums */
        }
        else
        {
            Meta::FieldHash changes;
            changes.insert( Meta::valCompilation, 0);

            Meta::TrackList myTracks = tracks();
            foreach( Meta::TrackPtr metaTrack, myTracks )
            {
                KSharedPtr<DatabaseTrack> databaseTrack = KSharedPtr<DatabaseTrack>::dynamicCast( metaTrack );

                // get the new album
                Meta::AlbumPtr metaAlbum = m_collection->registry()->getAlbum(
                    databaseTrack->album()->name(),
                    ArtistHelper::realTrackArtist( databaseTrack->artist()->name() ) );
                KSharedPtr<DatabaseAlbum> databaseAlbum = KSharedPtr<DatabaseAlbum>::dynamicCast( metaAlbum );

                // copy over the cover image
                // TODO

                // move the track
                databaseTrack->setAlbum( databaseAlbum->id() );
                Meta::Field::writeFields( databaseTrack->playableUrl().path(), changes );
            }
            /* TODO //step 5: delete the original album, if necessary */
        }

        m_collection->unblockUpdatedSignal();
    }
}

bool
DatabaseAlbum::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    return m_collection->albumCapabilityDelegate() &&
        m_collection->albumCapabilityDelegate()->hasCapabilityInterface( type, this );
}

Capabilities::Capability*
DatabaseAlbum::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_collection->albumCapabilityDelegate() )
        return m_collection->albumCapabilityDelegate()->createCapabilityInterface( type, this );
    return 0;
}

//---------------DatabaseComposer---------------------------------

DatabaseComposer::DatabaseComposer( Collections::DatabaseCollection* collection, int id, const QString &name ) : Composer()
    ,m_collection( QPointer<Collections::DatabaseCollection>( collection ) )
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_mutex( QMutex::Recursive )
{
    //nothing to do
}

void
DatabaseComposer::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
DatabaseComposer::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else if( m_collection )
    {
        Collections::DatabaseQueryMaker *qm = static_cast< Collections::DatabaseQueryMaker* >( m_collection->queryMaker() );
        qm->setQueryType( Collections::QueryMaker::Track );
        addMatchTo( qm );
        qm->setBlocking( true );
        qm->run();
        m_tracks = qm->tracks( m_collection->collectionId() );
        delete qm;
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

//---------------DatabaseGenre---------------------------------

DatabaseGenre::DatabaseGenre( Collections::DatabaseCollection* collection, int id, const QString &name ) : Genre()
    ,m_collection( QPointer<Collections::DatabaseCollection>( collection ) )
    ,m_id( id )
    ,m_name( name )
    ,m_tracksLoaded( false )
    ,m_mutex( QMutex::Recursive )
{
    //nothing to do
}

void
DatabaseGenre::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
DatabaseGenre::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else if( m_collection )
    {
        Collections::DatabaseQueryMaker *qm = static_cast< Collections::DatabaseQueryMaker* >( m_collection->queryMaker() );
        qm->setQueryType( Collections::QueryMaker::Track );
        addMatchTo( qm );
        qm->setBlocking( true );
        qm->run();
        m_tracks = qm->tracks( m_collection->collectionId() );
        delete qm;
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

//---------------DatabaseYear---------------------------------

DatabaseYear::DatabaseYear( Collections::DatabaseCollection* collection, int id, int year) : Year()
    ,m_collection( QPointer<Collections::DatabaseCollection>( collection ) )
    ,m_id( id )
    ,m_year( year )
    ,m_tracksLoaded( false )
    ,m_mutex( QMutex::Recursive )
{
    //nothing to do
}

void
DatabaseYear::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
DatabaseYear::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else if( m_collection )
    {
        Collections::DatabaseQueryMaker *qm = static_cast< Collections::DatabaseQueryMaker* >( m_collection->queryMaker() );
        qm->setQueryType( Collections::QueryMaker::Track );
        addMatchTo( qm );
        qm->setBlocking( true );
        qm->run();
        m_tracks = qm->tracks( m_collection->collectionId() );
        delete qm;
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

//---------------DatabaseLabel---------------------------------

DatabaseLabel::DatabaseLabel( Collections::DatabaseCollection *collection, int id, const QString &name ) : Meta::Label()
    ,m_collection( QPointer<Collections::DatabaseCollection>( collection ) )
    ,m_id( id )
    ,m_name( name )
    ,m_tracksLoaded( false )
    ,m_mutex( QMutex::Recursive )
{
    //nothing to do
}

void
DatabaseLabel::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
DatabaseLabel::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else if( m_collection )
    {
        Collections::DatabaseQueryMaker *qm = static_cast< Collections::DatabaseQueryMaker* >( m_collection->queryMaker() );
        qm->setQueryType( Collections::QueryMaker::Track );
        addMatchTo( qm );
        qm->setBlocking( true );
        qm->run();
        m_tracks = qm->tracks();
        delete qm;
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList();
}

