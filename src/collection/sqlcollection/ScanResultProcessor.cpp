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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
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
    foreach( QStringList *list, m_urlsHashByUid )
        delete list;
    foreach( QLinkedList<QStringList*> *list, m_albumsHashByName )
        delete list;
    foreach( QLinkedList<QStringList*> *list, m_tracksHashByAlbum )
        delete list;
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
    m_imageMap[path] = covers;
}

void
ScanResultProcessor::doneWithImages()
{
    if( m_imageMap.isEmpty() )
        return;

    //now -- find the best candidate with heuristics, then throw the rest away
    const QString path = findBestImagePath( m_imageMap.keys() );
    if( path.isEmpty() )
        return;

    QList< QPair<QString,QString> > covers = m_imageMap[path];
    QList< QPair<QString,QString> >::ConstIterator it = covers.begin();
    for( ; it != covers.end(); ++it )
    {
        QPair<QString,QString> key = (*it);
        if( key.first.isEmpty() || key.second.isEmpty() )
            continue;

        int artist = genericId( "artists", key.first );
        int album  = albumId( key.second, artist );

        // Will automatically add the image path to the database if needed
        imageId( path, album );
    }

    m_imageMap.clear();
}

QString
ScanResultProcessor::findBestImagePath( const QList<QString> &paths )
{
    //DEBUG_BLOCK
    QStringList files;

    //prioritize "front"
    QString front;
    foreach( QString path, paths )
    {
        QString file = QFileInfo( path ).fileName();
        if( file.contains( "front", Qt::CaseInsensitive ) ||
                file.contains( i18nc( "front", "Front cover of an album" ), Qt::CaseInsensitive ) )
            front = path;
    }
    if( !front.isEmpty() )
        return front;

    //then: try "cover"
    QString cover;
    foreach( QString path, paths )
    {
        QString file = QFileInfo( path ).fileName();
        if( file.contains( "cover", Qt::CaseInsensitive ) ||
                file.contains( i18nc( "cover", "(Front) Cover of an album" ), Qt::CaseInsensitive ) )
            cover = path;
    }
    if( !cover.isEmpty() )
        return cover;

    //last: try "large"
    QString large;
    foreach( const QString path, paths )
    {
        QString file = QFileInfo( path ).fileName();
        if( file.contains( "large", Qt::CaseInsensitive ) ||
                file.contains( i18nc( "large", "(Large front) Cover of an album" ), Qt::CaseInsensitive ) )
            large = path;
    }
    if( !large.isEmpty() )
        return large;

    //finally: pick largest image -- often a high-quality blowup of the front
    //so that people can print it out
    qint64 size = 0;
    QString current;
    foreach( QString path, paths )
    {
        QFileInfo info( path );
        if( info.size() > size )
        {
            size = info.size();
            current = path;
        }
    }
    return current;

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

    m_collection->dbUpdater()->deleteAllRedundant( "album" );
    m_collection->dbUpdater()->deleteAllRedundant( "artist" );
    m_collection->dbUpdater()->deleteAllRedundant( "genre" );
    m_collection->dbUpdater()->deleteAllRedundant( "composer" );
    m_collection->dbUpdater()->deleteAllRedundant( "year" );

    debug() << "Sending changed signal";
    m_collection->sendChangedSignal();

    updateAftPermanentTablesUrlString();
    updateAftPermanentTablesUidString();

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
            int artist = genericId( "artists", row.value( Field::ARTIST ).toString() );
            addTrack( row, artist );
        }
    }
    else
    {
        QString albumArtist = findAlbumArtist( artists, data.count() );
        //an empty string means that no albumartist was found
        int artist = albumArtist.isEmpty() ? 0 : genericId( "artists", albumArtist );

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
    //DEBUG_BLOCK
    //amarok 1 stored all tracks of a compilation in different directories.
    //when using its "Organize Collection" feature
    //try to detect these cases
    QString albumName = trackData.value( Field::ALBUM ).toString();
    int album = 0;

    QString path = trackData.value( Field::URL ).toString();

    QFileInfo file( path );

    QDir dir = file.dir();
    dir.setFilter( QDir::Files | QDir::Readable | QDir::CaseSensitive );

    QStringList filters;
    filters << "*.[mM][pP]3" << "*.[oO][gG][gG]" << "*.[oO][gG][aA]" << "*.[fF][lL][aA][cC]" << "*.[wW][mM][aA]" << "*.[mM]4[aAbB]";
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
        compilationId = checkExistingAlbums( albumName );

    QString uid = trackData.value( Field::UNIQUEID ).toString();

    //run a single query to fetch these at once, to save time
    //then values will be cached in local maps, so can use the same calls below
    databaseIdFetch( trackData.value( Field::ARTIST ).toString(),
                     trackData.value( Field::GENRE ).toString(),
                     trackData.value( Field::COMPOSER ).toString(),
                     trackData.value( Field::YEAR ).toString(),
                     albumName, albumArtistId, compilationId, path, uid );

    int artist = genericId( "artists", trackData.value( Field::ARTIST ).toString() );
    int genre = genericId( "genres", trackData.value( Field::GENRE ).toString() );
    int composer = genericId( "composers", trackData.value( Field::COMPOSER ).toString() );
    int year = genericId( "years", trackData.value( Field::YEAR ).toString() );

    if( !compilationId )
        album = albumId( albumName, albumArtistId );


    const int created  = file.created().toTime_t();
    const int modified = file.lastModified().toTime_t();

    //urlId will take care of the urls table part of AFT
    int url = urlId( path, uid );
    m_currUrlIdValues.clear();

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
                , QString::number( trackData[Field::LENGTH].toLongLong() )
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

    int compOrAlbum = compilationId ? compilationId : album;
    QStringList *trackList = new QStringList();
    trackList->append( QString::number( m_nextTrackNum ) );
    trackList->append( QString::number( url ) );
    trackList->append( QString::number( artist ) );
    trackList->append( QString::number( compOrAlbum ) );
    trackList->append( QString::number( genre ) );
    trackList->append( QString::number( composer ) );
    trackList->append( QString::number( year ) );
    trackList->append( trackData[ Field::TITLE ].toString() );
    trackList->append( trackData[ Field::COMMENT ].toString() );
    trackList->append( trackData[ Field::TRACKNUMBER ].toString() );
    trackList->append( trackData[ Field::DISCNUMBER ].toString() );
    trackList->append( trackData[ Field::BITRATE ].toString() );
    trackList->append( trackData[ Field::LENGTH ].toString() );
    trackList->append( trackData[ Field::SAMPLERATE ].toString() );
    trackList->append( trackData[ Field::FILESIZE ].toString() );
    trackList->append( 0 );
    trackList->append( 0 );
    trackList->append( QString::number( created ) );
    trackList->append( QString::number( modified ) );
    if( trackData.contains( Field::ALBUMGAIN ) && trackData.contains( Field::ALBUMPEAKGAIN ) )
    {
        trackList->append( QString::number( trackData[ Field::ALBUMGAIN ].toDouble() ) );
        trackList->append( QString::number( trackData[ Field::ALBUMPEAKGAIN ].toDouble() ) );
    }
    else
    {
        trackList->append( "NULL" );
        trackList->append( "NULL" );
    }
    if( trackData.contains( Field::TRACKGAIN ) && trackData.contains( Field::TRACKPEAKGAIN ) )
    {
        trackList->append( QString::number( trackData[ Field::TRACKGAIN ].toDouble() ) );
        trackList->append( QString::number( trackData[ Field::TRACKPEAKGAIN ].toDouble() ) );
    }

    //insert into hashes
    m_tracksHashById.insert( m_nextTrackNum, trackList );
    m_tracksHashByUrl.insert( url, trackList );
    
    if( m_tracksHashByAlbum.contains( compOrAlbum ) && m_tracksHashByAlbum[compOrAlbum] != 0 )
        m_tracksHashByAlbum[compOrAlbum]->append( trackList );
    else
    {
        QLinkedList<QStringList*> *list = new QLinkedList<QStringList*>();
        list->append( trackList );
        m_tracksHashByAlbum[compOrAlbum] = list;
    }

    //Don't forget to escape the fields that need it when inserting!
    
    m_nextTrackNum++;
}

int
ScanResultProcessor::genericId( const QString &key, const QString &value )
{
    QMap<QString, int> *currMap;
    if( key == "artists" )
        currMap = &m_artists;
    else if( key == "genres" )
        currMap = &m_genres;
    else if( key == "years" )
        currMap = &m_years;
    else if( key == "composers" )
        currMap = &m_composers;
    else
    {
        debug() << "Holy hell Batman, what just happened in genericId?";
        return -9999;
    }

    if( currMap->contains( value ) )
        return currMap->value( value );

    QString query = QString( "SELECT id FROM %1_temp WHERE name = '%2';" ).arg( key, m_collection->escape( value ) );
    QStringList res = m_collection->query( query );
    int id = 0;
    if( res.isEmpty() )
        id = genericInsert( key, value );
    else
        id = res[0].toInt();
    currMap->insert( value, id );
    return id;
}

int
ScanResultProcessor::genericInsert( const QString &key, const QString &value )
{
    QString insert = QString( "INSERT INTO %1_temp( name ) VALUES ('%2');" ).arg( key, m_collection->escape( value ) );
    int id = m_collection->insert( insert, QString( "%1_temp" ).arg( key ) );
    return id;
}

void
ScanResultProcessor::databaseIdFetch( const QString &artist, const QString &genre, const QString &composer, const QString &year, const QString &album, int albumArtistId, int compilationId, const QString &url, const QString &uid )
{
    //DEBUG_BLOCK
    QPair<QString, int> albumKey( album, albumArtistId );
    bool albumFound = compilationId || m_albums.contains( albumKey );
    bool artistFound = m_artists.contains( artist );
    bool genreFound = m_genres.contains( genre );
    bool composerFound = m_composers.contains( composer );
    bool yearFound = m_years.contains( year );

    int l = 0; //album
    int a = 0; //artist
    int g = 0; //genre
    int c = 0; //composer
    int y = 0; //year

    QString query;
    if( !albumFound )
    {
        if( m_albumsHashByName.contains( album ) && m_albumsHashByName[album] != 0 )
        {
            QLinkedList<QStringList*> *list = m_albumsHashByName[album];
            foreach( QStringList *slist, *list )
            {
                if( slist->at( 2 ).isEmpty() && albumArtistId == 0 )
                {
                    debug() << "id found by hashes is " << slist->at( 0 );
                    break;
                }
                else if( slist->at( 2 ).toInt() == albumArtistId )
                {
                    debug() << "id found by hashes is " << slist->at( 0 );
                    break;
                }
            }
        }
        if( albumArtistId == 0 )
            query += QString( "SELECT id, CONCAT('ALBUMNAME_', name), 'dummy1', 'dummy2', 'dummy3', 'dummy4' AS name FROM albums_temp WHERE artist IS NULL AND name = '%1' " )
                        .arg( m_collection->escape( album ) );
        else
            query += QString( "SELECT id, CONCAT('ALBUMNAME_', name), 'dummy1', 'dummy2', 'dummy3', 'dummy4' AS name FROM albums_temp WHERE artist = %1 AND name = '%2' " )
                            .arg( QString::number( albumArtistId ), m_collection->escape( album ) );
    }
    if( !artistFound )
        query += QString( "UNION ALL SELECT id, CONCAT('ARTISTNAME_', name), 'dummy1', 'dummy2', 'dummy3', 'dummy4' AS name FROM artists_temp WHERE name = '%1' " ).arg( m_collection->escape( artist ) );
    if( !genreFound )
        query += QString( "UNION ALL SELECT id, CONCAT('GENRENAME_', name), 'dummy1', 'dummy2', 'dummy3', 'dummy4' AS name FROM genres_temp WHERE name = '%1' " ).arg( m_collection->escape( genre ) );
    if( !composerFound )
        query += QString( "UNION ALL SELECT id, CONCAT('COMPOSERNAME_', name), 'dummy1', 'dummy2', 'dummy3', 'dummy4' AS name FROM composers_temp WHERE name = '%1' " ).arg( m_collection->escape( composer ) );
    if( !yearFound )
        query += QString( "UNION ALL SELECT id, CONCAT('YEARSNAME_', name), 'dummy1', 'dummy2', 'dummy3', 'dummy4' AS name FROM years_temp WHERE name = '%1' " ).arg( m_collection->escape( year ) );

    QFileInfo fileInfo( url );
    const QString dir = fileInfo.absoluteDir().absolutePath();
    int deviceId = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceId, url );
    QString deviceidString = QString::number( deviceId );
    QString escapedRpath = m_collection->escape( rpath );
    QString escapedUid = m_collection->escape( uid );
    //don't bother caching the data, we only call this method for each url once
    PERFORMTHISLOOKUPVIAHASHES
    query += QString( "UNION ALL SELECT 'DUMMYVALUE', id, deviceid, rpath, directory, uniqueid FROM urls_temp WHERE (deviceid = %1 AND rpath = '%2') OR uniqueid='%3' " )
                        .arg( deviceidString, escapedRpath, escapedUid );

    if( query.startsWith( "UNION ALL " ) )
        query.remove( 0, 10 );

    //debug() << "Running this query: " << query << endl;
    QStringList res = m_collection->query( query );
    //debug() << "res = " << res << endl;
    int index = 0;
    QString first;
    QString second;
    bool dummySeen = false;
    while( index < res.size() )
    {
        first = res.at( index++ );
        second = res.at( index++ );
             a = first.toInt();
        //debug() << "first = " << first;
        //debug() << "second = " << second;
        if( first == "DUMMYVALUE" )
        {
            dummySeen = true;
            break;
        }
        if( !albumFound && second == QString( "ALBUMNAME_" + album ) )
        {
            l = first.toInt();
            debug() << "album found by sql is: " << l;
            albumFound = true;
        }
        else if( !artistFound && second == QString( "ARTISTNAME_" + artist ) )
        {
            a = first.toInt();
            artistFound = true;
        }
        else if( !genreFound && second == QString( "GENRENAME_" + genre ) )
        {
            g = first.toInt();
            genreFound = true;
        }
        else if( !composerFound && second == QString( "COMPOSERNAME_" + composer ) )
        {
            c = first.toInt();
            composerFound = true;
        }
        else if( !yearFound && second == QString( "YEARSNAME_" + year ) )
        {
            y = first.toInt();
            yearFound = true;
        }
        index++;
        index++;
        index++;
        index++;
    }
    
    if( dummySeen )
    {
        m_currUrlIdValues = res.mid( res.size() - 5, 5 );
        //debug() << "m_currUrlIdValues = " << m_currUrlIdValues;
    }

    if( !albumFound )
    {
        m_albums.insert( albumKey, albumInsert( album, albumArtistId ) );
        albumFound = true;
    }
    if( !artistFound )
    {
        m_artists.insert( artist, genericInsert( "artists", artist ) );
        artistFound = true;
    }
    if( !genreFound )
    {
        m_genres.insert( genre, genericInsert( "genres", genre ) );
        genreFound = true;
    }
    if( !composerFound )
    {
        m_composers.insert( composer, genericInsert( "composers", composer ) );
        composerFound = true;
    }
    if( !yearFound )
    {
        m_years.insert( year, genericInsert( "years", year ) );
        yearFound = true;
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
        if( m_albumsHashById.contains( albumId ) && m_albumsHashById[albumId] != 0 )
        {
            QStringList *list = m_albumsHashById[albumId];
            list->replace( 3, QString::number( imageId ) );
        }
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
            if( m_albumsHashByName.contains( album ) && m_albumsHashByName[album] != 0 )
            {
                QStringList *slist;
                int maxImage = 0;
                QLinkedList<QStringList*> *llist = m_albumsHashByName[album];
                foreach( QStringList* list, *llist )
                {
                    if( !(list->at( 3 ).isEmpty()) && list->at( 3 ).toInt() > maxImage )
                    {
                        slist = list;
                        maxImage = list->at( 3 ).toInt();
                    }
                }
                if( maxImage > 0 )
                {
                    if( m_albumsHashById.contains( id ) && m_albumsHashById[id] != 0 )
                    {
                        QStringList *list = m_albumsHashById[id];
                        list->replace( 3, QString::number( maxImage ) );
                        debug() << "Hash: setting image to " << maxImage << "where id is " << id;
                    }
                }
            }
            QString select = QString( "SELECT MAX(image) FROM albums_temp WHERE name = '%1';" )
                .arg( m_collection->escape( album ) );
            QStringList res = m_collection->query( select );
            if( !res.isEmpty() && !res[0].isEmpty() )
            {
                QString update = QString( "UPDATE albums_temp SET image = %1 WHERE id = %2" )
                    .arg( res[0] , QString::number( id ) );
                m_collection->query( update );
                debug() << "SQL: setting image to " << res[0] << "where id is " << id;
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
    int id = 0;
    if( res.isEmpty() )
        id = albumInsert( album, artistId );
    else
        id = res[0].toInt();
    m_albums.insert( key, id );
    return id;
}

int
ScanResultProcessor::albumInsert( const QString &album, int artistId )
{
    DEBUG_BLOCK
    debug() << "id returned from hash: " << m_nextAlbumNum;
    QStringList* albumList = new QStringList();
    albumList->append( QString::number( m_nextAlbumNum ) );
    albumList->append( album );
    albumList->append( QString::number( artistId ) );
    albumList->append( QString() );
    m_albumsHashById[m_nextAlbumNum] = albumList;
    if( m_albumsHashByName.contains( album ) && m_albumsHashByName[album] != 0 )
        m_albumsHashByName[album]->append( albumList );
    else
    {
        QLinkedList<QStringList*> *list = new QLinkedList<QStringList*>();
        list->append( albumList );
        m_albumsHashByName[album] = list;
    }
    m_nextAlbumNum++;
    QString insert = QString( "INSERT INTO albums_temp( artist, name ) VALUES ( %1, '%2');" )
        .arg( artistId ? QString::number( artistId ) : "NULL", m_collection->escape( album ) );
    int id = m_collection->insert( insert, "albums_temp" );
    debug() << "id returned from SQL: " << id;
    return id;
}

int
ScanResultProcessor::urlId( const QString &url, const QString &uid )
{
    DEBUG_BLOCK
    QFileInfo fileInfo( url );
    const QString dir = fileInfo.absoluteDir().absolutePath();
    int dirId = directoryId( dir );
    int deviceId = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceId, url );

    if( m_currUrlIdValues.isEmpty() )  //fresh -- insert
    {
        QStringList *list = new QStringList();
        list->append( QString::number( m_nextUrlNum ) );
        list->append( QString::number( deviceId ) );
        list->append( rpath );
        list->append( QString::number( dirId ) );
        list->append( uid );
        m_urlsHashByUid[uid] = list;
        m_urlsHashById[m_nextUrlNum] = list;
        m_nextUrlNum++;
        QString insert = QString( "INSERT INTO urls_temp (deviceid,rpath,directory,uniqueid) VALUES ( %1, %2, '%3', '%4' );" )
                    .arg( QString::number( deviceId ), m_collection->escape( rpath ), QString::number( dirId ),
                              m_collection->escape( uid ) );
        return m_collection->insert( insert, "urls_temp" );
    }

    if( m_currUrlIdValues[1] == QString::number( deviceId ) &&
        m_currUrlIdValues[2] == rpath &&
        m_currUrlIdValues[3] == QString::number( dirId ) &&
        m_currUrlIdValues[4] == uid
      )
    {
        //everything matches, don't need to do anything, just return the ID
        return m_currUrlIdValues[0].toInt();
    }

    if( m_currUrlIdValues[4] == uid )
    {
        //we found an existing entry with this uniqueid, update the deviceid and path
        //Note that we ignore the situation where both a UID and path was found; UID takes precedence
        
        if( m_urlsHashByUid.contains( uid ) && m_urlsHashByUid[uid] != 0 )
        {
            QStringList *list = m_urlsHashByUid[uid];
            list->replace( 1, QString::number( deviceId ) );
            list->replace( 2, rpath );
            list->replace( 3, QString::number( dirId ) );
            debug() << "Hash updated UID-based values for uid " << uid;
        }
        QString query = QString( "UPDATE urls_temp SET deviceid=%1,rpath='%2',directory=%3 WHERE uniqueid='%4';" )
            .arg( QString::number( deviceId ), m_collection->escape( rpath ), QString::number( dirId ),
                            m_collection->escape( uid ) );
        m_collection->query( query );
        debug() << "SQL updated UID-based values for uid " << uid;
        m_permanentTablesUrlUpdates.insert( uid, url );
        m_changedUrls.insert( uid, QPair<QString, QString>( MountPointManager::instance()->getAbsolutePath( m_currUrlIdValues[1].toInt(), m_currUrlIdValues[2] ), url ) );
        return m_currUrlIdValues[0].toInt();
    }

    if( m_currUrlIdValues[1] == QString::number( deviceId ) && m_currUrlIdValues[2] == rpath )
    {
        //We found an existing path; give it the most recent UID value
        int urlId = m_currUrlIdValues[0].toInt();
        if( m_urlsHashById.contains( urlId ) && m_urlsHashById[urlId] != 0 )
        {
            QStringList *list = m_urlsHashById[urlId];
            list->replace( 4, uid );
            debug() << "Hash updated path-based values for uid " << uid;
        }
 
        QString query = QString( "UPDATE urls_temp SET uniqueid='%1' WHERE deviceid=%2 AND rpath='%3';" )
            .arg( uid, QString::number( deviceId ), m_collection->escape( rpath ) );
        m_collection->query( query );
        debug() << "SQL updated path-based values for uid " << uid;
        m_permanentTablesUidUpdates.insert( url, uid );
        m_changedUids.insert( m_currUrlIdValues[4], uid );
        return m_currUrlIdValues[0].toInt();
    }

    debug() << "AFT algorithm died...you should not be here!  Returning something negative and bad.";
    return -666;
}

void
ScanResultProcessor::updateAftPermanentTablesUrlString()
{
    //DEBUG_BLOCK
    if( m_permanentTablesUrlUpdates.isEmpty() )
        return;
    foreach( const QString &table, m_aftPermanentTablesUrlString )
    {
        QString query = QString( "UPDATE %1 SET url = CASE uniqueid" ).arg( table );
        QString query2;
        bool first = true;
        foreach( const QString key, m_permanentTablesUrlUpdates.keys() )
        {
            query += QString( " WHEN '%1' THEN '%2'" ).arg( m_collection->escape( key ),
                                                       m_collection->escape( m_permanentTablesUrlUpdates[key] ) );
            if( first )
                query2 += QString( "'%1'" ).arg( m_collection->escape( key ) );
            else
                query2 += QString( ", '%1'" ).arg( m_collection->escape( key ) );
            first = false;
        }
        query += QString( " END WHERE uniqueid IN(%1);" ).arg( query2 );

        m_collection->query( query );
    }
}

void
ScanResultProcessor::updateAftPermanentTablesUidString()
{
    //DEBUG_BLOCK
    if( m_permanentTablesUidUpdates.isEmpty() )
        return;
    foreach( const QString &table, m_aftPermanentTablesUrlString )
    {
        QString query = QString( "UPDATE %1 SET uniqueid = CASE url" ).arg( table );
        QString query2;
        bool first = true;
        foreach( const QString key, m_permanentTablesUidUpdates.keys() )
        {
            query += QString( " WHEN '%1' THEN '%2'" ).arg( m_collection->escape( key ),
                                                       m_collection->escape( m_permanentTablesUidUpdates[key] ) );
            if( first )
                query2 += QString( "'%1'" ).arg( m_collection->escape( key ) );
            else
                query2 += QString( ", '%1'" ).arg( m_collection->escape( key ) );
            first = false;
        }
        query += QString( " END WHERE url IN(%1);" ).arg( query2 );

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
    DEBUG_BLOCK
    // "Unknown" albums shouldn't be handled as compilations
    if( album.isEmpty() )
        return 0;

    //check if this album already exists, ignoring the albumartist
    //if it does, and if each file of the album is alone in its directory
    //it's probably a compilation.
    //this handles A1 compilations that were automatically organized by Amarok
    QString query = "SELECT urls_temp.deviceid,urls_temp.rpath,tracks_temp.id,albums_temp.id,albums_temp.artist FROM urls_temp "
                    "LEFT JOIN tracks_temp on urls_temp.id = tracks_temp.url LEFT JOIN albums_temp ON "
                    "tracks_temp.album = albums_temp.id WHERE albums_temp.name = '%1';";
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

    debug() << "trackIDs found via SQL: " << trackIds.size();
    qSort( trackIds );
    debug() << "Via SQL, trackIds is " << trackIds;

    QList<QString> hashTrackIds;
    if( !m_albumsHashByName.contains( album ) || m_albumsHashByName[album] == 0 )
    {
        debug() << "trackIDs found via hashes: 0 " << ( trackIds.size() == 0 ? "FUCKYEAH!" : "FUCKNAW" );
        return 0;
    }

    debug() << "Tracks size = " << m_tracksHashById.size();
    QLinkedList<QStringList*> *llist = m_albumsHashByName[album];
    QLinkedList<int> albumIntList;
    foreach( QStringList* albumList, *llist )
        albumIntList.append( (*albumList)[0].toInt() ); //list of album IDs, now find tracks

    QLinkedList<int> trackIntList;
    foreach( int albumInt, albumIntList )
    {
        if( !m_tracksHashByAlbum.contains( albumInt ) || m_tracksHashByAlbum[albumInt] == 0 )
            continue;
        foreach( QStringList* slist, *m_tracksHashByAlbum[albumInt] )
            trackIntList.append( (*slist)[0].toInt() ); //list of tracks matching those album IDs
    }
    
    foreach( int track, trackIntList )
        debug() << "trackIntList contains: " << track;
    //note that there will be a 1:1 mapping between tracks and urls, although the id is not necessarily the same
    //and there may be more urls than tracks -- this means that this track list is all we need
    //the big mama
    int l_deviceid;
    QString l_rpath, l_trackId, l_albumId, l_albumArtistId, l_currentPath;
    foreach( int track, trackIntList )
    {
        if( !m_tracksHashById.contains( track ) || m_tracksHashById[track] == 0 )
            continue;
        QStringList trackList = *m_tracksHashById[track];

        if( !m_urlsHashById.contains( trackList[1].toInt() ) || m_urlsHashById[trackList[1].toInt()] == 0 )
            continue;
        QStringList urlList = *m_urlsHashById[trackList[1].toInt()];

        if( !m_albumsHashById.contains( trackList[3].toInt() ) || m_albumsHashById[trackList[3].toInt()] == 0 )
            continue;
        QStringList albumList = *m_albumsHashById[trackList[3].toInt()];

        l_deviceid = urlList[1].toInt();
        l_rpath = urlList[2];
        l_trackId = QString::number( track );
        l_albumId = trackList[3];
        l_albumArtistId = albumList[2];
        l_currentPath = MountPointManager::instance()->getAbsolutePath( l_deviceid, l_rpath );
        QFileInfo info( l_currentPath );
        uint dirCount = m_filesInDirs.value( info.dir().absolutePath() );
        if( dirCount == 1 )
        {
            hashTrackIds << l_trackId;
        }
    }
    debug() << "trackIDs found via hash: " << hashTrackIds.size();
    qSort( hashTrackIds );
    debug() << "Via hash, trackIDs is " << hashTrackIds;

    debug() << "Are lists equal? " << ( ( trackIds == hashTrackIds ) ? "FUCKYEAH" : "FUCKNAW" );

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
    DEBUG_BLOCK
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
        populateCacheHashes();
        // /*
        debug() << "Last URL num: " << m_lastUrlNum << ", next URL num: " << m_nextUrlNum;
        //foreach( QString key, m_urlsHashByUid.keys() )
        //    debug() << "Key: " << key << ", list: " << *m_urlsHashByUid[key];
        //foreach( int key, m_urlsHashById.keys() )
        //    debug() << "Key: " << key << ", list: " << *m_urlsHashById[key];
        debug() << "Last album num: " << m_lastAlbumNum << ", next album num: " << m_nextAlbumNum;
        //foreach( int key, m_albumsHashById.keys() )
        //    debug() << "Key: " << key << ", list: " << *m_albumsHashById[key];
        //foreach( QString key, m_albumsHashByName.keys() )
        //{
        //    foreach( QStringList* list, *m_albumsHashByName[key] )
        //       debug() << "Key: " << key << ", list ptrs: " << *list;
        //}
        debug() << "Last track num: " << m_lastTrackNum << ", next album num: " << m_nextTrackNum;
        //foreach( int key, m_tracksHashById.keys() )
        //    debug() << "Key: " << key << ", list: " << *m_tracksHashById[key];
        //foreach( int key, m_tracksHashByUrl.keys() )
        //    debug() << "Key: " << key << ", list: " << *m_tracksHashByUrl[key];
        //foreach( int key, m_tracksHashByAlbum.keys() )
        //{
        //    foreach( QStringList* list, *m_tracksHashByAlbum[key] )
        //        debug() << "Key: " << key << ", list: " << *list;
        //}
        // */
    }

}

void
ScanResultProcessor::populateCacheHashes()
{
    DEBUG_BLOCK

    //urls
    QStringList res = m_collection->query( "SELECT * FROM urls_temp ORDER BY id ASC;" );
    int reserveSize = ( res.size() / 5 ) * 2; //Reserve plenty of space to bring insertion and lookup close to O(1)
    m_urlsHashByUid.reserve( reserveSize );
    m_urlsHashById.reserve( reserveSize );
    QStringList *currList;
    QLinkedList<QStringList*> *llist;
    int index = 0;
    while( index < res.size() )
    {
        currList = new QStringList();
        m_lastUrlNum = res.at( index ).toInt();
        for( int i = 0; i < 5; i++ )
            currList->append( res.at(index++) );
        m_urlsHashByUid.insert( currList->last(), currList );
        m_urlsHashById.insert( m_lastUrlNum, currList );
    }
    m_nextUrlNum = m_lastUrlNum + 1;

    //albums
    res = m_collection->query( "SELECT * FROM albums_temp ORDER BY id ASC;" );
    reserveSize = ( res.size() / 4 ) * 2;
    m_albumsHashByName.reserve( reserveSize );
    m_albumsHashById.reserve( reserveSize );
    index = 0;
    while( index < res.size() )
    {
        currList = new QStringList();
        m_lastAlbumNum = res.at( index ).toInt();
        for( int i = 0; i < 4; i++ )
            currList->append( res.at(index++) );
        m_albumsHashById.insert( m_lastAlbumNum, currList );

        if( m_albumsHashByName.contains( currList->at( 1 ) ) )
        {
            llist = m_albumsHashByName[currList->at( 1 )];
            llist->append( currList );
        }
        else
        {
            llist = new QLinkedList<QStringList*>();
            llist->append( currList );
            m_albumsHashByName.insert( currList->at( 1 ), llist );
        }
    }
    m_nextAlbumNum = m_lastAlbumNum + 1;

    //tracks
    res = m_collection->query( "SELECT * FROM tracks_temp ORDER BY id ASC;" );
    reserveSize = ( res.size() / 22 ) * 2;
    m_tracksHashById.reserve( reserveSize );
    index = 0;
    while( index < res.size() )
    {
        currList = new QStringList();
        m_lastTrackNum = res.at( index ).toInt();
        for( int i = 0; i < 23; i++ )
            currList->append( res.at(index++) );
        m_tracksHashById.insert( m_lastTrackNum, currList );
        m_tracksHashByUrl.insert( currList->at( 1 ).toInt(), currList );

        int currAlbum = currList->at( 3 ).toInt();
        if( m_tracksHashByAlbum.contains( currAlbum ) )
        {
            llist = m_tracksHashByAlbum[currAlbum];
            llist->append( currList );
        }
        else
        {
            llist = new QLinkedList<QStringList*>();
            llist->append( currList );
            m_tracksHashByAlbum.insert( currAlbum, llist );
        }
    }
    m_nextTrackNum = m_lastTrackNum + 1;

}

#include "ScanResultProcessor.moc"

