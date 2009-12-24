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

#include <QDir>
#include <QFileInfo>
#include <QListIterator>


using namespace Meta;

ScanResultProcessor::ScanResultProcessor( SqlCollection *collection )
    : m_collection( collection )
    , m_storage( 0 )
    , m_setupComplete( false )
    , m_type( FullScan )
    , m_aftPermanentTablesUrlString()
{
    m_aftPermanentTablesUrlString << "playlist_tracks";
}

ScanResultProcessor::~ScanResultProcessor()
{
    //everything has a URL, so enough to just delete from here
    foreach( QStringList *list, m_urlsHashByUid )
        delete list;
    foreach( QLinkedList<QStringList*> *list, m_albumsHashByName )
    {
        foreach( QStringList *slist, *list )
            delete slist;
        delete list;
    }
    foreach( QLinkedList<QStringList*> *list, m_tracksHashByAlbum )
    {
        foreach( QStringList *slist, *list )
            delete slist;
        delete list;
    }
}

void
ScanResultProcessor::setScanType( ScanType type )
{
    m_type = type;
}

void
ScanResultProcessor::addDirectory( const QString &dir, uint mtime )
{
    DEBUG_BLOCK
    debug() << "SRP::addDirectory on " << dir << " with mtime " << mtime;
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
                        .arg( QString::number( deviceId ), m_storage->escape( rdir ) );
    QStringList res = m_storage->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO directories_temp(deviceid,changedate,dir) VALUES (%1,%2,'%3');" )
                        .arg( QString::number( deviceId ), QString::number( mtime ),
                                m_storage->escape( rdir ) );
        int id = m_storage->insert( insert, "directories_temp" );
        m_directories.insert( dir, id );
    }
    else
    {
        if( res[1].toUInt() != mtime )
        {
            QString update = QString( "UPDATE directories_temp SET changedate = %1 WHERE id = %2;" )
                                .arg( QString::number( mtime ), res[0] );
            m_storage->query( update );
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

        int artist = genericId( &m_artists, key.first, &m_nextArtistNum );
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
        debug() << "Database temporary table setup did not complete due to no directories needing to be processed.";
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

    copyHashesToTempTables();

    debug() << "temp_tracks: " << m_storage->query("select count(*) from tracks_temp");
    debug() << "tracks before commit: " << m_storage->query("select count(*) from tracks");
    m_collection->dbUpdater()->copyToPermanentTables();
    debug() << "tracks after commit: " << m_storage->query("select count(*) from tracks");
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
             m_collection, SLOT( updateTrackUrlsUids( const ChangedTrackUrls &, const TrackUrls & ) ) );

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
//     DEBUG_BLOCK
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
            int artist = genericId( &m_artists, row.value( Field::ARTIST ).toString(), &m_nextArtistNum );
            addTrack( row, artist );
        }
    }
    else
    {
        QString albumArtist = findAlbumArtist( artists, data.count() );
        //an empty string means that no albumartist was found
        int artist = albumArtist.isEmpty() ? 0 : genericId( &m_artists, albumArtist, &m_nextArtistNum );

        //debug() << "albumartist " << albumArtist << "for artists" << artists;
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

    //do not check existing albums if there is more than one file in the directory
    //see comments in checkExistingAlbums

    //TODO: find a better way to ignore non-audio files than the extension matching below
    if( !m_filesInDirs.contains( dir.absolutePath() ) )
    {
        dir.setFilter( QDir::Files | QDir::Readable | QDir::CaseSensitive );
        QStringList filters;
        filters << "*.[mM][pP]3" << "*.[oO][gG][gG]" << "*.[oO][gG][aA]" << "*.[fF][lL][aA][cC]" << "*.[wW][mM][aA]" << "*.[mM]4[aAbB]";
        dir.setNameFilters( filters );
        m_filesInDirs.insert( dir.absolutePath(), dir.count() );
    }

    if( m_filesInDirs.value( dir.absolutePath() ) == 1 )
    {
        album = checkExistingAlbums( albumName );
    }

    QString uid = trackData.value( Field::UNIQUEID ).toString();

    int artist = genericId( &m_artists, trackData.value( Field::ARTIST ).toString(), &m_nextArtistNum );
    int genre = genericId( &m_genres, trackData.value( Field::GENRE ).toString(), &m_nextGenreNum );
    int composer = genericId( &m_composers, trackData.value( Field::COMPOSER ).toString(), &m_nextComposerNum );
    int year = genericId( &m_years, trackData.value( Field::YEAR ).toString(), &m_nextYearNum );

    if( !album ) //no compilation
    {
        album = albumId( albumName, albumArtistId );
        //debug() << "album set to " << album;
    }

    const int created  = file.created().toTime_t();
    const int modified = file.lastModified().toTime_t();

    //urlId will take care of the urls table part of AFT
    int url = urlId( path, uid );

    QStringList *trackList = new QStringList();
    int id = m_nextTrackNum;
    //debug() << "Appending new track number with tracknum: " << id;
    trackList->append( QString::number( m_nextTrackNum++ ) );
    trackList->append( QString::number( url ) );
    trackList->append( QString::number( artist ) );
    trackList->append( QString::number( album ) );
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
    trackList->append( QString() ); //filetype
    if( trackData.contains( Field::BPM ) )
        trackList->append( QString::number( trackData[ Field::BPM ].toDouble() ).replace( ',' , '.' ) );
    else
        trackList->append( QString() );
    trackList->append( QString::number( created ) );
    trackList->append( QString::number( modified ) );
    if( trackData.contains( Field::ALBUMGAIN ) && trackData.contains( Field::ALBUMPEAKGAIN ) )
    {
        //QLocale is set by default from LANG, but this will use , for floats, which screws up the SQL.
        trackList->append( QString::number( trackData[ Field::ALBUMGAIN ].toDouble() ).replace( ',' , '.' ) );
        trackList->append( QString::number( trackData[ Field::ALBUMPEAKGAIN ].toDouble() ).replace( ',' , '.' ) );
    }
    else
    {
        trackList->append( QString() );
        trackList->append( QString() );
    }
    if( trackData.contains( Field::TRACKGAIN ) && trackData.contains( Field::TRACKPEAKGAIN ) )
    {
        trackList->append( QString::number( trackData[ Field::TRACKGAIN ].toDouble() ).replace( ',' , '.' ) );
        trackList->append( QString::number( trackData[ Field::TRACKPEAKGAIN ].toDouble() ).replace( ',' , '.' ) );
    }
    else
    {
        trackList->append( QString() );
        trackList->append( QString() );
    }

    //insert into hashes
    if( m_tracksHashByUrl.contains( url ) && m_tracksHashByUrl[url] != 0 )
    {
        //debug() << "m_tracksHashByUrl contains the url!";
        //need to replace, not overwrite/add a new one
        QStringList *oldValues = m_tracksHashByUrl[url];
        QString oldId = oldValues->at( 0 );
        //debug() << "old id is " << oldId;
        oldValues->clear();
        oldValues->append( oldId );
        for( int i = 1; i < trackList->size(); i++ ) //not 0 because we want to keep old ID
            oldValues->append( trackList->at( i ) );
        delete trackList;
        trackList = oldValues;
        id = oldId.toInt();
        m_nextTrackNum--;
    }
    else
    {
        m_tracksHashByUrl.insert( url, trackList );
        m_tracksHashById.insert( id, trackList );
    }

    if( m_tracksHashByAlbum.contains( album ) && m_tracksHashByAlbum[album] != 0 )
        m_tracksHashByAlbum[album]->append( trackList );
    else
    {
        QLinkedList<QStringList*> *list = new QLinkedList<QStringList*>();
        list->append( trackList );
        m_tracksHashByAlbum[album] = list;
    }
}

int
ScanResultProcessor::genericId( QHash<QString, int> *hash, const QString &value, int *currNum )
{
    //DEBUG_BLOCK
    if( hash->contains( value ) )
        return hash->value( value );
    else
    {
        int id = *currNum;
        hash->insert( value, (*currNum)++ );
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

    int imageId = -1;
    if( m_imagesFlat.contains( image ) )
        imageId = m_imagesFlat[image];
    else
    {
        imageId = m_nextImageNum;
        m_imagesFlat[image] = m_nextImageNum++;
    }

    if( imageId >= 0 )
    {
        if( m_albumsHashById.contains( albumId ) && m_albumsHashById[albumId] != 0 )
        {
            QStringList *list = m_albumsHashById[albumId];
            list->replace( 3, QString::number( imageId ) );
        }
        m_images.insert( key, imageId );
    }

    return imageId;
}

int
ScanResultProcessor::albumId( const QString &album, int albumArtistId )
{
    //DEBUG_BLOCK
    //debug() << "Looking up album " << album;
    //albumArtistId == 0 means no albumartist
    QPair<QString, int> key( album, albumArtistId );
    if( m_albums.contains( key ) )
    {
        //debug() << "m_albums contains album/albumArtistId key";
        // if we already have the key but the artist == 0,
        // UPDATE the image field so that we won't forget the cover for a compilation
        int id = m_albums.value( key );
        if ( albumArtistId == 0 )
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
                    }
                }
            }
        }
        return id;
    }

    int id = 0;
    if( m_albumsHashByName.contains( album ) && m_albumsHashByName[album] != 0 )
    {
        //debug() << "Hashes contain it";
        QLinkedList<QStringList*> *list = m_albumsHashByName[album];
        foreach( QStringList *slist, *list )
        {
            if( slist->at( 2 ).isEmpty() && albumArtistId == 0 )
            {
                //debug() << "artist is empty and albumArtistId = 0, returning " << slist->at( 0 );
                id = slist->at( 0 ).toInt();
                break;
            }
            else if( slist->at( 2 ).toInt() == albumArtistId )
            {
                //debug() << "artist == albumArtistId,  returning " << slist->at( 0 );
                id = slist->at( 0 ).toInt();
                break;
            }
        }
    }
    if( !id )
    {
        //debug() << "Not found! Inserting...";
        id = albumInsert( album, albumArtistId );
    }
    m_albums.insert( key, id );
    //debug() << "returning id = " << id;
    return id;
}

int
ScanResultProcessor::albumInsert( const QString &album, int albumArtistId )
{
    //DEBUG_BLOCK
    int returnedNum = m_nextAlbumNum++;
    QStringList* albumList = new QStringList();
    albumList->append( QString::number( returnedNum ) );
    albumList->append( album );
    albumList->append( albumArtistId ? QString::number( albumArtistId ) : QString() );
    albumList->append( QString() );
    m_albumsHashById[returnedNum] = albumList;
    if( m_albumsHashByName.contains( album ) && m_albumsHashByName[album] != 0 )
        m_albumsHashByName[album]->append( albumList );
    else
    {
        QLinkedList<QStringList*> *list = new QLinkedList<QStringList*>();
        list->append( albumList );
        m_albumsHashByName[album] = list;
    }
    //debug() << "albumInsert returning " << returnedNum;
    return returnedNum;
}

int
ScanResultProcessor::urlId( const QString &url, const QString &uid )
{
    /*
    DEBUG_BLOCK
    foreach( QString key, m_urlsHashByUid.keys() )
    debug() << "Key: " << key << ", list: " << *m_urlsHashByUid[key];
    foreach( int key, m_urlsHashById.keys() )
    debug() << "Key: " << key << ", list: " << *m_urlsHashById[key];
    typedef QPair<int, QString> blahType; //QFOREACH is stupid when it comes to QPairs
    foreach( blahType key, m_urlsHashByLocation.keys() )
    debug() << "Key: " << key << ", list: " << *m_urlsHashByLocation[key];
    */
 
    QFileInfo fileInfo( url );
    const QString dir = fileInfo.absoluteDir().absolutePath();
    int dirId = directoryId( dir );
    int deviceId = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceId, url );

    QPair<int, QString> locationPair( deviceId, rpath );
    //debug() << "in urlId with url = " << url << " and uid = " << uid;
    //debug() << "checking locationPair " << locationPair;
    if( m_urlsHashByLocation.contains( locationPair ) )
    {
        QStringList values;
        if( m_urlsHashByLocation[locationPair] != 0 )
            values = *m_urlsHashByLocation[locationPair];
        else
            values << "zero";
        //debug() << "m_urlsHashByLocation contains it! It is " << values;
    }
    QStringList currUrlIdValues;
    if( m_urlsHashByUid.contains( uid ) && m_urlsHashByUid[uid] != 0 )
        currUrlIdValues = *m_urlsHashByUid[uid];
    else if( m_urlsHashByLocation.contains( locationPair ) && m_urlsHashByLocation[locationPair] != 0 )
        currUrlIdValues = *m_urlsHashByLocation[locationPair];

    if( currUrlIdValues.isEmpty() )  //fresh -- insert
    {
        //debug() << "locationPair did not match!";
        int returnedNum = m_nextUrlNum++;
        QStringList *list = new QStringList();
        list->append( QString::number( returnedNum ) );
        list->append( QString::number( deviceId ) );
        list->append( rpath );
        list->append( QString::number( dirId ) );
        list->append( uid );
        m_urlsHashByUid[uid] = list;
        m_urlsHashById[returnedNum] = list;
        m_urlsHashByLocation[locationPair] = list;
        return returnedNum;
    }

    if( currUrlIdValues[1] == QString::number( deviceId ) &&
        currUrlIdValues[2] == rpath &&
        currUrlIdValues[3] == QString::number( dirId ) &&
        currUrlIdValues[4] == uid
      )
    {
        //everything matches, don't need to do anything, just return the ID
        //debug() << "Everything matches, just returning id";
        return currUrlIdValues[0].toInt();
    }

    if( currUrlIdValues[4] == uid )
    {
        //we found an existing entry with this uniqueid, update the deviceid and path
        //Note that we ignore the situation where both a UID and path was found; UID takes precedence
        //debug() << "found entry with this UID";
        if( m_urlsHashByUid.contains( uid ) && m_urlsHashByUid[uid] != 0 )
        {
            //debug() << "m_urlsHashByUid contains this UID, updating deviceId and path";
            QStringList *list = m_urlsHashByUid[uid];
            //debug() << "list from UID hash is " << list << " with values " << *list;
            list->replace( 1, QString::number( deviceId ) );
            list->replace( 2, rpath );
            list->replace( 3, QString::number( dirId ) );
            //debug() << "Hash updated UID-based values for uid " << uid;
            //Now remove original locations if they exist
            if( m_urlsHashByLocation.contains( locationPair )
                && m_urlsHashByLocation[locationPair] != 0
                && m_urlsHashByLocation[locationPair] != list )
            {
                //debug() << "If condition checked out; removing old stuff";
                //Have existing entries for both location and UID, so to prevent conflicts remove the old
                //entry. This can happen if for instance a track with a changed UID is added in
                //two places to the collection.
                QStringList *oldList = m_urlsHashByLocation[locationPair];
                //debug() << "old list is " << oldList << " with contents " << *oldList;
                m_urlsHashById.remove( oldList->at( 0 ).toInt() );
                m_urlsHashByUid.remove( oldList->at( 4 ) );
                delete oldList;
            }
            m_urlsHashByLocation[locationPair] = list;
        }
        m_permanentTablesUrlUpdates.insert( uid, url );
        m_changedUrls.insert( uid, QPair<QString, QString>( MountPointManager::instance()->getAbsolutePath( currUrlIdValues[1].toInt(), currUrlIdValues[2] ), url ) );
        return currUrlIdValues[0].toInt();
    }

    if( currUrlIdValues[1] == QString::number( deviceId ) && currUrlIdValues[2] == rpath )
    {
        //We found an existing path; give it the most recent UID value
        //debug() << "In urlId, found deviceid " << QString::number( deviceId ) << " and rpath " << rpath;
        if( m_urlsHashByLocation.contains( locationPair ) && m_urlsHashByLocation[locationPair] != 0 )
        {
            QStringList *list = m_urlsHashByLocation[locationPair];
            //debug() << "Replacing hash " << list->at( 4 ) << " with " << uid;
            list->replace( 4, uid );
            if( m_urlsHashByUid.contains( uid )
                && m_urlsHashByUid[uid] != 0 
                && m_urlsHashByUid[uid] != list )
            {
                QStringList *oldList = m_urlsHashByUid[uid];
                m_urlsHashById.remove( oldList->at( 0 ).toInt() );
                m_urlsHashByLocation.remove( QPair<int, QString>( oldList->at( 1 ).toInt(), oldList->at( 2 ) ) );
                delete oldList;
            }
            m_urlsHashByUid[uid] = list;
        }
        m_permanentTablesUidUpdates.insert( url, uid );
        m_changedUids.insert( currUrlIdValues[4], uid );
        return currUrlIdValues[0].toInt();
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
            query += QString( " WHEN '%1' THEN '%2'" ).arg( m_storage->escape( key ),
                                                       m_storage->escape( m_permanentTablesUrlUpdates[key] ) );
            if( first )
                query2 += QString( "'%1'" ).arg( m_storage->escape( key ) );
            else
                query2 += QString( ", '%1'" ).arg( m_storage->escape( key ) );
            first = false;
        }
        query += QString( " END WHERE uniqueid IN(%1);" ).arg( query2 );

        m_storage->query( query );
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
            query += QString( " WHEN '%1' THEN '%2'" ).arg( m_storage->escape( key ),
                                                       m_storage->escape( m_permanentTablesUidUpdates[key] ) );
            if( first )
                query2 += QString( "'%1'" ).arg( m_storage->escape( key ) );
            else
                query2 += QString( ", '%1'" ).arg( m_storage->escape( key ) );
            first = false;
        }
        query += QString( " END WHERE url IN(%1);" ).arg( query2 );

        m_storage->query( query );
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
                        .arg( QString::number( deviceId ), m_storage->escape( rpath ) );
    QStringList result = m_storage->query( query );
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
//     DEBUG_BLOCK
    // "Unknown" albums shouldn't be handled as compilations
    if( album.isEmpty() )
        return 0;

    //check if this album already exists, ignoring the albumartist
    //if it does, and if each file of the album is alone in its directory
    //it's probably a compilation.
    //this handles A1 compilations that were automatically organized by Amarok
    if( !m_albumsHashByName.contains( album ) || m_albumsHashByName[album] == 0 )
        return 0;

    QStringList trackIds;
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
            trackIds << l_trackId;
        }
    }

    if( trackIds.isEmpty() )
    {
        return 0;
    }
    else
    {
        trackIds << QString::number( -1 );
        int compilationId = albumId( album, 0 );
        QString compilationString = QString::number( compilationId );
        foreach( QString trackId, trackIds )
        {
            int value = trackId.toInt();
            if( m_tracksHashById.contains( value ) && m_tracksHashById[value] != 0 )
            {
                QStringList* list = m_tracksHashById[value];
                list->replace( 3, compilationString );
            }
        }
        return compilationId;
    }
}

void
ScanResultProcessor::setupDatabase()
{
//     DEBUG_BLOCK
    if( !m_setupComplete )
    {
        debug() << "Setting up database";
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
        /*
        debug() << "Next URL num: " << m_nextUrlNum;
        foreach( QString key, m_urlsHashByUid.keys() )
            debug() << "Key: " << key << ", list: " << *m_urlsHashByUid[key];
        foreach( int key, m_urlsHashById.keys() )
            debug() << "Key: " << key << ", list: " << *m_urlsHashById[key];
        typedef QPair<int, QString> blahType; //QFOREACH is stupid when it comes to QPairs
        foreach( blahType key, m_urlsHashByLocation.keys() )
            debug() << "Key: " << key << ", list: " << *m_urlsHashByLocation[key];
        debug() << "Next album num: " << m_nextAlbumNum;
        //foreach( int key, m_albumsHashById.keys() )
        //    debug() << "Key: " << key << ", list: " << *m_albumsHashById[key];
        //foreach( QString key, m_albumsHashByName.keys() )
        //{
        //    foreach( QStringList* list, *m_albumsHashByName[key] )
        //       debug() << "Key: " << key << ", list ptrs: " << *list;
        //}
        debug() << "Next track num: " << m_nextTrackNum;
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
    QStringList res = m_storage->query( "SELECT * FROM urls_temp ORDER BY id ASC;" );
    int reserveSize = ( res.size() / 5 ) * 2; //Reserve plenty of space to bring insertion and lookup close to O(1)
    m_urlsHashByUid.reserve( reserveSize );
    m_urlsHashById.reserve( reserveSize );
    m_urlsHashByLocation.reserve( reserveSize );
    QStringList *currList;
    QLinkedList<QStringList*> *llist;
    int index = 0;
    int lastNum = 0;
    while( index < res.size() )
    {
        if( !res.at( index + 4 ).startsWith( "amarok-sqltrackuid" ) )
        {
            debug() << "UHOH: Found track with invalid uid of " << res.at( index + 4 );
            index += 5;
            continue;
        }
        else
        {
            currList = new QStringList();
            lastNum = res.at( index ).toInt();
            for( int i = 0; i < 5; i++ )
                currList->append( res.at(index++) );
            m_urlsHashByUid.insert( currList->last(), currList );
            m_urlsHashById.insert( lastNum, currList );
            QPair<int, QString> locationPair( currList->at( 1 ).toInt(), currList->at( 2 ) );
            //debug() << "inserting locationPair " << locationPair;
            m_urlsHashByLocation.insert( locationPair, currList );
        }
    }
    m_nextUrlNum = lastNum + 1;
    m_storage->query( "DELETE FROM urls_temp;" );

    //albums
    res = m_storage->query( "SELECT * FROM albums_temp ORDER BY id ASC;" );
    reserveSize = ( res.size() / 4 ) * 2;
    m_albumsHashByName.reserve( reserveSize );
    m_albumsHashById.reserve( reserveSize );
    index = 0;
    lastNum = 0;
    while( index < res.size() )
    {
        currList = new QStringList();
        lastNum = res.at( index ).toInt();
        for( int i = 0; i < 4; i++ )
            currList->append( res.at(index++) );
        m_albumsHashById.insert( lastNum, currList );

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
    m_nextAlbumNum = lastNum + 1;
    m_storage->query( "DELETE FROM albums_temp;" );

    //tracks
    res = m_storage->query( "SELECT * FROM tracks_temp ORDER BY id ASC;" );
    reserveSize = ( res.size() / 22 ) * 2;
    m_tracksHashById.reserve( reserveSize );
    index = 0;
    lastNum = 0;
    while( index < res.size() )
    {
        currList = new QStringList();
        lastNum = res.at( index ).toInt();
        for( int i = 0; i < 23; i++ )
            currList->append( res.at(index++) );
        m_tracksHashById.insert( lastNum, currList );
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
    m_nextTrackNum = lastNum + 1;
    m_storage->query( "DELETE FROM tracks_temp;" );

    //artists
    res = m_storage->query( "SELECT * FROM artists_temp ORDER BY id ASC;" );
    m_artists.reserve( res.size() );
    index = 0;
    lastNum = 0;
    while( index < res.size() )
    {
        lastNum = res.at( index++ ).toInt();
        m_artists.insert( res.at( index++ ), lastNum );
    }
    m_nextArtistNum = lastNum + 1;
    m_storage->query( "DELETE FROM artists_temp;" );

    //composers
    res = m_storage->query( "SELECT * FROM composers_temp ORDER BY id ASC;" );
    m_composers.reserve( res.size() );
    index = 0;
    lastNum = 0;
    while( index < res.size() )
    {
        lastNum = res.at( index++ ).toInt();
        m_composers.insert( res.at( index++ ), lastNum );
    }
    m_nextComposerNum = lastNum + 1;
    m_storage->query( "DELETE FROM composers_temp;" );

    //genres
    res = m_storage->query( "SELECT * FROM genres_temp ORDER BY id ASC;" );
    m_genres.reserve( res.size() );
    index = 0;
    lastNum = 0;
    while( index < res.size() )
    {
        lastNum = res.at( index++ ).toInt();
        m_genres.insert( res.at( index++ ), lastNum );
    }
    m_nextGenreNum = lastNum + 1;
    m_storage->query( "DELETE FROM genres_temp;" );

    //images
    res = m_storage->query( "SELECT * FROM images_temp ORDER BY id ASC;" );
    m_imagesFlat.reserve( res.size() );
    index = 0;
    lastNum = 0;
    while( index < res.size() )
    {
        lastNum = res.at( index++ ).toInt();
        m_imagesFlat.insert( res.at( index++ ), lastNum );
    }
    m_nextImageNum = lastNum + 1;
    m_storage->query( "DELETE FROM images_temp;" );

    //years
    res = m_storage->query( "SELECT * FROM years_temp ORDER BY id ASC;" );
    m_years.reserve( res.size() );
    index = 0;
    lastNum = 0;
    while( index < res.size() )
    {
        lastNum = res.at( index++ ).toInt();
        m_years.insert( res.at( index++ ), lastNum );
    }
    m_nextYearNum = lastNum + 1;
    m_storage->query( "DELETE FROM years_temp;" );

}

void
ScanResultProcessor::copyHashesToTempTables()
{
    /*
    debug() << "Next URL num: " << m_nextUrlNum;
    foreach( QString key, m_urlsHashByUid.keys() )
        debug() << "Key: " << key << ", list: " << *m_urlsHashByUid[key];
    foreach( int key, m_urlsHashById.keys() )
        debug() << "Key: " << key << ", list: " << *m_urlsHashById[key];
    typedef QPair<int, QString> blahType; //QFOREACH is stupid when it comes to QPairs
    foreach( blahType key, m_urlsHashByLocation.keys() )
        debug() << "Key: " << key << ", list: " << *m_urlsHashByLocation[key];
    debug() << "Next album num: " << m_nextAlbumNum;
    */
 
    DEBUG_BLOCK
    QString query;
    QString queryStart;
    QString currQuery;
    QStringList *currList;
    QStringList res;
    bool valueReady;

    res = m_storage->query( "SHOW VARIABLES LIKE 'max_allowed_packet';" );
    if( res.size() < 2 || res[1].toInt() == 0 )
    {
        debug() << "Uh oh! For some reason MySQL thinks there isn't a max allowed size!";
        return;
    }
    debug() << "obtained max_allowed_packet is " << res[1];
    int maxSize = res[1].toInt() / 3; //for safety, due to multibyte encoding

    //urls
    debug() << "urls key size is " << m_urlsHashById.keys().size();
    queryStart = "INSERT INTO urls_temp VALUES ";
    query = queryStart;
    valueReady = false;
    QList<int> keys = m_urlsHashById.keys();
    qSort( keys );
    QSet<int> invalidUrls;
    foreach( int key, keys )
    {
        currList = m_urlsHashById[key];
        if( !currList->at( 4 ).startsWith( "amarok-sqltrackuid" ) )
        {
            debug() << "UHOH: Trying to insert invalid entry into urls with id of " << currList->at( 0 ) << " and uid of " << currList->at( 4 );
            invalidUrls << currList->at( 0 ).toInt();
            continue;
        }
        //debug() << "inserting following list: " << currList;
        currQuery =   "(" + currList->at( 0 ) + ","
                          + ( currList->at( 1 ).isEmpty() ? "NULL" : currList->at( 1 ) ) + ","
                          + "'" + m_storage->escape( currList->at( 2 ) ) + "',"
                          + ( currList->at( 3 ).isEmpty() ? "NULL" : currList->at( 3 ) ) + ","
                          + "'" + m_storage->escape( currList->at( 4 ) ) + "')"; //technically allowed to be NULL but it's the primary key so won't get far
        if( query.size() + currQuery.size() + 1 >= maxSize - 3 ) // ";"
        {
            query += ";";
            //debug() << "inserting " << query << ", size " << query.size();
            m_storage->insert( query, QString() );
            query = queryStart;
            valueReady = false;
        }

        if( !valueReady )
        {
            query += currQuery;
            //debug() << "appending " << currQuery;
            valueReady = true;
        }
        else
        {
            //debug() << "appending , + " << currQuery;
            query += "," + currQuery;
        }
    }
    if( query != queryStart )
    {
        query += ";";
        //debug() << "inserting " << query << ", size " << query.size();
        m_storage->insert( query, QString() );
    }
    keys.clear();

    //albums
    queryStart = "INSERT INTO albums_temp VALUES ";
    query = queryStart;
    valueReady = false;
    keys = m_albumsHashById.keys();
    qSort( keys );
    foreach( int key, keys )
    {
        currList = m_albumsHashById[key];
        currQuery =   "(" + currList->at( 0 ) + ","
                          + "'" + m_storage->escape( currList->at( 1 ) ) + "',"
                          + ( currList->at( 2 ).isEmpty() ? "NULL" : currList->at( 2 ) ) + ","
                          + ( currList->at( 3 ).isEmpty() ? "NULL" : currList->at( 3 ) ) + ")";
        if( query.size() + currQuery.size() + 1 >= maxSize - 3 ) // ";"
        {
            query += ";";
            //debug() << "inserting " << query << ", size " << query.size();
            m_storage->insert( query, QString() );
            query = queryStart;
            valueReady = false;
        }

        if( !valueReady )
        {
            query += currQuery;
            valueReady = true;
        }
        else
            query += "," + currQuery;
    }
    if( query != queryStart )
    {
        query += ";";
        //debug() << "inserting " << query << ", size " << query.size();
        m_storage->insert( query, QString() );
    }
    keys.clear();

    //tracks
    debug() << "tracks key size is " << m_tracksHashById.keys().size();
    queryStart = "INSERT INTO tracks_temp VALUES ";
    query = queryStart;
    valueReady = false;
    keys = m_tracksHashById.keys();
    qSort( keys );
    foreach( int key, keys )
    {
        //debug() << "key = " << key << ", id = " << m_tracksHashById[key]->at( 0 );
        currList = m_tracksHashById[key];
        if( invalidUrls.contains( currList->at( 1 ).toInt() ) )
        {
            debug() << "UHOH: Skipping track with url id " << currList->at( 1 ) << " because it's in the invalidUrls set";
            continue;
        }
        currQuery =   "(" + currList->at( 0 ) + ","                                               //id
                          + ( currList->at( 1 ).isEmpty() ? "NULL" : currList->at( 1 ) ) + ","    //url
                          + ( currList->at( 2 ).isEmpty() ? "NULL" : currList->at( 2 ) ) + ","    //artist
                          + ( currList->at( 3 ).isEmpty() ? "NULL" : currList->at( 3 ) ) + ","    //album
                          + ( currList->at( 4 ).isEmpty() ? "NULL" : currList->at( 4 ) ) + ","    //genre
                          + ( currList->at( 5 ).isEmpty() ? "NULL" : currList->at( 5 ) ) + ","    //composer
                          + ( currList->at( 6 ).isEmpty() ? "NULL" : currList->at( 6 ) ) + ","    //year
                          + "'" + m_storage->escape( currList->at( 7 ) ) + "',"                //title
                          + "'" + m_storage->escape( currList->at( 8 ) ) + "',"                //text
                          + ( currList->at( 9 ).isEmpty() ? "NULL" : currList->at( 9 ) ) + ","    //tracknumber
                          + ( currList->at( 10 ).isEmpty() ? "NULL" : currList->at( 10 ) ) + ","  //discnumber
                          + ( currList->at( 11 ).isEmpty() ? "NULL" : currList->at( 11 ) ) + ","  //bitrate
                          + ( currList->at( 12 ).isEmpty() ? "NULL" : currList->at( 12 ) ) + ","  //length
                          + ( currList->at( 13 ).isEmpty() ? "NULL" : currList->at( 13 ) ) + ","  //samplerate
                          + ( currList->at( 14 ).isEmpty() ? "NULL" : currList->at( 14 ) ) + ","  //filesize
                          + ( currList->at( 15 ).isEmpty() ? "NULL" : currList->at( 15 ) ) + ","  //filetype
                          + ( currList->at( 16 ).isEmpty() ? "NULL" : QString( currList->at( 16 ) ).replace( ',' , '.' ) ) + ","  //bpm
                          + ( currList->at( 17 ).isEmpty() ? "NULL" : currList->at( 17 ) ) + ","  //createdate
                          + ( currList->at( 18 ).isEmpty() ? "NULL" : currList->at( 18 ) ) + ","  //modifydate
                          + ( currList->at( 19 ).isEmpty() ? "NULL" : QString( currList->at( 19 ) ).replace( ',' , '.' ) ) + ","  //albumgain
                          + ( currList->at( 20 ).isEmpty() ? "NULL" : QString( currList->at( 20 ) ).replace( ',' , '.' ) ) + ","  //albumpeakgain
                          + ( currList->at( 21 ).isEmpty() ? "NULL" : QString( currList->at( 21 ) ).replace( ',' , '.' ) ) + ","  //trackgain
                          + ( currList->at( 22 ).isEmpty() ? "NULL" : QString( currList->at( 22 ) ).replace( ',' , '.' ) ) + ")"; //trackpeakgain
        if( query.size() + currQuery.size() + 1 >= maxSize - 3 ) // ";"
        {
            query += ";";
            //debug() << "inserting " << query << ", size " << query.size();
            m_storage->insert( query, QString() );
            query = queryStart;
            valueReady = false;
        }

        if( !valueReady )
        {
            query += currQuery;
            valueReady = true;
        }
        else
            query += "," + currQuery;
    }
    if( query != queryStart )
    {
        query += ";";
        //debug() << "inserting " << query << ", size " << query.size();
        m_storage->insert( query, QString() );
    }

    genericCopyHash( "artists", &m_artists, maxSize );
    genericCopyHash( "composers", &m_composers, maxSize );
    genericCopyHash( "genres", &m_genres, maxSize );
    genericCopyHash( "images", &m_imagesFlat, maxSize );
    genericCopyHash( "years", &m_years, maxSize );
}

void
ScanResultProcessor::genericCopyHash( const QString &tableName, const QHash<QString, int> *hash, int maxSize )
{
    QString currString;
    QString currQuery;
    QString queryStart = "INSERT INTO " + tableName + "_temp VALUES ";
    QString query = queryStart;
    bool valueReady = false;
    QStringList keys = hash->keys();
    QHash<int, QString> sortedHash;
    foreach( QString key, keys )
        sortedHash.insert( hash->value( key ), key );
    QList<int> intKeys = sortedHash.keys();
    qSort( intKeys );
    foreach( int key, intKeys )
    {

        currString = sortedHash[key];
        //currQuery =   "(" + QString::number( hash->value( key ) ) + ",'" + m_storage->escape( key ) + "')";
        currQuery =   "(" + QString::number( key ) + ",'" + m_storage->escape( sortedHash[key] ) + "')";
        if( query.size() + currQuery.size() + 1 >= maxSize - 3 ) // ";"
        {
            query += ";";
            //debug() << "inserting " << query << ", size " << query.size();
            m_storage->insert( query, QString() );
            query = queryStart;
            valueReady = false;
        }

        if( !valueReady )
        {
            query += currQuery;
            valueReady = true;
        }
        else
            query += "," + currQuery;
    }
    if( query != queryStart )
    {
        query += ";";
        //debug() << "inserting " << query << ", size " << query.size();
        m_storage->insert( query, QString() );
    }
}

#include "ScanResultProcessor.moc"

