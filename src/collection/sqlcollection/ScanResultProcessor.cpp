/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ScanResultProcessor.h"

#include "Debug.h"
#include "meta/MetaConstants.h"
#include "meta/MetaUtility.h"
#include "MountPointManager.h"
#include "SqlCollection.h"

#include <QDir>
#include <QFileInfo>
#include <QListIterator>


using namespace Meta;

ScanResultProcessor::ScanResultProcessor( SqlCollection *collection )
    : m_collection( collection )
    , m_setupComplete( false )
    , m_type( FullScan )
    , m_aftPermanentTablesUrlString()
{
    DEBUG_BLOCK

    m_aftPermanentTablesUrlString << "playlist_tracks";
}

ScanResultProcessor::~ScanResultProcessor()
{
    DEBUG_BLOCK
}

void
ScanResultProcessor::setScanType( ScanType type )
{
    m_type = type;
}

void
ScanResultProcessor::addDirectory( const QString &dir, uint mtime )
{
    if( dir.isEmpty() )
    {
        debug() << "got directory with no path from the scanner, not adding";
        return;
    }
    setupDatabase();
    int deviceId = MountPointManager::instance()->getIdForUrl( dir );
    QString rdir = MountPointManager::instance()->getRelativePath( deviceId, dir );
    QString query = QString( "SELECT         id, changedate               "
                             "FROM           directories_temp             "
                             "WHERE          deviceid = %1 AND dir = '%2';" )
                        .arg( QString::number( deviceId ), m_collection->escape( rdir ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO directories_temp(deviceid,changedate,dir) VALUES (%1,%2,'%3');" )
                        .arg( QString::number( deviceId ), QString::number( mtime ),
                                m_collection->escape( rdir ) );
        int id = m_collection->insert( insert, "directories_temp" );
        m_directories.insert( dir, id );
    }
    else
    {
        if( res[1].toUInt() != mtime )
        {
            QString update = QString( "UPDATE directories_temp SET changedate = %1 WHERE id = %2;" )
                                .arg( QString::number( mtime ), res[0] );
            m_collection->query( update );
        }
        m_directories.insert( dir, res[0].toInt() );
        m_collection->dbUpdater()->removeFilesInDirFromTemporaryTables( deviceId, rdir );
    }
}

void
ScanResultProcessor::addImage( const QString &path, const QList< QPair<QString, QString> > covers )
{
    QList< QPair<QString,QString> >::ConstIterator it = covers.begin();
    for( ; it != covers.end(); ++it )
    {
        QPair<QString,QString> key = (*it);
        if( key.first.isEmpty() || key.second.isEmpty() )
            continue;

        int artist = artistId( key.first );
        int album  = albumId( key.second, artist );

        // Will automatically add the image path to the database if needed
        imageId( path, album );
    }
}

void
ScanResultProcessor::commit()
{
    if( !m_setupComplete )
    {
        debug() << "ERROR: Database temporary table setup did not complete.  This is probably a result of no directories being scanned.";
        return;
    }
    if( m_type == ScanResultProcessor::IncrementalScan )
    {
        foreach( const QString &dir, m_directories.keys() )
        {
            int deviceid = MountPointManager::instance()->getIdForUrl( dir );
            const QString rpath = MountPointManager::instance()->getRelativePath( deviceid, dir );
            m_collection->dbUpdater()->removeFilesInDir( deviceid, rpath );
        }
    }
    else
    {
        m_collection->dbUpdater()->cleanPermanentTables();
    }
    debug() << "temp_tracks: " << m_collection->query("select count(*) from tracks_temp");
    debug() << "tracks before commit: " << m_collection->query("select count(*) from tracks");
    m_collection->dbUpdater()->copyToPermanentTables();
    debug() << "tracks after commit: " << m_collection->query("select count(*) from tracks");
    m_collection->dbUpdater()->removeTemporaryTables();
    if( m_type == ScanResultProcessor::IncrementalScan )
    {
        m_collection->dbUpdater()->deleteAllRedundant( "album" );
        m_collection->dbUpdater()->deleteAllRedundant( "artist" );
        m_collection->dbUpdater()->deleteAllRedundant( "genre" );
        m_collection->dbUpdater()->deleteAllRedundant( "composer" );
        m_collection->dbUpdater()->deleteAllRedundant( "year" );
    }
    debug() << "Sending changed signal";
    m_collection->sendChangedSignal();

    connect( this, SIGNAL( changedTrackUrlsUids( const ChangedTrackUrls &, const TrackUrls & ) ),
             CollectionManager::instance()->primaryCollection(), SLOT( updateTrackUrlsUids( const ChangedTrackUrls &, const TrackUrls & ) ) );

    emit changedTrackUrlsUids( m_changedUrls, m_changedUids );
}

void
ScanResultProcessor::rollback()
{
    m_collection->dbUpdater()->removeTemporaryTables();
}

void
ScanResultProcessor::processDirectory( const QList<QVariantMap > &data )
{
    setupDatabase();
    //using the following heuristics:
    //if more than one album is in the dir, use the artist of each track as albumartist
    //if all tracks have the same artist, use it as albumartist
    //try to find the albumartist A: tracks must have the artist A or A feat. B (and variants)
    //if no albumartist could be found, it's a compilation

    QSet<QString> artists;
    QString album;
    bool multipleAlbums = false;
    if( !data.isEmpty() )
        album = data[0].value( Field::ALBUM ).toString();

    foreach( const QVariantMap &row, data )
    {
        artists.insert( row.value( Field::ARTIST ).toString() );
        if( row.value( Field::ALBUM ).toString() != album )
            multipleAlbums = true;
    }
    if( multipleAlbums || album.isEmpty() || artists.size() == 1 )
    {
        foreach( const QVariantMap &row, data )
        {
            int artist = artistId( row.value( Field::ARTIST ).toString() );
            addTrack( row, artist );
        }
    }
    else
    {
        QString albumArtist = findAlbumArtist( artists, data.count() );
        //an empty string means that no albumartist was found
        int artist = albumArtist.isEmpty() ? 0 : artistId( albumArtist );

        debug() << "albumartist " << albumArtist << "for artists" << artists;
        foreach( const QVariantMap &row, data )
        {
            addTrack( row, artist );
        }
    }
}

QString
ScanResultProcessor::findAlbumArtist( const QSet<QString> &artists, int trackCount ) const
{
    QMap<QString, int> artistCount;
    bool featuring;
    QStringList trackArtists;
    foreach( const QString &artist, artists )
    {
        featuring = false;
        trackArtists.clear();
        if( artist.contains( "featuring" ) )
        {
            featuring = true;
            trackArtists = artist.split( "featuring" );
        }
        else if( artist.contains( "feat." ) )
        {
            featuring = true;
            trackArtists = artist.split( "feat." );
        }
        else if( artist.contains( "ft." ) )
        {
            featuring = true;
            trackArtists = artist.split( "ft." );
        }
        else if( artist.contains( "f." ) )
        {
            featuring = true;
            trackArtists = artist.split( "f." );
        }

        //this needs to be improved

        if( featuring )
        {
            //always use the first artist
            QString tmp = trackArtists[0].simplified();
            if( tmp.isEmpty() )
            {
                //TODO error handling
            }
            else
            {
                if( artistCount.contains( tmp ) )
                    artistCount.insert( tmp, artistCount.value( tmp ) + 1 );
                else
                    artistCount.insert( tmp, 1 );
            }
        }
        else
        {
            if( artistCount.contains( artist ) )
                artistCount.insert( artist, artistCount.value( artist ) + 1 );
            else
                artistCount.insert( artist, 1 );
        }
    }
    QString albumArtist;
    int count = 0;
    foreach( const QString &key, artistCount.keys() )
    {
        if( artistCount.value( key ) > count )
        {
            albumArtist = key;
            count = artistCount.value( key );
        }
    }
    //if an artist is the primary artist of each track in the directory, assume the artist is the albumartist
    return count == trackCount ? albumArtist : QString();
}

void
ScanResultProcessor::addTrack( const QVariantMap &trackData, int albumArtistId )
{
//     DEBUG_BLOCK
    //amarok 1 stored all tracks of a compilation in different directories.
    //when using its "Organize Collection" feature
    //try to detect these cases
    QString albumName = trackData.value( Field::ALBUM ).toString();
    int album = 0;

    QString path = trackData.value( Field::URL ).toString();

    QFileInfo file( path );

    QDir dir = file.dir();
    dir.setFilter( QDir::Files );

    //name filtering should be case-insensitive because we do not use QDir::CaseSensitive
    QStringList filters;
    filters << "*.mp3" << "*.ogg" << "*.oga" << "*.flac" << "*.wma" << "*.m4a";
    dir.setNameFilters( filters );

    int compilationId = 0;

    //do not check existing albums if there is more than one file in the directory
    //see comments in checkExistingAlbums

    //TODO: find a better way to ignore non-audio files than the extension matching above
    if( !m_filesInDirs.contains( dir.absolutePath() ) )
    {
        m_filesInDirs.insert( dir.absolutePath(), dir.count() );
    }

    if( dir.count() == 1 )
    {
        compilationId = checkExistingAlbums( albumName );
    }

    if( 0 == compilationId )
    {
        album = albumId( albumName, albumArtistId );
    }

    int artist = artistId( trackData.value( Field::ARTIST ).toString() );
    int genre = genreId( trackData.value( Field::GENRE ).toString() );
    int composer = composerId( trackData.value( Field::COMPOSER ).toString() );
    int year = yearId( trackData.value( Field::YEAR ).toString() );

    QString uid = trackData.value( Field::UNIQUEID ).toString();

    const int created  = file.created().toTime_t();
    const int modified = file.lastModified().toTime_t();

    //urlId will take care of the urls table part of AFT
    int url = urlId( path, uid );

    QString sql,sql2,sql3;
    sql = "REPLACE INTO tracks_temp(url,artist,album,genre,composer,year,title,comment,"
                    "tracknumber,discnumber,bitrate,length,samplerate,filesize,filetype,bpm,"
                    "createdate,modifydate,albumgain,albumpeakgain,trackgain,trackpeakgain) "
                    "VALUES ( %1,%2,%3,%4,%5,%6,'%7','%8',%9"; //goes up to tracknumber
    sql = sql.arg( QString::number( url )
                , QString::number( artist )
                , QString::number( compilationId ? compilationId : album )
                , QString::number( genre )
                , QString::number( composer )
                , QString::number( year )
                , m_collection->escape( trackData[ Field::TITLE ].toString() )
                , m_collection->escape( trackData[ Field::COMMENT ].toString() )
                , QString::number( trackData[Field::TRACKNUMBER].toInt() ) );

    sql2 = ",%1,%2,%3,%4,%5,%6,%7,%8,%9"; // goes up to modifydate
    sql2 = sql2.arg( QString::number( trackData[Field::DISCNUMBER].toInt() )
                , QString::number( trackData[Field::BITRATE].toInt() )
                , QString::number( trackData[Field::LENGTH].toInt() )
                , QString::number( trackData[Field::SAMPLERATE].toInt() )
                , QString::number( trackData[Field::FILESIZE].toInt() )
                , "0" // NYI: filetype
                , "0" // NYI: bpm
                , QString::number( created )
                , QString::number( modified ) );

    // replay gain - only store gain values if we also have peak gain values
    //               (ie: ignore broken tags)
    sql3 = ",%1,%2,%3,%4);";
    if ( trackData.contains( Field::ALBUMGAIN ) && trackData.contains( Field::ALBUMPEAKGAIN ) )
    {
        sql3 = sql3.arg( QString::number( trackData[ Field::ALBUMGAIN ].toDouble() ) );
        sql3 = sql3.arg( trackData[ Field::ALBUMPEAKGAIN ].toDouble() );
    }
    else
        sql3 = sql3.arg( "NULL", "NULL" );
    if ( trackData.contains( Field::TRACKGAIN ) && trackData.contains( Field::TRACKPEAKGAIN ) )
    {
        sql3 = sql3.arg( QString::number( trackData[ Field::TRACKGAIN ].toDouble() ) );
        sql3 = sql3.arg( trackData[ Field::TRACKPEAKGAIN ].toDouble() );
    }
    else
        sql3 = sql3.arg( "NULL", "NULL" );

    sql += sql2 + sql3;

    m_collection->query( sql );
}

int
ScanResultProcessor::artistId( const QString &artist )
{
    if( m_artists.contains( artist ) )
        return m_artists.value( artist );
    QString query = QString( "SELECT id FROM artists_temp WHERE name = '%1';" ).arg( m_collection->escape( artist ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO artists_temp( name ) VALUES ('%1');" ).arg( m_collection->escape( artist ) );
        int id = m_collection->insert( insert, "artists_temp" );
        m_artists.insert( artist, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_artists.insert( artist, id );
        return id;
    }
}

int
ScanResultProcessor::genreId( const QString &genre )
{
    if( m_genre.contains( genre ) )
        return m_genre.value( genre );
    QString query = QString( "SELECT id FROM genres_temp WHERE name = '%1';" ).arg( m_collection->escape( genre ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO genres_temp( name ) VALUES ('%1');" ).arg( m_collection->escape( genre ) );
        int id = m_collection->insert( insert, "genres_temp" );
        m_genre.insert( genre, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_genre.insert( genre, id );
        return id;
    }
}

int
ScanResultProcessor::composerId( const QString &composer )
{
    if( m_composer.contains( composer ) )
        return m_composer.value( composer );
    QString query = QString( "SELECT id FROM composers_temp WHERE name = '%1';" ).arg( m_collection->escape( composer ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO composers_temp( name ) VALUES ('%1');" ).arg( m_collection->escape( composer ) );
        int id = m_collection->insert( insert, "composers_temp" );
        m_composer.insert( composer, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_composer.insert( composer, id );
        return id;
    }
}

int
ScanResultProcessor::yearId( const QString &year )
{
    if( m_year.contains( year ) )
        return m_year.value( year );
    QString query = QString( "SELECT id FROM years_temp WHERE name = '%1';" ).arg( m_collection->escape( year ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO years_temp( name ) VALUES ('%1');" ).arg( m_collection->escape( year ) );
        int id = m_collection->insert( insert, "years_temp" );
        m_year.insert( year, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_year.insert( year, id );
        return id;
    }
}

int
ScanResultProcessor::imageId( const QString &image, int albumId )
{
    // assume the album is valid
    if( albumId < 0 )
        return -1;

    QPair<QString, int> key( image, albumId );
    if( m_images.contains( key ) )
        return m_images.value( key );

    QString query = QString( "SELECT images_temp.id FROM images_temp WHERE images_temp.path = '%1'" )
                        .arg( m_collection->escape( image ) );
    QStringList res = m_collection->query( query );
    int imageId = -1;
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO images_temp( path ) VALUES ('%1');" ).arg( m_collection->escape( image ) );
        imageId = m_collection->insert( insert, "images_temp" );
    }
    else
        imageId = res[0].toInt();

    if( imageId >= 0 )
    {
        // Make sure the album table is up to date
        QString update = QString( "UPDATE albums_temp SET image = %1 WHERE id = %2" )
                            .arg( QString::number( imageId ), QString::number( albumId ) );
        m_collection->query( update );
        m_images.insert( key, imageId );
    }

    return imageId;
}

int
ScanResultProcessor::albumId( const QString &album, int artistId )
{
    //artistId == 0 means no albumartist
    QPair<QString, int> key( album, artistId );
    if( m_albums.contains( key ) )
    {
        // if we already have the key but the artist == 0,
        // UPDATE the image field so that we won't forget the cover for a compilation
        int id = m_albums.value( key );
        if ( artistId == 0 )
        {
            QString select = QString( "SELECT MAX(image) FROM albums_temp WHERE name = '%1';" )
                .arg( m_collection->escape( album ) );
            QStringList res = m_collection->query( select );
            if( !res.isEmpty() && !res[0].isEmpty() )
            {
                QString update = QString( "UPDATE albums_temp SET image = %1 WHERE id = %2" )
                    .arg( res[0] , QString::number( id ) );
                m_collection->query( update );
            }
        }
        return id;
    }

    QString query;
    if( artistId == 0 )
    {
        query = QString( "SELECT id FROM albums_temp WHERE artist IS NULL AND name = '%1';" )
                    .arg( m_collection->escape( album ) );
    }
    else
    {
        query = QString( "SELECT id FROM albums_temp WHERE artist = %1 AND name = '%2';" )
                        .arg( QString::number( artistId ), m_collection->escape( album ) );
    }
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO albums_temp(artist, name) VALUES( %1, '%2' );" )
                    .arg( artistId ? QString::number( artistId ) : "NULL", m_collection->escape( album ) );
        int id = m_collection->insert( insert, "albums_temp" );
        m_albums.insert( key, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_albums.insert( key, id );
        return id;
    }
}

int
ScanResultProcessor::urlId( const QString &url, const QString &uid )
{
    int deviceId = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceId, url );
    //don't bother caching the data, we only call this method for each url once
    QString query = QString( "SELECT id, deviceid, rpath, uniqueid FROM urls_temp WHERE deviceid = %1 AND rpath = '%2';" )
                        .arg( QString::number( deviceId ), m_collection->escape( rpath ) );
    QStringList pathres = m_collection->query( query ); //tells us if the path existed
    query = QString( "SELECT id, deviceid, rpath, uniqueid FROM urls_temp WHERE uniqueid='%1';" )
                        .arg( m_collection->escape( uid ) );
    QStringList uidres = m_collection->query( query ); //tells us if the uid existed
    if( pathres.isEmpty() && uidres.isEmpty() ) //fresh -- insert
    {
        QFileInfo fileInfo( url );
        const QString dir = fileInfo.absoluteDir().absolutePath();
        int dirId = directoryId( dir );
        QString insert = QString( "INSERT INTO urls_temp(directory,deviceid,rpath,uniqueid) VALUES ( %1, %2, '%3', '%4' );" )
                    .arg( QString::number( dirId ), QString::number( deviceId ), m_collection->escape( rpath ),
                              m_collection->escape( uid ) );
        return m_collection->insert( insert, "urls_temp" );
    }
    else if( !uidres.isEmpty() )
    {
        //we found an existing entry with this uniqueid, update the deviceid and path
        //Note that we ignore the situation where both a UID and path was found; UID takes precedence
        QFileInfo fileInfo( url );
        const QString dir = fileInfo.absoluteDir().absolutePath();
        int dirId = directoryId( dir );
        QString query = QString( "UPDATE urls_temp SET directory=%1,deviceid=%2,rpath='%3' WHERE uniqueid='%4';" )
            .arg( QString::number( dirId ), QString::number( deviceId ), m_collection->escape( rpath ),
                            m_collection->escape( uid ) );
        m_collection->query( query );
        updateAftPermanentTablesUrlString( url, uid );
        m_changedUrls.insert( uid, QPair<QString, QString>( MountPointManager::instance()->getAbsolutePath( uidres[1].toInt(), uidres[2] ), url ) );
        return uidres[0].toInt();
    }
    else if( !pathres.isEmpty() )
    {
        //We found an existing path; give it the most recent UID value
        QString query = QString( "UPDATE urls_temp SET uniqueid='%1' WHERE deviceid=%2 AND rpath='%3';" )
            .arg( uid, QString::number( deviceId ), m_collection->escape( rpath ) );
        m_collection->query( query );
        updateAftPermanentTablesUidString( url, uid );
        m_changedUids.insert( pathres[3], uid ); 
        return pathres[0].toInt();
    }
    else
        debug() << "AFT algorithm died...you should not be here!  Returning something negative and bad.";
    return -666;
}

void
ScanResultProcessor::updateAftPermanentTablesUrlString( const QString &url, const QString &uid )
{
    foreach( const QString &table, m_aftPermanentTablesUrlString )
    {
        QString query = QString( "UPDATE %1 SET url='%2' WHERE uniqueid='%3';" )
            .arg( table, m_collection->escape( url ), m_collection->escape( uid ) );
        m_collection->query( query );
    }
}

void
ScanResultProcessor::updateAftPermanentTablesUidString( const QString &url, const QString &uid )
{
    foreach( const QString &table, m_aftPermanentTablesUrlString )
    {
        QString query = QString( "UPDATE %1 SET uniqueid='%2' WHERE url='%3';" )
            .arg( table, m_collection->escape( uid ), m_collection->escape( url ) );
        m_collection->query( query );
    }
}

int
ScanResultProcessor::directoryId( const QString &dir )
{
    if( m_directories.contains( dir ) )
        return m_directories.value( dir );

    int deviceId = MountPointManager::instance()->getIdForUrl( dir );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceId, dir );
    if( !rpath.endsWith( '/' ) )
    {
        rpath += '/';
    }
    QString query = QString( "SELECT id, changedate FROM directories_temp WHERE deviceid = %1 AND dir = '%2';" )
                        .arg( QString::number( deviceId ), m_collection->escape( rpath ) );
    QStringList result = m_collection->query( query );
    if( result.isEmpty() )
    {
        return 0;
    }
    else
    {
        m_directories.insert( dir, result[0].toInt() );
        return result[0].toInt();
    }
}

int
ScanResultProcessor::checkExistingAlbums( const QString &album )
{
    // "Unknown" albums shouldn't be handled as compilations
    if( album.isEmpty() )
        return 0;

    //check if this album already exists, ignoring the albumartist
    //if it does, and if each file of the album is alone in its directory
    //it's probably a compilation.
    //this handles A1 compilations that were automatically organized by Amarok
    QString query = "SELECT urls.deviceid,urls.rpath,tracks_temp.id,albums.id,albums.artist FROM urls_temp AS urls "
                    "LEFT JOIN tracks_temp on urls.id = tracks_temp.url LEFT JOIN albums_temp AS albums ON "
                    "tracks_temp.album = albums.id WHERE albums.name = '%1';";
    query = query.arg( m_collection->escape( album ) );
    QStringList result = m_collection->query( query );
    QList<QString> trackIds;
    for( QListIterator<QString> iter( result ); iter.hasNext(); )
    {
        int deviceid = iter.next().toInt();
        QString rpath = iter.next();
        QString trackId = iter.next();
        QString albumId = iter.next();
        QString albumArtistId = iter.next();
        QString currentPath = MountPointManager::instance()->getAbsolutePath( deviceid, rpath );
        QFileInfo info( currentPath );
        uint dirCount = m_filesInDirs.value( info.dir().absolutePath() );
        if( dirCount == 1 )
        {
            trackIds << trackId;
        }
    }

    if( trackIds.isEmpty() )
    {
        return 0;
    }
    else
    {
        int compilationId = albumId( album, 0 );
        QString trackIdsSql = "-1";
        foreach( const QString &trackId, trackIds )
        {
            trackIdsSql += ',';
            trackIdsSql += trackId;
        }
        QString update = "UPDATE tracks_temp SET album = %1 where id IN (%2);";
        m_collection->query( update.arg( QString::number( compilationId ), trackIdsSql ) );
        return compilationId;
    }
}

void
ScanResultProcessor::setupDatabase()
{
    if( !m_setupComplete )
    {
        m_collection->dbUpdater()->createTemporaryTables();
        if( m_type == IncrementalScan )
        {
            m_collection->dbUpdater()->prepareTemporaryTables();
        }
        else
        {
            m_collection->dbUpdater()->prepareTemporaryTablesForFullScan();
        }
        m_setupComplete = true;
    }
}

#include "ScanResultProcessor.moc"

