// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "config.h"

#include "amarokconfig.h"
#include "collectionbrowser.h"    //updateTags()
#include "collectiondb.h"
#include "coverfetcher.h"
#include "enginecontroller.h"
#include "metabundle.h"           //updateTags()
#include "playlistbrowser.h"
#include "threadweaver.h"

#include <qfile.h>
#include <qimage.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kinputdialog.h>         //setupCoverFetcher()
#include <kio/job.h>
#include <klineedit.h>            //setupCoverFetcher()
#include <klocale.h>
#include <kmdcodec.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <time.h>                 //query()
#include <unistd.h>

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionDB
//////////////////////////////////////////////////////////////////////////////////////////

CollectionEmitter* CollectionDB::s_emitter = 0;


#ifdef __USE_MYSQL
//TODO: this has to go into Settings dialog
#define __MYSQL_HOST "localhost"
#define __MYSQL_PORT 3306
#define __MYSQL_USER "root"
#define __MYSQL_PASSWORD "root"
#define __MYSQL_DB "amarok"
#endif

CollectionDB::CollectionDB()
        : m_weaver( new ThreadWeaver( this ) )
        , m_cacheDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) )
        , m_coverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) )
{
#ifdef __USE_MYSQL
    mysql::mysql_init(&m_db);
    if (mysql::mysql_real_connect(&m_db, __MYSQL_HOST, __MYSQL_USER, __MYSQL_PASSWORD, __MYSQL_DB, __MYSQL_PORT, NULL, CLIENT_COMPRESS))
    {
        if (!isValid())
        	createTables();
    }
    else
    {
        if (mysql::mysql_real_connect(&m_db, __MYSQL_HOST, __MYSQL_USER, __MYSQL_PASSWORD, NULL, __MYSQL_PORT, NULL, CLIENT_COMPRESS))
        {
            mysql::mysql_query(&m_db, "CREATE DATABASE " __MYSQL_DB);
            createTables();
        }
    }
#else
    QCString path = ( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" )
                  + "collection.db" ).local8Bit();

    // Open database file and check for correctness
    bool failOpen = true;
    QFile file( path );
    if ( file.open( IO_ReadOnly ) ) {
        QString format;
        file.readLine( format, 50 );
        if ( !format.startsWith( "SQLite format 3" ) ) {
            kdWarning() << "Database versions incompatible. Removing and rebuilding database.\n";
        }
        else if ( sqlite3_open( path, &m_db ) != SQLITE_OK ) {
            kdWarning() << "Database file corrupt. Removing and rebuilding database.\n";
            sqlite3_close( m_db );
        }
        else
            failOpen = false;
    }
    if ( failOpen ) {
        // Remove old db file; create new
        QFile::remove( path );
        sqlite3_open( path, &m_db );
    }
#endif

    // create cover dir, if it doesn't exist.
    if( !m_coverDir.exists( "albumcovers", false ) )
        m_coverDir.mkdir( "albumcovers", false );
    m_coverDir.cd( "albumcovers" );

     // create image cache dir, if it doesn't exist.
    if( !m_cacheDir.exists( "albumcovers/cache", false ) )
        m_cacheDir.mkdir( "albumcovers/cache", false );
    m_cacheDir.cd( "albumcovers/cache" );

    if ( !s_emitter ) s_emitter = new CollectionEmitter();
}


CollectionDB::~CollectionDB()
{
#ifdef __USE_MYSQL
    mysql::mysql_close( &m_db );
#else
    sqlite3_close( m_db );
#endif
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * Executes a SQL query on the already opened database
 * @param statement SQL program to execute. Only one SQL statement is allowed.
 * @param debug     Set to true for verbose debug output.
 * @retval names    Will contain all column names, set to NULL if not used.
 * @return          The queried data, or QStringList() on error.
 */
QStringList
CollectionDB::query( const QString& statement, QStringList* const names, bool debug )
{
    m_names.clear();
    m_values.clear();

    if ( debug )
        kdDebug() << "query-start: " << statement << endl;

    QStringList values;

    clock_t start = clock();

#ifdef __USE_MYSQL
    if (!mysql::mysql_query(&m_db, statement.ascii()))
    {
        mysql::MYSQL_RES* result;
        if ((result = mysql::mysql_use_result(&m_db)))
        {
            if (names)
            {
                mysql::MYSQL_FIELD* field;
                while ((field = mysql::mysql_fetch_field(result)))
                    *names  << QString( field->name );
            }

            int number = mysql::mysql_field_count(&m_db);
            mysql::MYSQL_ROW row;
            while ((row = mysql::mysql_fetch_row(result)))
            {
                for ( int i = 0; i < number; i++ )
                {
                  values << QString::fromUtf8( (const char*)row[i] );
                }
            }
            mysql::mysql_free_result(result);
        }
        else
        {
            if (mysql::mysql_field_count(&m_db) != 0)
            {
                kdDebug() << "MYSQL QUERY FAILED: " << mysql::mysql_error(&m_db) << "\n" << "FAILED QUERY: " << statement << "\n";
                return false;
            }
        }
    }
    else
    {
        kdDebug() << "MYSQL QUERY FAILED: " << mysql::mysql_error(&m_db) << "\n" << "FAILED QUERY: " << statement << "\n";
        return false;
    }
#else
    if ( !m_db )
    {
        kdError() << k_funcinfo << "[CollectionDB] SQLite pointer == NULL.\n";
        return QStringList();
    }

    int error;
    const char* tail;
    sqlite3_stmt* stmt;

    //compile SQL program to virtual machine
    error = sqlite3_prepare( m_db, statement.utf8(), statement.length(), &stmt, &tail );

    if ( error != SQLITE_OK )
    {
        kdError() << k_funcinfo << "[CollectionDB] sqlite3_compile error:" << endl;
        kdError() << sqlite3_errmsg( m_db ) << endl;
        kdError() << "on query: " << statement << endl;

        return QStringList();
    }

    int busyCnt = 0;
    int number = sqlite3_column_count( stmt );
    //execute virtual machine by iterating over rows
    while ( true )
    {
        error = sqlite3_step( stmt );

        if ( error == SQLITE_BUSY )
        {
            if ( busyCnt++ > 20 ) {
                kdError() << "[CollectionDB] Busy-counter has reached maximum. Aborting this sql statement!\n";
                break;
            }
            ::usleep( 100000 ); // Sleep 100 msec
            kdDebug() << "[CollectionDB] sqlite3_step: BUSY counter: " << busyCnt << endl;
        }
        if ( error == SQLITE_MISUSE )
            kdDebug() << "[CollectionDB] sqlite3_step: MISUSE" << endl;
        if ( error == SQLITE_DONE || error == SQLITE_ERROR )
            break;

        //iterate over columns
        for ( int i = 0; i < number; i++ )
        {
            values << QString::fromUtf8( (const char*) sqlite3_column_text( stmt, i ) );
            if ( names ) *names << QString( sqlite3_column_name( stmt, i ) );
        }
    }
    //deallocate vm ressources
    sqlite3_finalize( stmt );

    if ( error != SQLITE_DONE )
    {
        kdError() << k_funcinfo << "sqlite_step error.\n";
        kdError() << sqlite3_errmsg( m_db ) << endl;
        kdError() << "on query: " << statement << endl;

        return QStringList();
    }
#endif

    if ( debug )
    {
        clock_t finish = clock();
        const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
        kdDebug() << "[CollectionDB] SQL-query (" << duration << "s): " << statement << endl;
    }

    // Cache results in member variables for convenience access
    m_values = values;
    if ( names ) m_names = *names;

    return values;
}


/**
 * Returns the rowid of the most recently inserted row
 * @return          int rowid
 */
int
CollectionDB::sqlInsertID()
{
#ifdef __USE_MYSQL
    return mysql::mysql_insert_id(&m_db);
#else
    if ( !m_db )
    {
        kdWarning() << k_funcinfo << "SQLite pointer == NULL.\n";
        return -1;
    }

    return sqlite3_last_insert_rowid( m_db );
#endif
}


QString
CollectionDB::escapeString( QString string )
{
    string.replace( "'", "''" );
    return string;
}


bool
CollectionDB::isValid()
{
    QStringList values1 = query( "SELECT COUNT( url ) FROM tags LIMIT 0, 1;" );
    QStringList values2 = query( "SELECT COUNT( url ) FROM statistics LIMIT 0, 1;" );

    return !values1.isEmpty() || !values2.isEmpty();
}


bool
CollectionDB::isEmpty()
{
    query( "SELECT COUNT( url ) FROM tags LIMIT 0, 1;" );

    return m_values.isEmpty() ? true : m_values.first() == "0";
}


QString
CollectionDB::albumSongCount( const QString &artist_id, const QString &album_id )
{
    query( QString( "SELECT COUNT( url ) FROM tags WHERE album = %1 AND artist = %2;" )
                    .arg( album_id )
                    .arg( artist_id ) );

    return m_values.first();
}


QString
CollectionDB::getPathForAlbum( const QString &artist, const QString &album )
{
    query( QString( "SELECT tags.url FROM tags, album, artist WHERE tags.album = album.id AND album.name = '%1' AND tags.artist = artist.id AND artist.name = '%2' LIMIT 0, 1;" )
                    .arg( escapeString( album ) )
                    .arg( escapeString( artist ) ) );

    return m_values.first();
}


QString
CollectionDB::getPathForAlbum( const uint artist_id, const uint album_id )
{
    query( QString( "SELECT url FROM tags WHERE album = %1 AND artist = %2 LIMIT 0, 1;" )
                    .arg( album_id )
                    .arg( artist_id ) );

    return m_values.first();
}


void
CollectionDB::addImageToPath( const QString path, const QString image, bool temporary )
{
    query( QString( "INSERT INTO images%1 ( path, name ) VALUES ( '%1', '%2' );" )
     .arg( temporary ? "_temp" : "" )
     .arg( escapeString( path ) )
     .arg( escapeString( image ) ) );
}


bool
CollectionDB::setAlbumImage( const QString& artist, const QString& album, const KURL& url )
{
    QImage img( url.path() );
    return setAlbumImage( artist, album, img );
}


bool
CollectionDB::setAlbumImage( const QString& artist, const QString& album, QImage img, const QString& amazonUrl )
{
    // remove existing album covers
    removeAlbumImage( artist, album );

    QDir largeCoverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/albumcovers/large/" ) );
    KMD5 context( artist.lower().local8Bit() + album.lower().local8Bit() );
    QCString key = context.hexDigest();

    // Save Amazon product page URL as embedded string, for later retreival
    if ( !amazonUrl.isEmpty() )
        img.setText( "amazon-url", 0, amazonUrl );

    return img.save( largeCoverDir.filePath( key ), "PNG");
}


QString
CollectionDB::albumImage( const QString &artist, const QString &album, uint width )
{
    // we aren't going to need a 1x1 size image. this is just a quick hack to be able to show full size images.
    if ( width == 1) width = AmarokConfig::coverPreviewSize();
    
    QCString widthKey = QString::number( width ).local8Bit() + "@";

    if ( artist.isEmpty() && album.isEmpty() )
    {
        if ( m_cacheDir.exists( widthKey + "nocover.png" ) )
            return m_cacheDir.filePath( widthKey + "nocover.png" );
        else
        {
            QImage nocover( locate( "data", "amarok/images/nocover.png" ) );
            nocover.smoothScale( width, width ).save( m_cacheDir.filePath( widthKey + "nocover.png" ), "PNG" );
            return m_cacheDir.filePath( widthKey + "nocover.png" );
        }
    }
    else
    {
        KMD5 context( artist.lower().local8Bit() + album.lower().local8Bit() );
        QCString key = context.hexDigest();

        // check cache for existing cover
        if ( m_cacheDir.exists( widthKey + key ) )
            return m_cacheDir.filePath( widthKey + key );
        else
        {
            // we need to create a scaled version of this cover
            QDir largeCoverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/albumcovers/large/" ) );
            if ( largeCoverDir.exists( key ) )
                if ( width > 1 )
                {
                    QImage img( largeCoverDir.filePath( key ) );
                    img.smoothScale( width, width ).save( m_cacheDir.filePath( widthKey + key ), "PNG" );

                    return m_cacheDir.filePath( widthKey + key );
                }
                else
                    return largeCoverDir.filePath( key );
        }

        // no amazon cover found, let's try to find a cover in the song's directory
        KURL url;
        url.setPath( getPathForAlbum( artist, album ) );

        return getImageForPath( url.directory(), width );
    }
}

QString
CollectionDB::albumImage( const uint artist_id, const uint album_id, const uint width )
{
    return albumImage( artistValue( artist_id ), albumValue( album_id ), width );
}


QString
CollectionDB::getImageForPath( const QString path, uint width )
{
    if ( !width ) width = AmarokConfig::coverPreviewSize();
    QString widthKey = QString::number( width ) + "@";

    if ( path.isEmpty() )
    {
        if( m_cacheDir.exists( widthKey + "nocover.png" ) )
            return m_cacheDir.filePath( widthKey + "nocover.png" );
        else
        {
            QImage nocover( locate( "data", "amarok/images/nocover.png" ) );
            nocover.smoothScale( width, width ).save( m_cacheDir.filePath( widthKey + "nocover.png" ), "PNG" );
            return m_cacheDir.filePath( widthKey + "nocover.png" );
        }
    }

    KURL file( path );
    QString filename( QString::number( width ) + "@" + file.fileName() );
    filename.replace( "'", "_" ).append( ".png" );

#ifdef AMAZON_SUPPORT
    if ( m_cacheDir.exists( filename.lower() ) )
        return m_cacheDir.absPath() + "/" + filename.lower();
#endif
    query( QString( "SELECT name FROM images WHERE path = '%1';" )
                    .arg( escapeString( path ) ) );

    if ( !m_values.isEmpty() )
    {
        QString image( m_values.first() );
        for ( uint i = 0; i < m_values.count(); i++ )
            if ( m_values[i].contains( "front", false ) )
                image = m_values[i];

        if ( width > 0 )
        {
            QImage img = QImage( path + "/" + image );
            img.smoothScale( width, width ).save( m_cacheDir.absPath() + "/" + filename.lower(), "PNG" );
            return m_cacheDir.absPath() + "/" + filename.lower();

        } else

            return path + "/" + image;
    }

    if( m_cacheDir.exists( widthKey + "nocover.png" ) )
        return m_cacheDir.filePath( widthKey + "nocover.png" );
    else
    {
        QImage nocover( locate( "data", "amarok/images/nocover.png" ) );
        nocover.smoothScale( width, width ).save( m_cacheDir.filePath( widthKey + "nocover.png" ), "PNG" );
        return m_cacheDir.filePath( widthKey + "nocover.png" );
    }
}


bool
CollectionDB::removeAlbumImage( const uint artist_id, const uint album_id )
{
    return removeAlbumImage( artistValue( artist_id ), albumValue( album_id ) );
}


bool
CollectionDB::removeAlbumImage( const QString &artist, const QString &album )
{
    QCString widthKey = "*@";
    KMD5 context( artist.lower().local8Bit() + album.lower().local8Bit() );
    QCString key = context.hexDigest();

    // remove scaled versions of images
    QStringList scaledList = m_cacheDir.entryList( widthKey + key );
    if ( scaledList.count() > 0 )
        for ( uint i = 0; i < scaledList.count(); i++ )
            QFile::remove( m_cacheDir.filePath( scaledList[ i ] ) );

    // remove large, original image
    QDir largeCoverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/albumcovers/large/" ) );
    if ( largeCoverDir.exists( key ) )
        return QFile::remove( largeCoverDir.filePath( key ) );

    return false;
}


QStringList
CollectionDB::artistList( bool withUnknown, bool withCompilations )
{
    if ( withUnknown && withCompilations )
        return query( "SELECT DISTINCT name FROM artist "
                      "ORDER BY lower( name );" );
    else
        return query( "SELECT DISTINCT artist.name FROM tags, artist WHERE 1 " +
                      ( withUnknown ? QString() : "AND artist.name <> 'Unknown' " ) +
                      ( withCompilations ? QString() : "AND tags.artist = artist.id AND tags.sampler = 0 " ) +
                      "ORDER BY lower( artist.name );" );
}


QStringList
CollectionDB::albumList( bool withUnknown, bool withCompilations )
{
    if ( withUnknown && withCompilations )
        return query( "SELECT DISTINCT name FROM album "
                      "ORDER BY lower( name );" );
    else
        return query( "SELECT DISTINCT album.name FROM tags, album WHERE 1 " +
                      ( withUnknown ? QString() : "AND album.name <> 'Unknown' " ) +
                      ( withCompilations ? QString() : "AND tags.album = album.id AND tags.sampler = 0 " ) +
                      "ORDER BY lower( album.name );" );
}


QStringList
CollectionDB::albumListOfArtist( const QString &artist, bool withUnknown, bool withCompilations )
{
    return query( "SELECT DISTINCT album.name FROM tags, album, artist WHERE "
                  "tags.album = album.id AND tags.artist = artist.id "
                  "AND artist.name = '" + escapeString( artist ) + "' " +
                  ( withUnknown ? QString() : "AND album.name <> 'Unknown' " ) +
                  ( withCompilations ? QString() : "AND tags.sampler = 0 " ) +
                  "ORDER BY lower( album.name );" );
}


QStringList
CollectionDB::artistAlbumList( bool withUnknown, bool withCompilations )
{
    return query( "SELECT DISTINCT artist.name, album.name FROM tags, album, artist WHERE "
                  "tags.album = album.id AND tags.artist = artist.id " +
                  ( withUnknown ? QString() : "AND album.name <> 'Unknown' AND artist.name <> 'Unknown' " ) +
                  ( withCompilations ? QString() : "AND tags.sampler = 0 " ) +
                  "ORDER BY lower( album.name );" );
}


bool
CollectionDB::getMetaBundleForUrl( const QString &url , MetaBundle *bundle )
{
    query( QString( "SELECT album.name, artist.name, genre.name, tags.title, year.name, tags.comment, tags.track, tags.bitrate, tags.length, tags.samplerate "
                    "FROM tags, album, artist, genre, year "
                    "WHERE album.id = tags.album AND artist.id = tags.artist AND genre.id = tags.genre AND year.id = tags.year AND url = '%1';" )
                    .arg( escapeString( url ) ) );

    if ( !m_values.isEmpty() )
    {
        bundle->setAlbum( m_values[0] );
        bundle->setArtist( m_values[1] );
        bundle->setGenre( m_values[2] );
        bundle->setTitle( m_values[3] );
        bundle->setYear( m_values[4] );
        bundle->setComment( m_values[5] );
        bundle->setTrack( m_values[6] );
        bundle->setBitrate( m_values[7].toInt() );
        bundle->setLength( m_values[8].toInt() );
        bundle->setSampleRate( m_values[9].toInt() );

        bundle->setUrl( url );

        return true;
    }

    return false;
}


void
CollectionDB::addAudioproperties( const MetaBundle& bundle )
{
    query( QString( "UPDATE tags SET bitrate='%1', length='%2', samplerate='%3' WHERE url='%4';" )
                    .arg( bundle.bitrate() )
                    .arg( bundle.length() )
                    .arg( bundle.sampleRate() )
                    .arg( escapeString( bundle.url().path() ) ) );
}


int
CollectionDB::addSongPercentage( const QString &url , const int percentage )
{
    float score;

    query( QString( "SELECT playcounter, createdate, percentage FROM statistics WHERE url = '%1';" )
                    .arg( escapeString( url ) ) );

    if ( !m_values.isEmpty() )
    {
        // entry exists, increment playcounter and update accesstime
        score = ( ( m_values[2].toDouble() * m_values.first().toInt() ) + percentage ) / ( m_values.first().toInt() + 1 );

        query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
#ifdef __USE_MYSQL
                        "VALUES ( '%1', %2, %3, %4, %5 );" )
                        .arg( escapeString( url ) )
                        .arg( m_values[1] )
                        .arg( QDateTime::currentDateTime().toTime_t() ) //TODO: maybe this could be used for sqlite too?
                        .arg( score )
                        .arg( m_values[0] + " + 1" ) );
#else
                        "VALUES ( '%1', '%2', strftime('%s', 'now'), %3, %4 );" )
                        .arg( escapeString( url ) )
                        .arg( m_values[1] )
                        .arg( score )
                        .arg( m_values[0] + " + 1" ) );
#endif
    }
    else
    {
        // entry didnt exist yet, create a new one
        score = ( ( 50 + percentage ) / 2 );

        query( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
#ifdef __USE_MYSQL
                        "VALUES ( '%1', %2, %3, %4, 1 );" )
                        .arg( escapeString( url ) )
                        .arg( QDateTime::currentDateTime().toTime_t() ) //TODO: maybe this could be used for sqlite too?
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( score ) );
#else
                        "VALUES ( '%1', strftime('%s', 'now'), strftime('%s', 'now'), %2, 1 );" )
                        .arg( escapeString( url ) )
                        .arg( score ) );
#endif
    }

    int iscore = getSongPercentage( url );
    emit s_emitter->scoreChanged( url, iscore );
    return iscore;
}


int
CollectionDB::getSongPercentage( const QString &url  )
{
    QStringList values = query( QString( "SELECT round( percentage + 0.4 ) FROM statistics WHERE url = '%1';" )
                                         .arg( escapeString( url ) ) );

    if( values.count() )
        return values.first().toInt();

    return 0;
}


void
CollectionDB::setSongPercentage( const QString &url , int percentage )
{
    query( QString( "SELECT playcounter, createdate, accessdate FROM statistics WHERE url = '%1';" )
                    .arg( escapeString( url ) ) );

    // check boundaries
    if ( percentage > 100 ) percentage = 100;
    if ( percentage < 1 )   percentage = 1;

    if ( !m_values.isEmpty() )
    {
        // entry exists
        query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                        "VALUES ( '%1', '%2', '%3', %4, %5 );" )
                        .arg( escapeString( url ) )
                        .arg( m_values[1] )
                        .arg( m_values[2] )
                        .arg( percentage )
                        .arg( m_values[0] ) );
    }
    else
    {
        query( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
#ifdef __USE_MYSQL
                        "VALUES ( '%1', %2, %3, %4, 0 );" )
                        .arg( escapeString( url ) )
                        .arg( QDateTime::currentDateTime().toTime_t() ) //TODO: maybe this could be used for sqlite too?
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( percentage ) );
#else
                        "VALUES ( '%1', strftime('%s', 'now'), strftime('%s', 'now'), %2, 0 );" )
                        .arg( escapeString( url ) )
                        .arg( percentage ) );
#endif
    }

    emit s_emitter->scoreChanged( url, percentage );
}


void
CollectionDB::updateDirStats( QString path, const long datetime )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    query( QString( "REPLACE INTO directories ( dir, changedate ) VALUES ( '%1', %2 );" )
                    .arg( escapeString( path ) )
                    .arg( datetime ) );
}


void
CollectionDB::removeSongsInDir( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    query( QString( "DELETE FROM tags WHERE dir = '%1';" )
                    .arg( escapeString( path ) ) );
}


bool
CollectionDB::isDirInCollection( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    query( QString( "SELECT changedate FROM directories WHERE dir = '%1';" )
                    .arg( escapeString( path ) ) );

    return !m_values.isEmpty();
}


bool
CollectionDB::isFileInCollection( const QString &url  )
{
    query( QString( "SELECT url FROM tags WHERE url = '%1';" )
                    .arg( escapeString( url ) ) );

    return !m_values.isEmpty();
}


bool
CollectionDB::isSamplerAlbum( const QString &album )
{
    QStringList values_artist;
    QStringList values_dir;

    if ( album == "Unknown" || album.isEmpty() )
        return false;

    const uint album_id = albumID( album, FALSE, FALSE );
    values_artist = query( QString( "SELECT DISTINCT artist.name FROM artist, tags WHERE tags.artist = artist.id AND tags.album = '%1';" )
                                    .arg( album_id ) );
    values_dir    = query( QString( "SELECT DISTINCT dir FROM tags WHERE album = '%1';" )
                                    .arg( album_id ) );

    if ( values_artist.count() > values_dir.count() )
    {

        query( QString( "UPDATE tags SET sampler = 1 WHERE album = '%1';" )
                        .arg( album_id ) );
        return true;
    }

    return false;
}


void
CollectionDB::removeDirFromCollection( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    query( QString( "DELETE FROM directories WHERE dir = '%1';" )
                    .arg( escapeString( path ) ) );
}


void
CollectionDB::createTables( bool temporary )
{
    kdDebug() << k_funcinfo << endl;

    //create tag table
    query( QString( "CREATE %1 TABLE tags%2 ("
                    "url VARCHAR(255),"
                    "dir VARCHAR(255),"
                    "createdate INTEGER,"
                    "album INTEGER,"
                    "artist INTEGER,"
                    "genre INTEGER,"
                    "title VARCHAR(255),"
                    "year INTEGER,"
                    "comment VARCHAR(255),"
#ifdef __USE_MYSQL
                    "track NUMERIC(4),"
#else
                    "track NUMBER(4),"
#endif
                    "bitrate INTEGER,"
                    "length INTEGER,"
                    "samplerate INTEGER,"
#ifdef __USE_MYSQL
                    "sampler BOOL );" )
#else
                    "sampler BOOLEAN );" )
#endif
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create album table
    query( QString( "CREATE %1 TABLE album%2 ("
#ifdef __USE_MYSQL
                    "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
#else
                    "id INTEGER PRIMARY KEY,"
#endif
                    "name VARCHAR(255) );" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create artist table
    query( QString( "CREATE %1 TABLE artist%2 ("
#ifdef __USE_MYSQL
                    "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
#else
                    "id INTEGER PRIMARY KEY,"
#endif
                    "name VARCHAR(255) );" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create genre table
    query( QString( "CREATE %1 TABLE genre%2 ("
#ifdef __USE_MYSQL
                    "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
#else
                    "id INTEGER PRIMARY KEY,"
#endif
                    "name VARCHAR(255) );" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create year table
    query( QString( "CREATE %1 TABLE year%2 ("
#ifdef __USE_MYSQL
                    "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
#else
                    "id INTEGER PRIMARY KEY,"
#endif
                    "name VARCHAR(4) );" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create images table
    query( QString( "CREATE %1 TABLE images%2 ("
                    "path VARCHAR(255),"
                    "name VARCHAR(255) );" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create indexes
    query( QString( "CREATE INDEX album_idx%1 ON album%2( name );" )
                    .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "CREATE INDEX artist_idx%1 ON artist%2( name );" )
                    .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "CREATE INDEX genre_idx%1 ON genre%2( name );" )
                    .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "CREATE INDEX year_idx%1 ON year%2( name );" )
                    .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );

    if ( !temporary )
    {
        query( "CREATE INDEX url_tag ON tags( url );" );
        query( "CREATE INDEX album_tag ON tags( album );" );
        query( "CREATE INDEX artist_tag ON tags( artist );" );
        query( "CREATE INDEX genre_tag ON tags( genre );" );
        query( "CREATE INDEX year_tag ON tags( year );" );
        query( "CREATE INDEX sampler_tag ON tags( sampler );" );

        // create directory statistics database
        query( QString( "CREATE TABLE directories ("
                        "dir VARCHAR(255) UNIQUE,"
                        "changedate INTEGER );" ) );
    }
}


void
CollectionDB::dropTables( bool temporary )
{
    kdDebug() << k_funcinfo << endl;

    query( QString( "DROP TABLE tags%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE album%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE artist%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE genre%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE year%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE images%1;" ).arg( temporary ? "_temp" : "" ) );
}


void
CollectionDB::moveTempTables()
{
    query( "INSERT INTO tags SELECT * FROM tags_temp;" );
    query( "INSERT INTO album SELECT * FROM album_temp;" );
    query( "INSERT INTO artist SELECT * FROM artist_temp;" );
    query( "INSERT INTO genre SELECT * FROM genre_temp;" );
    query( "INSERT INTO year SELECT * FROM year_temp;" );
    query( "INSERT INTO images SELECT * FROM images_temp;" );
}


void
CollectionDB::createStatsTable()
{
    kdDebug() << k_funcinfo << endl;

    // create music statistics database
    query( QString( "CREATE TABLE statistics ("
                    "url VARCHAR(255) UNIQUE,"
                    "createdate INTEGER,"
                    "accessdate INTEGER,"
                    "percentage FLOAT,"
                    "playcounter INTEGER );" ) );

    query( "CREATE INDEX url_stats ON statistics( url );" );
    query( "CREATE INDEX percentage_stats ON statistics( percentage );" );
    query( "CREATE INDEX playcounter_stats ON statistics( playcounter );" );
}


void
CollectionDB::dropStatsTable()
{
    kdDebug() << k_funcinfo << endl;

    query( "DROP TABLE statistics;" );
}


void
CollectionDB::purgeDirCache()
{
    query( "DELETE FROM directories;" );
}


void
CollectionDB::scan( const QStringList& folders, bool recursively, bool importPlaylists )
{
    kdDebug() << k_funcinfo << endl;

    emit s_emitter->scanStarted();
    if ( !folders.isEmpty() )
        m_weaver->append( new CollectionReader( this, PlaylistBrowser::instance(), folders,
                                                recursively, importPlaylists, false ) );
    else
        emit s_emitter->scanDone( false );
}


void
CollectionDB::updateTags( const QString &url, const MetaBundle &bundle, bool updateCB )
{
    QString command = "UPDATE tags SET ";
    command += "title = '" + escapeString( bundle.title() ) + "', ";
    command += "artist = " + QString::number( artistID( bundle.artist(), true ) ) + ", ";
    command += "album = "  + QString::number(  albumID( bundle.album(), true ) ) + ", ";
    command += "genre = "  + QString::number(  genreID( bundle.genre(), true ) ) + ", ";
    command += "year = "   + QString::number(   yearID( bundle.year(), true ) ) + ", ";
    if ( !bundle.track().isEmpty() )
        command += "track = " + bundle.track() + ", ";
    command += "comment = '" + escapeString( bundle.comment() ) + "' ";
    command += "WHERE url = '" + escapeString( url ) + "';";

    query( command );

    if ( updateCB )    //update the collection browser
        CollectionView::instance()->renderView();

    emit s_emitter->metaDataEdited( bundle );
}


void
CollectionDB::updateTag( const QString &url, const QString &field, const QString &newTag )
{
    QStringList idFields;
    idFields << "artist" << "album" << "genre" << "year";

    QString command = "UPDATE tags "
                      "SET " + field + " = ";

    if( idFields.contains( field ) )
        command += escapeString( QString::number( IDFromValue( field, newTag, true ) ) ) + " ";
    else
        command += "'" + escapeString( newTag ) + "' ";

    command += "WHERE url = '" + escapeString(url) + "';";

    query( command );

    CollectionView::instance()->renderView();

}


void
CollectionDB::scanModifiedDirs( bool recursively, bool importPlaylists )
{
    emit s_emitter->scanStarted();

    QStringList folders;
    struct stat statBuf;

    query( "SELECT dir, changedate FROM directories;" );

    for ( uint i = 0; i < m_values.count(); i = i + 2 )
    {
        if ( stat( m_values[i].local8Bit(), &statBuf ) == 0 )
        {
            if ( QString::number( (long)statBuf.st_mtime ) != m_values[i + 1] )
            {
                folders << m_values[i];
                kdDebug() << "Collection dir changed: " << m_values[i] << endl;
            }
        }
        else
        {
            // this folder has been removed
            folders << m_values[i];
            kdDebug() << "Collection dir removed: " << m_values[i] << endl;
        }
    }

    if ( !folders.isEmpty() )
        m_weaver->append( new CollectionReader( this, PlaylistBrowser::instance(), folders,
                                                recursively, importPlaylists, true ) );
    else
        emit s_emitter->scanDone( false );
}


uint
CollectionDB::artistID( QString value, bool autocreate, bool useTempTables )
{
    // lookup cache
    if ( m_cacheArtist == value )
        return m_cacheArtistID;

    uint id = IDFromValue( "artist", value, autocreate, useTempTables );

    // cache values
    m_cacheArtist = value;
    m_cacheArtistID = id;

    return id;
}


QString
CollectionDB::artistValue( uint id )
{
    // lookup cache
    if ( m_cacheArtistID == id )
        return m_cacheArtist;

    QString value = valueFromID( "artist", id );

    // cache values
    m_cacheArtist = value;
    m_cacheArtistID = id;

    return value;
}



uint
CollectionDB::albumID( QString value, bool autocreate, bool useTempTables )
{
    // lookup cache
    if ( m_cacheAlbum == value )
        return m_cacheAlbumID;

    uint id = IDFromValue( "album", value, autocreate, useTempTables );

    // cache values
    m_cacheAlbum = value;
    m_cacheAlbum = id;

    return id;
}


QString
CollectionDB::albumValue( uint id )
{
    // lookup cache
    if ( m_cacheAlbumID == id )
        return m_cacheAlbum;

    QString value = valueFromID( "album", id );

    // cache values
    m_cacheAlbum = value;
    m_cacheAlbumID = id;

    return value;
}


uint
CollectionDB::genreID( QString value, bool autocreate, bool useTempTables )
{
    return IDFromValue( "genre", value, autocreate, useTempTables );
}


QString
CollectionDB::genreValue( uint id )
{
    return valueFromID( "genre", id );
}


uint
CollectionDB::yearID( QString value, bool autocreate, bool useTempTables )
{
    return IDFromValue( "year", value, autocreate, useTempTables );
}


QString
CollectionDB::yearValue( uint id )
{
    return valueFromID( "year", id );
}


uint
CollectionDB::IDFromValue( QString name, QString value, bool autocreate, bool useTempTables )
{
    if ( useTempTables )
        name.append( "_temp" );

    query( QString( "SELECT id FROM %1 WHERE name LIKE '%2';" )
                    .arg( name )
                    .arg( escapeString( value ) ) );

    uint id;
    //check if item exists. if not, should we autocreate it?
    if ( m_values.isEmpty() && autocreate )
    {
        query( QString( "INSERT INTO %1 ( name ) VALUES ( '%2' );" )
                        .arg( name )
                        .arg( escapeString( value ) ) );

        id = sqlInsertID();

        return id;
    }

    return m_values.isEmpty() ? 0 : m_values.first().toUInt();
}


QString
CollectionDB::valueFromID( QString table, uint id )
{
    query( QString( "SELECT name FROM %1 WHERE id=%2;" )
                    .arg( table )
                    .arg( id ) );


    return m_values.isEmpty() ? 0 : m_values.first();
}


void
CollectionDB::retrieveFirstLevel( QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    // apply special user-filter
    if ( !filter.isEmpty() )
    {
        filter = escapeString( filter );
        filterToken = "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        if ( !category2.isEmpty() ) {
            filterToken.prepend( "AND tags." + category2.lower() + "=" + category2.lower() + ".id " );
            filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
            if ( !category3.isEmpty() ) {
                filterToken.prepend( "AND tags." + category3.lower() + "=" + category3.lower() + ".id " );
                filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
            }
        }
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    QString command = "SELECT DISTINCT " + category1.lower() + ".name";
    command += category1.lower() == "artist" ? ", tags.sampler " : ", '0' ";

    command += "FROM tags, " + category1.lower();
    if( !filter.isEmpty() ) {
        if ( !category2.isEmpty() ) {
            command += ", " + category2.lower();
            if( !category3.isEmpty() )
                command += ", " + category3.lower();
        }
    }

    command += " WHERE " + category1.lower() + ".id=tags." + category1.lower();
    command += " " + filterToken;
    command += " ORDER BY lower(" + category1.lower() + ".name) DESC;";

    *values = query( command, names );
}


void
CollectionDB::retrieveSecondLevel( QString itemText, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    // apply special user-filter
    if ( !filter.isEmpty() )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        if ( !category2.isEmpty() ) {
            filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
            if ( !category3.isEmpty() ) {
                filterToken.prepend( "AND tags." + category3.lower() + "=" + category3.lower() + ".id " );
                filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
            }
        }
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    QString command;
    if ( category2.isEmpty() )
    {
        QString sorting = category1.lower() == "album" ? "track" : "title";

        command = "SELECT DISTINCT tags.title, tags.url FROM tags";
        if( !filter.isEmpty() )
            command += ", " + category1.lower();

        command += " WHERE tags.";

        if ( itemText == i18n( "Various Artists" ) )
            command += "sampler = 1";
        else {
            QString id = QString::number( IDFromValue( category1.lower(), itemText, false ) );
            command += category1.lower() + "=" + id;
        }
        command += " " + filterToken + " ORDER BY tags." + sorting + " DESC;";
    }
    else
    {
        command = "SELECT DISTINCT " + category2.lower() + ".name, '0' ";
        command += "FROM tags, " + category2.lower();
        if( !filter.isEmpty() ) {
            command += ", " + category1.lower();
            if( !category3.isEmpty() )
                command += ", " + category3.lower();
        }
        command += " WHERE tags." + category2.lower() + "=" + category2.lower() + ".id AND tags.";

        if ( itemText == i18n( "Various Artists" ) )
            command += "sampler = 1";
        else {
            QString id = QString::number( IDFromValue( category1.lower(), itemText, false ) );
            command += category1.lower() + "=" + id;
        }
        command += " " + filterToken + " ORDER BY lower(" + category2.lower() + ".name) DESC;";
    }

    *values = query( command, names );
}


void
CollectionDB::retrieveThirdLevel( QString itemText1, QString itemText2, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    // apply special user-filter
    if ( !filter.isEmpty() )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND tags." + category2.lower() + "=" + category2.lower() + ".id ";
        filterToken += "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
        if ( !category3.isEmpty() )
            filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    QString command;
    if( category3.isEmpty() ) {
        QString sorting = category2.lower() == "album" ? "track" : "title";

        QString id_sub = QString::number( IDFromValue( category2.lower(), itemText2, false ) );
        command = "SELECT DISTINCT tags.title, tags.url FROM tags";
        if( !filter.isEmpty() )
            command += ", " + category1.lower() + ", " + category2.lower();

        command += " WHERE tags." + category2.lower() + " = " + id_sub
                +  " AND tags.";

        if ( itemText1 == i18n( "Various Artists" ) )
            command += "sampler = 1 ";
        else {
            QString id = QString::number( IDFromValue( category1.lower(), itemText1, false ) );
            command += category1.lower() + "=" + id;
        }

        command += " " + filterToken + " ORDER BY tags." + sorting + " DESC;";
    }
    else {

        QString sub_id = QString::number( IDFromValue( category2.lower(), itemText2, false ) );
        command = "SELECT DISTINCT " + category3.lower() + ".name, '0' FROM tags, " + category3.lower();
        if( !filter.isEmpty() )
            command += ", " + category1.lower() + ", " + category2.lower();
        command += " WHERE tags." + category3.lower() + "=" + category3.lower() + ".id AND ";
        command += "tags." + category2.lower() + "=" + sub_id + " AND tags.";

        if ( itemText1 == i18n( "Various Artists" ) )
            command += "sampler = 1";
        else {
            QString id = QString::number( IDFromValue( category1.lower(), itemText1, false ) );
            command += category1.lower() + "=" + id;
        }

        command += " " + filterToken + " ORDER BY lower(" + category3.lower() + ".name) DESC;";
    }

    *values = query( command, names );
}


void
CollectionDB::retrieveFourthLevel( QString itemText1, QString itemText2, QString itemText3, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    // apply special user-filter
    if ( !filter.isEmpty() )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND tags." + category2.lower() + "=" + category2.lower() + ".id ";
        filterToken += "AND tags." + category3.lower() + "=" + category3.lower() + ".id ";
        filterToken += "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    QString sorting = category3.lower() == "album" ? "track" : "title";

    QString id_sub = QString::number( IDFromValue( category2.lower(), itemText2, false ) );
    QString id_sub2 = QString::number( IDFromValue( category3.lower(), itemText3, false ) );

    QString command = "SELECT DISTINCT tags.title, tags.url FROM tags";
    if( !filter.isEmpty() )
        command += ", " + category1.lower() + ", " + category2.lower() + ", " + category3.lower();

    command += " WHERE tags." + category3.lower() + " = " + id_sub2
                +  " AND tags." + category2.lower() + " = " + id_sub
                +  " AND tags.";

    if ( itemText1 == i18n( "Various Artists" ) )
        command += "sampler = 1 ";
    else {
        QString id = QString::number( IDFromValue( category1.lower(), itemText1, false ) );
        command += category1.lower() + "=" + id;
    }

    command += " " + filterToken + " ORDER BY tags." + sorting + " DESC;";
    *values = query( command, names );
}


void
CollectionDB::retrieveFirstLevelURLs( QString itemText, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    if ( !filter.isEmpty() )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        if ( !category2.isEmpty() ) {
            filterToken.prepend( "AND tags." + category2.lower() + "=" + category2.lower() + ".id " );
            filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
            if ( !category3.isEmpty() ) {
                filterToken.prepend( "AND tags." + category3.lower() + "=" + category3.lower() + ".id " );
                filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
            }
        }
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    //query database for all tracks in our category
    QString command = "SELECT DISTINCT tags.url FROM tags";
    if( !filter.isEmpty() ) {
        command += ", " + category1.lower();
        if ( !category2.isEmpty() ) {
            command += ", " + category2.lower();
            if( !category3.isEmpty() )
                command += ", " + category3.lower();
        }
    }

    command += " WHERE tags.";

    if ( itemText == i18n( "Various Artists" ) )
        command += "sampler = 1 ";
    else {
        QString id = QString::number( IDFromValue( category1.lower(), itemText, false ) );
        command += category1.lower() + "=" + id;
    }

    command += " " + filterToken;
    QString sorting = category1.lower() == "album" ? "track" : "title";
    command += " ORDER BY tags." + sorting;

    *values = query( command, names );
}


void
CollectionDB::retrieveSecondLevelURLs( QString itemText1, QString itemText2, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( !filter.isEmpty() )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND tags." + category2.lower() + "=" + category2.lower() + ".id ";
        filterToken += "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
        if ( !category3.isEmpty() ) {
            filterToken.prepend( "AND tags." + category3.lower() + "=" + category3.lower() + ".id " );
            filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
        }
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }


    QString id_sub = QString::number( IDFromValue( category2.lower(), itemText2, false ) );

    QString command = "SELECT DISTINCT tags.url FROM tags";
    if( !filter.isEmpty() ) {
        command += ", " + category1.lower() + ", " + category2.lower();
        if( !category3.isEmpty() )
            command += ", " + category3.lower();
    }

    command += " WHERE  tags." + category2.lower() + "=" + id_sub + " AND tags.";

    if ( itemText1 == i18n( "Various Artists" ) )
        command += "sampler = 1 ";
    else {
        QString id = QString::number( IDFromValue( category1.lower(), itemText1, false ) );
        command += category1.lower() + "=" + id;
    }

    command += " " + filterToken;
    QString sorting = category2.lower() == "album" ? "track" : "title";
    command += " ORDER BY tags." + sorting;

    *values = query( command, names );
}


void
CollectionDB::retrieveThirdLevelURLs( QString itemText1, QString itemText2, QString itemText3, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( !filter.isEmpty() )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND tags." + category2.lower() + "=" + category2.lower() + ".id ";
        filterToken += "AND tags." + category3.lower() + "=" + category3.lower() + ".id ";
        filterToken += "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }


    QString id_sub = QString::number( IDFromValue( category2.lower(), itemText2, false ) );
    QString id_sub2 = QString::number( IDFromValue( category3.lower(), itemText3, false ) );

    QString command = "SELECT DISTINCT tags.url FROM tags";
    if( !filter.isEmpty() )
        command += ", " + category1.lower() + ", " + category2.lower() + ", " + category3.lower();

    command += " WHERE tags." + category3.lower() + "=" + id_sub2 + " AND"
             + " tags." + category2.lower() + "=" + id_sub + " AND tags.";

    if ( itemText1 == i18n( "Various Artists" ) )
        command += "sampler = 1 ";
    else {
        QString id = QString::number( IDFromValue( category1.lower(), itemText1, false ) );
        command += category1.lower() + "=" + id;
    }

    command += " " + filterToken;
    QString sorting = category3.lower() == "album" ? "track" : "title";
    command += " ORDER BY tags." + sorting;

    *values = query( command, names );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::fetchCover( QObject* parent, const QString& artist, const QString& album, bool noedit ) //SLOT
{
    #ifdef AMAZON_SUPPORT
    /* Static license Key. Thanks muesli ;-) */
    QString amazonLicense = "D1URM11J3F2CEH";
    kdDebug() << "Querying amazon with artist: " << artist << " and album " << album << endl;
    QString keyword = artist + " - " + album;

    CoverFetcher* fetcher = new CoverFetcher( amazonLicense, parent );
    connect( fetcher, SIGNAL( imageReady( const QString&, const QString&, const QImage& ) ),
             this,      SLOT( saveCover( const QString&, const QString&, const QImage& ) ) );
    connect( fetcher, SIGNAL( error() ), this, SLOT( fetcherError() ) );

    fetcher->getCover( artist, album, keyword, CoverFetcher::heavy, noedit, 2, false );

    #endif
}


void
CollectionDB::stopScan() //SLOT
{
    CollectionReader::stop();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::dirDirty( const QString& path )
{
    kdDebug() << k_funcinfo << "Dirty: " << path << endl;

    m_weaver->append( new CollectionReader( this, PlaylistBrowser::instance(), path, false, false, true ) );
}


void
CollectionDB::saveCover( const QString& keyword, const QString& amazonUrl, const QImage& img )
{
    kdDebug() << k_funcinfo << endl;

    QStringList values = QStringList::split( " - ", keyword );
    setAlbumImage( values[ 0 ], values[ 1 ], img, amazonUrl );

    emit s_emitter->coverFetched( keyword );
    emit s_emitter->coverFetched();
}


void
CollectionDB::fetcherError()
{
    //this is called when there is an error with a coverfetcher
    emit s_emitter->coverFetcherError();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::customEvent( QCustomEvent *e )
{
    if ( e->type() == (QEvent::Type) ThreadWeaver::Job::CollectionReader )
    {
        kdDebug() << k_funcinfo << endl;
        emit s_emitter->scanDone( true );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionEmitter
//////////////////////////////////////////////////////////////////////////////////////////

CollectionEmitter::CollectionEmitter()
{
    EngineController::instance()->attach( this );
}


void CollectionEmitter::engineTrackEnded( int finalPosition, int trackLength )
{
    //This is where percentages are calculated
    //TODO statistics are not calculated when currentTrack doesn't exist

    const KURL &url = EngineController::instance()->bundle().url();
    if ( url.path().isEmpty() ) return;

    // sanity check
    if ( finalPosition > trackLength || finalPosition == 0 )
        finalPosition = trackLength;

    int pct = (int) ( ( (double) finalPosition / (double) trackLength ) * 100 );

    // increase song counter & calculate new statistics
    CollectionDB().addSongPercentage( url.path(), pct );
}


#include "collectiondb.moc"
