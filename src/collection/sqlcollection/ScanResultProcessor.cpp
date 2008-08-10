/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *  Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>
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

#include "ScanResultProcessor.h"

#include "Debug.h"
#include "meta/MetaConstants.h"
#include "meta/MetaUtility.h"
#include "mountpointmanager.h"
#include "SqlCollection.h"

#include <QDir>
#include <QFileInfo>
#include <QListIterator>

#include <KUrl>

using namespace Meta;

ScanResultProcessor::ScanResultProcessor( SqlCollection *collection )
    : m_collection( collection )
    , m_setupComplete( false )
    , m_filesDeleted( 0 )
    , m_type( FullScan )
    , m_aftPermanentTables()
{
    DEBUG_BLOCK
    m_aftPermanentTables << "statistics" << "lyrics" << "playlist_tracks";
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
ScanResultProcessor::setFilesDeletedHash( QHash<QString, QString>* hash )
{
    m_filesDeleted = hash;
}

void
ScanResultProcessor::setChangedUrlsHash( QHash<QString, QString>* hash )
{
    m_changedUrls = hash;
}

void
ScanResultProcessor::addDirectory( const QString &dir, uint mtime )
{
    setupDatabase();
    int deviceId = MountPointManager::instance()->getIdForUrl( dir );
    QString rdir = MountPointManager::instance()->getRelativePath( deviceId, dir );
    QString query = QString( "SELECT id, changedate FROM directories_temp WHERE deviceid = %1 AND dir = '%2';" )
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
    for( ; it != covers.end(); it++ )
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
    if( m_type == ScanResultProcessor::IncrementalScan )
    {
        foreach( const QString &dir, m_directories.keys() )
        {
            debug() << "removing " << dir << " from database";
            int deviceid = MountPointManager::instance()->getIdForUrl( dir );
            const QString rpath = MountPointManager::instance()->getRelativePath( deviceid, dir );
            m_collection->dbUpdater()->removeFilesInDir( deviceid, rpath, m_filesDeleted );
        }
    }
    else
    {
        m_collection->dbUpdater()->cleanPermanentTables();
    }
    m_collection->dbUpdater()->copyToPermanentTables();
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
    //if more than 60 files are in the dir, use the artist of each track as albumartist
    //if all tracks have the same artist, use it as albumartist
    //try to find the albumartist A: tracks must have the artist A or A feat. B (and variants)
    //if no albumartist could be found, it's a compilation

    QSet<QString> artists;
    QString album;
    bool multipleAlbums = false;
    if( !data.isEmpty() )
        album = data[0].value( Field::ALBUM ).toString();
    QVariantMap row;
    foreach( row, data )
    {
        artists.insert( row.value( Field::ARTIST ).toString() );
        if( row.value( Field::ALBUM ).toString() != album )
            multipleAlbums = true;
    }
    if( multipleAlbums || album.isEmpty() || data.count() > 60 || artists.size() == 1 )
    {
        QVariantMap row;
        foreach( row, data )
        {
            int artist = artistId( row.value( Field::ARTIST ).toString() );
            addTrack( row, artist );
        }
    }
    else
    {
        QString albumArtist = findAlbumArtist( artists );
        //an empty string means that no albumartist was found
        int artist = albumArtist.isEmpty() ? 0 : artistId( albumArtist );
        QVariantMap row;
        foreach( row, data )
        {
            addTrack( row, artist );
        }
    }
}

QString
ScanResultProcessor::findAlbumArtist( const QSet<QString> &artists ) const
{
    QMap<QString, int> artistCount;
    foreach( const QString &artist, artists )
    {
        //this needs to be improved
        if( artist.contains( "featuring" ) )
        {
            QStringList trackArtists = artist.split( "featuring" );
            //always use the first artist
            QString tmp = trackArtists[0].simplified();
            if( tmp.isEmpty() )
            {
                //TODO error handling
            }
            else
            {
                if( artistCount.contains( tmp ) )
                {
                    artistCount.insert( tmp, artistCount.value( tmp ) + 1 );
                }
                else
                {
                    artistCount.insert( tmp, 1 );
                }
            }
        }
        else if( artist.contains( "feat." ) )
        {
            //FIXME code duplication, refactor!
            QStringList trackArtists = artist.split( "feat." );
            //always use the first artist
            QString tmp = trackArtists[0].simplified();
            if( tmp.isEmpty() )
            {
                //TODO error handling
            }
            else
            {
                if( artistCount.contains( tmp ) )
                {
                    artistCount.insert( tmp, artistCount.value( tmp ) + 1 );
                }
                else
                {
                    artistCount.insert( tmp, 1 );
                }
            }
        }
        else
        {
            if( artistCount.contains( artist ) )
            {
                artistCount.insert( artist, artistCount.value( artist ) + 1 );
            }
            else
            {
                artistCount.insert( artist, 1 );
            }
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
    return albumArtist;
}

void
ScanResultProcessor::addTrack( const QVariantMap &trackData, int albumArtistId )
{
    //amarok 1 stored all tracks of a compilation in different directories.
    //when using its "Organize Collection" feature
    //try to detect these cases
    QString albumName = trackData.value( Field::ALBUM ).toString();
    int album = 0;
    QFileInfo file( trackData.value( Field::URL ).toString() );
    QDir dir = file.dir();
    dir.setFilter( QDir::Files );
    //name filtering should be case-insensitive because we do not use QDir::CaseSensitive
    QStringList filters;
    filters << "*.mp3" << "*.ogg" << "*.oga" << "*.flac" << "*.wma";
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

    //urlId will take care of the urls table part of AFT
    int url = urlId( trackData.value( Field::URL ).toString(), uid );

    QString sql,sql2;
    sql = "REPLACE INTO tracks_temp(url,artist,album,genre,composer,year,title,comment,"
                    "tracknumber,discnumber,bitrate,length,samplerate,filesize,filetype,bpm,"
                    "createdate,modifydate) VALUES ( %1,%2,%3,%4,%5,%6,'%7','%8',%9"; //goes up to tracknumber
    sql = sql.arg( QString::number( url )
                , QString::number( artist )
                , QString::number( compilationId ? compilationId : album )
                , QString::number( genre )
                , QString::number( composer )
                , QString::number( year )
                , m_collection->escape( trackData[ Field::TITLE ].toString() )
                , m_collection->escape( trackData[ Field::COMMENT ].toString() )
                , QString::number( trackData[Field::TRACKNUMBER].toInt() ) );

    sql2 = ",%1,%2,%3,%4,%5,%6,%7,%8,%9);";
    sql2 = sql2.arg( QString::number( trackData[Field::DISCNUMBER].toInt() )
                , QString::number( trackData[Field::BITRATE].toInt() )
                , QString::number( trackData[Field::LENGTH].toInt() )
                , QString::number( trackData[Field::SAMPLERATE].toInt() )
                , QString::number( trackData[Field::FILESIZE].toInt() ), "0", "0", "0", "0" ); //filetype,bpm, createdate, modifydate not implemented yet
    sql += sql2;

    m_collection->query( sql );

    if( m_changedUrls )
        m_changedUrls->insert( uid, trackData.value( Field::URL ).toString() );

    //NEXT: this takes care of Meta, now update other tables with deviceid,rpath,uniqueid
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
        return m_albums.value( key );

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
    QString query = QString( "SELECT id FROM urls_temp WHERE deviceid = %1 AND rpath = '%2';" )
                        .arg( QString::number( deviceId ), m_collection->escape( rpath ) );
    QStringList pathres = m_collection->query( query ); //tells us if the path existed
    query = QString( "SELECT id FROM urls_temp WHERE uniqueid='%1';" )
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
        updateAftPermanentTablesUrl( uidres[0].toInt(), uid );
        return uidres[0].toInt();
    }
    else if( !pathres.isEmpty() )
    {
        //We found an existing path; give it the most recent UID value
        QString query = QString( "UPDATE urls_temp SET uniqueid='%1' WHERE deviceid=%2 AND rpath='%3';" )
            .arg( uid, QString::number( deviceId ), m_collection->escape( rpath ) );
        m_collection->query( query );
        updateAftPermanentTablesUid( pathres[0].toInt(), uid );
        return pathres[0].toInt();
    }
    else
        debug() << "AFT algorithm died...you shouldn't be here!  Returning something negative and bad.";
    return -666;
}

void
ScanResultProcessor::updateAftPermanentTablesUrl( int urlId, QString uid )
{
    foreach( QString table, m_aftPermanentTables )
    {
        QString query = QString( "UPDATE %1 SET url=%2 WHERE uniqueid='%3';" )
            .arg( table, QString::number( urlId ), m_collection->escape( uid ) );
        m_collection->query( query );
    }
}

void
ScanResultProcessor::updateAftPermanentTablesUid( int urlId, QString uid )
{
    foreach( QString table, m_aftPermanentTables )
    {
        QString query = QString( "UPDATE %1 SET uniqueid='%2' WHERE url=%3;" )
            .arg( table, m_collection->escape( uid ), QString::number( urlId ) );
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
    AMAROK_NOTIMPLEMENTED
    //check if this album already exists, ignoring the albumartist
    //if it does, and if each file of the album is alone in its directory
    //it's probably a compilation.
    QString query = "SELECT urls.deviceid,urls.rpath,albums.id,albums.artist FROM urls_temp AS urls "
                    "LEFT JOIN tracks_temp on urls.id = tracks_temp.url LEFT JOIN albums_temp AS albums ON "
                    "tracks_temp.album = albums.id WHERE albums.name = '%1';";
    query = query.arg( m_collection->escape( album ) );
    QStringList result = m_collection->query( query );
    for( QListIterator<QString> iter( result ); iter.hasNext(); )
    {
        int deviceid = iter.next().toInt();
        QString rpath = iter.next();
        iter.next(); //albumId
        QString albumArtist = iter.next();
        QString currentPath = MountPointManager::instance()->getAbsolutePath( deviceid, rpath );
        QFileInfo info( currentPath );
        //uint dirCount = m_filesInDirs.value( info.dir().absolutePath() ); //FIXME what's the point?
    }
    return 0;
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
