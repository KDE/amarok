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
#include "sqlcollection.h"

#include <KUrl>

using namespace Meta;

ScanResultProcessor::ScanResultProcessor( SqlCollection *collection )
    : m_collection( collection )
{
    //nothing to do
}

ScanResultProcessor::~ScanResultProcessor()
{
    //nothing to do
}

void
ScanResultProcessor::processScanResult( const QMap<QString, QHash<QString, QString> > &scanResult )
{
    DEBUG_BLOCK
    QList<QHash<QString, QString> > dirData;
    bool firstTrack = true;
    QString dir;
    QHash<QString,QString> track;
    foreach( track, scanResult )
    {
        if( firstTrack )
        {
            KUrl url( track.value( Field::URL ) );
            dir = url.directory();
            firstTrack = false;
        }

        KUrl url( track.value( Field::URL ) );
        if( url.directory() == dir )
        {
            dirData.append( track );
        }
        else
        {
            processDirectory( dirData );
            dirData.clear();
            dir = url.directory();
        }
    }
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
    if( multipleAlbums || data.count() > 60 || artists.size() == 1 )
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
ScanResultProcessor::findAlbumArtist( const QSet<QString> &artist ) const
{
    return QString();
}

void
ScanResultProcessor::addTrack( const QHash<QString, QString> &trackData, int albumArtistId )
{
    DEBUG_BLOCK
    int album = albumId( trackData.value( Field::ALBUM ), albumArtistId );
    int artist = artistId( trackData.value( Field::ARTIST ) );
    int genre = genreId( trackData.value( Field::GENRE ) );
    int composer = composerId( trackData.value( Field::COMPOSER ) );
    int year = yearId( trackData.value( Field::YEAR ) );
    int url = urlId( trackData.value( Field::URL ) );

    QString insert = "INSERT INTO tracks_temp(url,artist,album,genre,composer,year,title,comment,"
                     "tracknumber,discnumber,bitrate,length,samplerate,filesize,filetype,bpm"
                     "createdate,modifydate) VALUES ( %1,%2,%3,%4,%5,%6,'%7','%8'"; //goes up to comment
    insert = insert.arg( url ).arg( artist ).arg( album ).arg( genre ).arg( composer ).arg( year );
    insert = insert.arg( m_collection->escape( trackData[ Field::TITLE ] ), m_collection->escape( trackData[ Field::COMMENT ] ) );

    QString insert2 = ",%1,%2,%3,%4,%5,%6,%7,%8,%9,%10);";
    insert2 = insert2.arg( trackData[Field::TRACKNUMBER].toInt() ).arg( trackData[Field::DISCNUMBER].toInt() ).arg( trackData[Field::BITRATE].toInt() );
    insert2 = insert2.arg( trackData[Field::LENGTH].toInt() ).arg( trackData[Field::SAMPLERATE].toInt() ).arg( trackData[Field::FILESIZE].toInt() );
    insert2 = insert2.arg( "0", "0", "0", "0" ); //filetype,bpm, createdate, modifydate not implemented yet
    insert += insert2;

    //m_collection->insert( insert, "tracks" );
    debug() << insert;
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
        QString insert = QString( "INSERT INTO albums_temp( name ) VALUES ('%1');" ).arg( m_collection->escape( artist ) );
        int id = m_collection->insert( insert, "albums_temp" );
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
        query = QString( "SELECT if FROM albums_temp WHERE artist IS NULL AND name = '%1';" )
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
        QString insert = QString( "INSERT INTO urls_temp(deviceid, rpath) VALUES ( %1, '%2' );" )
                            .arg( QString::number( deviceId ), m_collection->escape( rpath ) );
        return m_collection->insert( insert, "urls_temp" );
    }
    else
    {
        return res[0].toInt();
    }
}
