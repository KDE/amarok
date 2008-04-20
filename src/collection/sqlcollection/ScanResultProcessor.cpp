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

#include "ScanResultProcessor.h"

#include "debug.h"
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
    , m_type( FullScan )
{
    DEBUG_BLOCK
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
    }
}

void
ScanResultProcessor::commit()
{
    if( m_type == ScanResultProcessor::FullScan )
    {
        //TODO clean permanent tables
    }
    m_collection->dbUpdater()->copyToPermanentTables();
    m_collection->dbUpdater()->removeTemporaryTables();
    m_collection->sendChangedSignal();
}

void
ScanResultProcessor::rollback()
{
    m_collection->dbUpdater()->removeTemporaryTables();
}

void
ScanResultProcessor::processDirectory( const QList<QHash<QString, QString> > &data )
{
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
        album = data[0].value( Field::ALBUM );
    QHash<QString,QString> row;
    foreach( row, data )
    {
        artists.insert( row.value( Field::ARTIST ) );
        if( row.value( Field::ALBUM ) != album )
            multipleAlbums = true;
    }
    if( multipleAlbums || album.isEmpty() || data.count() > 60 || artists.size() == 1 )
    {
        QHash<QString, QString> row;
        foreach( row, data )
        {
            int artist = artistId( row.value( Field::ARTIST ) );
            addTrack( row, artist );
        }
    }
    else
    {
        QString albumArtist = findAlbumArtist( artists );
        //an empty string means that no albumartist was found
        int artist = albumArtist.isEmpty() ? 0 : artistId( albumArtist );
        QHash<QString, QString> row;
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
ScanResultProcessor::addTrack( const QHash<QString, QString> &trackData, int albumArtistId )
{

    //amarok 1 stored all tracks of a compilation in different directories.
    //when using its "Organize Collection" feature
    //try to detect these cases
    QString albumName = trackData.value( Field::ALBUM );
    int album = 0;
    QFileInfo file( trackData.value( Field::URL ) );
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
    int artist = artistId( trackData.value( Field::ARTIST ) );
    int genre = genreId( trackData.value( Field::GENRE ) );
    int composer = composerId( trackData.value( Field::COMPOSER ) );
    int year = yearId( trackData.value( Field::YEAR ) );
    int url = urlId( trackData.value( Field::URL ) );

    QString insert = "INSERT INTO tracks_temp(url,artist,album,genre,composer,year,title,comment,"
                     "tracknumber,discnumber,bitrate,length,samplerate,filesize,filetype,bpm,"
                     "createdate,modifydate) VALUES ( %1,%2,%3,%4,%5,%6,'%7','%8',%9"; //goes up to tracknumber
    insert = insert.arg( url ).arg( artist ).arg( compilationId ? compilationId : album ).arg( genre ).arg( composer ).arg( year );
    insert = insert.arg( m_collection->escape( trackData[ Field::TITLE ] ), m_collection->escape( trackData[ Field::COMMENT ] ) );
    insert = insert.arg( trackData[Field::TRACKNUMBER].toInt() );

    QString insert2 = ",%1,%2,%3,%4,%5,%6,%7,%8,%9);";
    insert2 = insert2.arg( trackData[Field::DISCNUMBER].toInt() );
    insert2 = insert2.arg( trackData[Field::BITRATE].toInt() ).arg( trackData[Field::LENGTH].toInt() );
    insert2 = insert2.arg( trackData[Field::SAMPLERATE].toInt() ).arg( trackData[Field::FILESIZE].toInt() );
    insert2 = insert2.arg( "0", "0", "0", "0" ); //filetype,bpm, createdate, modifydate not implemented yet
    insert += insert2;

    m_collection->insert( insert, "tracks_temp" );
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
                    .arg( artistId ? QString::number( artistId ) : "NULL" )
                    .arg( m_collection->escape( album ) );
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
ScanResultProcessor::urlId( const QString &url )
{
    int deviceId = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceId, url );
    //don't bother caching the data, we only call this method for each url once
    QString query = QString( "SELECT id FROM urls_temp WHERE deviceid = %1 AND rpath = '%2';" )
                        .arg( QString::number( deviceId ), m_collection->escape( rpath ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QFileInfo fileInfo( url );
        const QString dir = fileInfo.absoluteDir().absolutePath();
        int dirId = directoryId( dir );
        QString insert = QString( "INSERT INTO urls_temp(directory,deviceid, rpath) VALUES ( %1, %2, '%3' );" )
                            .arg( QString::number( dirId ), QString::number( deviceId ), m_collection->escape( rpath ) );
        return m_collection->insert( insert, "urls_temp" );
    }
    else
    {
        return res[0].toInt();
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
                        .arg( deviceId ).arg( m_collection->escape( rpath ) );
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
        int albumId = iter.next().toInt();
        QString albumArtist = iter.next();
        QString currentPath = MountPointManager::instance()->getAbsolutePath( deviceid, rpath );
        QFileInfo info( currentPath );
        uint dirCount = m_filesInDirs.value( info.dir().absolutePath() );
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
