// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "config.h"

#include "collectionbrowser.h"    //updateTags()
#include "collectiondb.h"
#include "coverfetcher.h"
#include "metabundle.h"    //updateTags()
#include "playlistbrowser.h"
#include "threadweaver.h"

#include <qfile.h>
#include <qimage.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kinputdialog.h>   //setupCoverFetcher()
#include <klineedit.h>       //setupCoverFetcher()
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kio/job.h>

#include <unistd.h>

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionDB
//////////////////////////////////////////////////////////////////////////////////////////

CollectionDB::CollectionDB()
        : m_weaver( new ThreadWeaver( this ) )
        , m_cacheDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) )
        , m_coverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) )
{
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

    // create cover dir, if it doesn't exist.
    if( !m_coverDir.exists( "albumcovers", false ) )
        m_coverDir.mkdir( "albumcovers", false );
    m_coverDir.cd( "albumcovers" );

     // create image cache dir, if it doesn't exist.
    if( !m_cacheDir.exists( "albumcovers/cache", false ) )
        m_cacheDir.mkdir( "albumcovers/cache", false );
    m_cacheDir.cd( "albumcovers/cache" );
}


CollectionDB::~CollectionDB()
{
    sqlite3_close( m_db );
}


QString
CollectionDB::escapeString( QString string )
{
    string.replace( "'", "''" );
    return string;
}


bool
CollectionDB::isDbValid()
{
    if ( ( !execSql( "SELECT COUNT( url ) FROM tags LIMIT 0, 1;" ) ) ||
         ( !execSql( "SELECT COUNT( url ) FROM statistics LIMIT 0, 1;" ) ) )
        return false;
    else
        return true;
}


bool
CollectionDB::isEmpty()
{
    QStringList values;

    if ( execSql( "SELECT COUNT( url ) FROM tags LIMIT 0, 1;", &values ) )
        return ( values[0] == "0" );
    else
        return true;
}


QString
CollectionDB::albumSongCount( const QString artist_id, const QString album_id )
{
    QStringList values;

    execSql( QString( "SELECT COUNT( url ) FROM tags WHERE album = %1 AND artist = %2;" )
             .arg( album_id )
             .arg( artist_id ), &values );

    return values[0];
}


void
CollectionDB::addImageToPath( const QString path, const QString image, bool temporary )
{
    execSql( QString( "INSERT INTO images%1 ( path, name ) VALUES ( '%1', '%2' );" )
             .arg( temporary ? "_temp" : "" )
             .arg( escapeString( path ) )
             .arg( escapeString( image ) ) );
}


QString
CollectionDB::getPathForAlbum( const QString artist, const QString album )
{
    QStringList values;

    execSql( QString( "SELECT tags.url FROM tags, album, artist WHERE tags.album = album.id AND album.name = '%1' AND tags.artist = artist.id AND artist.name = '%2' LIMIT 0, 1;" )
                   .arg( escapeString( album ) )
                   .arg( escapeString( artist ) ), &values );

    return values[0];
}


QString
CollectionDB::getPathForAlbum( const uint artist_id, const uint album_id )
{
    QStringList values;

    execSql( QString( "SELECT url FROM tags WHERE album = %1 AND artist = %2 LIMIT 0, 1;" )
             .arg( album_id )
             .arg( artist_id ), &values );

    return values[0];
}


bool
CollectionDB::setImageForAlbum( const QString& artist, const QString& album, const QString& url, QImage img )
{
    QDir largeCoverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/albumcovers/large/" ) );
    QString fileName( artist + " - " + album );
    fileName = fileName.lower().replace( " ", "_" ).replace( "?", "" ).replace( "/", "_" ) + ".png";

    if( largeCoverDir.exists( fileName ) ) {
        //remove cache images
        QStringList scaledList = m_cacheDir.entryList( "*@" + fileName );
        for ( uint i = 0; i < scaledList.count(); i++ )
                m_cacheDir.remove( scaledList[i] );
    }

    img.setText( "amazon-url", 0, url );
    return img.save( largeCoverDir.filePath( fileName ), "PNG");
}


QString
CollectionDB::getImageForAlbum( const QString artist, const QString album, const uint width )
{
    QString widthKey = QString::number( width ) + "@";

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
        QString key( QFile::encodeName( artist.lower() + " - " + album.lower() ) );
        key.replace( " ", "_" ).replace( "?", "" ).replace( "/", "_" ).append( ".png" );

        if ( m_cacheDir.exists( widthKey + key ) )
            return m_cacheDir.filePath( widthKey + key );
        else
        {
            QDir largeCoverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/albumcovers/large/" ) );
            if ( largeCoverDir.exists( key ) )
                if ( width > 0 )
                {
                    QImage img( largeCoverDir.filePath( key ) );
                    img.smoothScale( width, width ).save( m_cacheDir.filePath( widthKey + key ), "PNG" );

                    return m_cacheDir.filePath( widthKey + key );
                }
                else
                    return largeCoverDir.filePath( key );
        }

        KURL url;
        url.setPath( getPathForAlbum( artist, album ) );

        return getImageForPath( url.directory(), width );
    }
}

QString
CollectionDB::getImageForAlbum( const uint artist_id, const uint album_id, const uint width )
{
    QStringList values;
    execSql( QString( "SELECT DISTINCT artist.name, album.name FROM artist, album "
                      "WHERE artist.id = %1 AND album.id = %2;" )
                      .arg( artist_id ).arg( album_id ), &values );

    return getImageForAlbum( values[0], values[1], width );
}


QString
CollectionDB::getImageForPath( const QString path, const uint width )
{
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

    QStringList values;
    QStringList names;
    KURL file( path );

    QString filename( QString::number( width ) + "@" + file.fileName() );
    filename.replace( "'", "_" ).append( ".png" );

#ifdef AMAZON_SUPPORT
    if ( m_cacheDir.exists( filename.lower() ) )
        return m_cacheDir.absPath() + "/" + filename.lower();
#endif
    execSql( QString( "SELECT name FROM images WHERE path = '%1';" )
             .arg( escapeString( path ) ), &values, &names );

    if ( values.count() )
    {
        QString image( values[0] );
        for ( uint i = 0; i < values.count(); i++ )
            if ( values[i].contains( "front", false ) )
                image = values[i];

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
CollectionDB::removeImageFromAlbum( const uint artist_id, const uint album_id )
{
    QStringList values;

    execSql( QString( "SELECT url FROM tags WHERE album = %1 AND artist = %2;" )
             .arg( album_id )
             .arg( artist_id ), &values );

    if ( !values.isEmpty() )
        return removeImageFromAlbum( values[0], values[1] );
    else
        return false;
}


bool
CollectionDB::removeImageFromAlbum( const QString artist, const QString album )
{
    QString widthKey = "*@";
    QString key( QFile::encodeName( artist + " - " + album ) );
    key.replace( " ", "_" ).replace( "?", "" ).replace( "/", "_" ).append( ".png" );

    // remove scaled versions of images
    QStringList scaledList = m_cacheDir.entryList( widthKey + key.lower() );
    if ( scaledList.count() > 0 )
        for ( uint i = 0; i < scaledList.count(); i++ )
            QFile::remove( m_cacheDir.filePath( scaledList[ i ] ) );

    // remove large, original image
    QDir largeCoverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/albumcovers/large/" ) );
    if ( largeCoverDir.exists( key.lower() ) )
        return QFile::remove( largeCoverDir.filePath( key.lower() ) );

    return false;
}


QStringList
CollectionDB::artistList( bool withUnknown, bool withCompilations )
{
    QStringList values;

    if ( withUnknown && withCompilations )
        execSql( "SELECT DISTINCT name FROM artist "
                 "ORDER BY lower( name );", &values );
    else
        execSql( "SELECT DISTINCT artist.name FROM tags, artist WHERE 1 " +
               ( withUnknown ? QString() : "AND artist.name <> 'Unknown' " ) +
               ( withCompilations ? QString() : "AND tags.artist = artist.id AND tags.sampler = 0 " ) +
                 "ORDER BY lower( artist.name );", &values );

    return values;
}


QStringList
CollectionDB::albumList( bool withUnknown, bool withCompilations )
{
    QStringList values;

    if ( withUnknown && withCompilations )
        execSql( "SELECT DISTINCT name FROM album "
                 "ORDER BY lower( name );", &values );
    else
        execSql( "SELECT DISTINCT album.name FROM tags, album WHERE 1 " +
               ( withUnknown ? QString() : "AND album.name <> 'Unknown' " ) +
               ( withCompilations ? QString() : "AND tags.album = album.id AND tags.sampler = 0 " ) +
                 "ORDER BY lower( album.name );", &values );

    return values;
}


QStringList
CollectionDB::albumListOfArtist( const QString artist, bool withUnknown, bool withCompilations )
{
    QStringList values;

    execSql( "SELECT DISTINCT album.name FROM tags, album, artist WHERE "
             "tags.album = album.id AND tags.artist = artist.id "
             "AND artist.name = '" + escapeString( artist ) + "' " +
             ( withUnknown ? QString() : "AND album.name <> 'Unknown' " ) +
             ( withCompilations ? QString() : "AND tags.sampler = 0 " ) +
             "ORDER BY lower( album.name );", &values );

    return values;
}


QStringList
CollectionDB::artistAlbumList( bool withUnknown, bool withCompilations )
{
    QStringList values;

    execSql( "SELECT DISTINCT artist.name, album.name FROM tags, album, artist WHERE "
             "tags.album = album.id AND tags.artist = artist.id " +
             ( withUnknown ? QString() : "AND album.name <> 'Unknown' AND artist.name <> 'Unknown' " ) +
             ( withCompilations ? QString() : "AND tags.sampler = 0 " ) +
             "ORDER BY lower( album.name );", &values );

    return values;
}


bool
CollectionDB::getMetaBundleForUrl( const QString url, MetaBundle *bundle )
{
    QStringList values;

    execSql( QString( "SELECT album.name, artist.name, genre.name, tags.title, year.name, tags.comment, tags.track, tags.createdate, tags.dir "
                      "FROM tags, album, artist, genre, year "
                      "WHERE album.id = tags.album AND artist.id = tags.artist AND genre.id = tags.genre AND year.id = tags.year AND url = '%1';" )
                .arg( escapeString( url ) ), &values );

    if ( values.count() )
    {
        bundle->m_album = values[0];
        bundle->m_artist = values[1];
        bundle->m_genre = values[2];
        bundle->m_title = values[3];
        bundle->m_year = values[4];
        bundle->m_comment = values[5];
        bundle->m_track = values[6];
        bundle->m_url = url;

        return true;
    }

    return false;
}


uint
CollectionDB::addSongPercentage( const QString url, const int percentage )
{
    QStringList values, names;

    execSql( QString( "SELECT playcounter, createdate, percentage FROM statistics WHERE url = '%1';" )
                .arg( escapeString( url ) ), &values, &names );

    if ( values.count() )
    {
        // entry exists, increment playcounter and update accesstime
        float score = ( ( values[2].toDouble() * values[0].toInt() ) + percentage ) / ( values[0].toInt() + 1 );

        execSql( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                          "VALUES ( '%1', '%2', strftime('%s', 'now'), %3, %4 );" )
                    .arg( escapeString( url ) )
                    .arg( values[1] )
                    .arg( score )
                    .arg( values[0] + " + 1" ) );
        return (uint)score;
    } else
    {
        // entry didnt exist yet, create a new one
        execSql( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                          "VALUES ( '%1', strftime('%s', 'now'), strftime('%s', 'now'), %2, 1 );" )
                    .arg( escapeString( url ) )
                    .arg( (float)( ( 50 + percentage ) / 2 ) ) );
        return percentage;
    }
}


uint
CollectionDB::getSongPercentage( const QString url )
{
    QStringList values;

    execSql( QString( "SELECT round( percentage + 0.5 ) FROM statistics WHERE url = '%1';" )
                .arg( escapeString( url ) ), &values );

    if( values.count() )
        return values[0].toInt();

    return 0;
}


void
CollectionDB::updateDirStats( QString path, const long datetime )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    execSql( QString( "REPLACE INTO directories ( dir, changedate ) VALUES ( '%1', %2 );" )
                .arg( escapeString( path ) )
                .arg( datetime ) );
}


void
CollectionDB::removeSongsInDir( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    execSql( QString( "DELETE FROM tags WHERE dir = '%1';" )
                .arg( escapeString( path ) ) );
}


bool
CollectionDB::isDirInCollection( QString path )
{
    QStringList values;

    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    execSql( QString( "SELECT changedate FROM directories WHERE dir = '%1';" )
                .arg( escapeString( path ) ), &values );

    return !values.isEmpty();
}


bool
CollectionDB::isFileInCollection( const QString url )
{
    QStringList values;

    execSql( QString( "SELECT url FROM tags WHERE url = '%1';" )
                .arg( escapeString( url ) ), &values );

    return !values.isEmpty();
}


bool
CollectionDB::isSamplerAlbum( const QString album )
{
    QStringList values_artist;
    QStringList values_dir;
    QStringList names_artist;
    QStringList names_dir;

    if ( album == "Unknown" || album == "" )
        return false;

    const uint album_id = getValueID( "album", album, FALSE, FALSE );
    execSql( QString( "SELECT DISTINCT artist.name FROM artist, tags WHERE tags.artist = artist.id AND tags.album = '%1';" )
                .arg( album_id ), &values_artist, &names_artist );
    execSql( QString( "SELECT DISTINCT dir FROM tags WHERE album = '%1';" )
                .arg( album_id ), &values_dir, &names_dir );

    if ( values_artist.count() > values_dir.count() )
    {

        execSql( QString( "UPDATE tags SET sampler = 1 WHERE album = '%1';" )
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

    execSql( QString( "DELETE FROM directories WHERE dir = '%1';" )
                .arg( escapeString( path ) ) );
}

#include <time.h>
bool
CollectionDB::execSql( const QString& statement, QStringList* const values, QStringList* const names, const bool debug )
{
    if ( debug )
        kdDebug() << "query-start: " << statement << endl;

    clock_t start = clock();

    if ( !m_db )
    {
        kdError() << k_funcinfo << "[CollectionDB] SQLite pointer == NULL.\n";
        return false;
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

        return false;
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
            if ( values ) *values << QString::fromUtf8( (const char*) sqlite3_column_text( stmt, i ) );
            if ( names )  *names  << QString( sqlite3_column_name( stmt, i ) );
        }
    }
    //deallocate vm ressources
    sqlite3_finalize( stmt );

    if ( error != SQLITE_DONE )
    {
        kdError() << k_funcinfo << "sqlite_step error.\n";
        kdError() << sqlite3_errmsg( m_db ) << endl;
        kdError() << "on query: " << statement << endl;

        return false;
    }

    if ( debug )
    {
        clock_t finish = clock();
        const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
        kdDebug() << "[CollectionDB] SQL-query (" << duration << "s): " << statement << endl;
    }

    return true;
}


int
CollectionDB::sqlInsertID()
{
    if ( !m_db )
    {
        kdWarning() << k_funcinfo << "SQLite pointer == NULL.\n";
        return -1;
    }

    return sqlite3_last_insert_rowid( m_db );
}


void
CollectionDB::createTables( bool temporary )
{
    kdDebug() << k_funcinfo << endl;

    //create tag table
    execSql( QString( "CREATE %1 TABLE tags%2 ("
                        "url VARCHAR(100),"
                        "dir VARCHAR(100),"
                        "createdate INTEGER,"
                        "album INTEGER,"
                        "artist INTEGER,"
                        "genre INTEGER,"
                        "title VARCHAR(100),"
                        "year INTEGER,"
                        "comment VARCHAR(100),"
                        "track NUMBER(4),"
                        "sampler BOOLEAN );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create album table
    execSql( QString( "CREATE %1 TABLE album%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create artist table
    execSql( QString( "CREATE %1 TABLE artist%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create genre table
    execSql( QString( "CREATE %1 TABLE genre%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create year table
    execSql( QString( "CREATE %1 TABLE year%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create images table
    execSql( QString( "CREATE %1 TABLE images%2 ("
                        "path VARCHAR(100),"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create indexes
    execSql( QString( "CREATE INDEX album_idx%1 ON album%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "CREATE INDEX artist_idx%1 ON artist%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "CREATE INDEX genre_idx%1 ON genre%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "CREATE INDEX year_idx%1 ON year%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );

    if ( !temporary )
    {
        execSql( "CREATE INDEX album_tag ON tags( album );" );
        execSql( "CREATE INDEX artist_tag ON tags( artist );" );
        execSql( "CREATE INDEX genre_tag ON tags( genre );" );
        execSql( "CREATE INDEX year_tag ON tags( year );" );
        execSql( "CREATE INDEX sampler_tag ON tags( sampler );" );

        // create directory statistics database
        execSql( QString( "CREATE TABLE directories ("
                            "dir VARCHAR(100) UNIQUE,"
                            "changedate INTEGER );" ) );
    }
}


void
CollectionDB::dropTables( bool temporary )
{
    kdDebug() << k_funcinfo << endl;

    execSql( QString( "DROP TABLE tags%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE album%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE artist%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE genre%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE year%1;" ).arg( temporary ? "_temp" : "" ) );
    execSql( QString( "DROP TABLE images%1;" ).arg( temporary ? "_temp" : "" ) );
}


void
CollectionDB::moveTempTables()
{
    execSql( "INSERT INTO tags SELECT * FROM tags_temp;" );
    execSql( "INSERT INTO album SELECT * FROM album_temp;" );
    execSql( "INSERT INTO artist SELECT * FROM artist_temp;" );
    execSql( "INSERT INTO genre SELECT * FROM genre_temp;" );
    execSql( "INSERT INTO year SELECT * FROM year_temp;" );
    execSql( "INSERT INTO images SELECT * FROM images_temp;" );
}


void
CollectionDB::createStatsTable()
{
    kdDebug() << k_funcinfo << endl;

    // create music statistics database
    execSql( QString( "CREATE TABLE statistics ("
                      "url VARCHAR(100) UNIQUE,"
                      "createdate INTEGER,"
                      "accessdate INTEGER,"
                      "percentage FLOAT,"
                      "playcounter INTEGER );" ) );

    execSql( "CREATE INDEX url_stats ON statistics( url );" );
    execSql( "CREATE INDEX percentage_stats ON statistics( percentage );" );
    execSql( "CREATE INDEX playcounter_stats ON statistics( playcounter );" );
}


void
CollectionDB::dropStatsTable()
{
    kdDebug() << k_funcinfo << endl;

    execSql( "DROP TABLE statistics;" );
}


void
CollectionDB::purgeDirCache()
{
    execSql( "DELETE FROM directories;" );
}


void
CollectionDB::scan( const QStringList& folders, bool recursively, bool importPlaylists )
{
    kdDebug() << k_funcinfo << endl;

    if ( !folders.isEmpty() )
        m_weaver->append( new CollectionReader( this, PlaylistBrowser::instance(), folders,
                                                                              recursively, importPlaylists, false ) );
    else
        emit scanDone( false );
}


void
CollectionDB::updateTags( const QString &url, const MetaBundle &bundle )
{
    QString command = "UPDATE tags SET ";
    command += "title = '" + escapeString( bundle.title() ) + "', ";
    command += "artist = " + escapeString( QString::number( getValueID( "artist", bundle.artist(), true ) ) ) + ", ";
    command += "album = " + escapeString( QString::number( getValueID( "album", bundle.album(), true ) ) ) + ", ";
    command += "genre = " + escapeString( QString::number( getValueID( "genre", bundle.genre(), true ) ) ) + ", ";
    command += "year = " + escapeString( QString::number( getValueID( "year", bundle.year(), true ) ) ) + ", ";
    if( !bundle.track().isEmpty() )
        command += "track = " + escapeString( bundle.track() ) + ", ";
    command += "comment = '" + escapeString( bundle.comment() ) + "' ";
    command += "WHERE url = '" + escapeString( url ) + "';";

    execSql( command );

    CollectionView::instance()->renderView();

}


void
CollectionDB::updateTag( const QString &url, const QString &field, const QString &newTag )
{
    QStringList idFields;
    idFields << "artist" << "album" << "genre" << "year";

    QString command = "UPDATE tags "
                                   "SET " + field + " = ";

    if( idFields.contains( field ) )
        command += escapeString( QString::number( getValueID( field, newTag, true ) ) ) + " ";
    else
        command += "'" + escapeString( newTag ) + "' ";

    command += "WHERE url = '" + escapeString(url) + "';";

    execSql( command );

    CollectionView::instance()->renderView();

}


void
CollectionDB::scanModifiedDirs( bool recursively, bool importPlaylists )
{
    QStringList values;
    QStringList folders;
    struct stat statBuf;

    QString command = QString( "SELECT dir, changedate FROM directories;" );
    execSql( command, &values );

    for ( uint i = 0; i < values.count(); i = i + 2 )
    {
        if ( stat( values[i].local8Bit(), &statBuf ) == 0 )
        {
            if ( QString::number( (long)statBuf.st_mtime ) != values[i + 1] )
            {
                folders << values[i];
                kdDebug() << "Collection dir changed: " << values[i] << endl;
            }
        }
        else
        {
            // this folder has been removed
            folders << values[i];
            kdDebug() << "Collection dir removed: " << values[i] << endl;
        }
    }

    if ( !folders.isEmpty() )
        m_weaver->append( new CollectionReader( this, PlaylistBrowser::instance(), folders,
                                                                             recursively, importPlaylists, true ) );
    else
        emit scanDone( false );
}


uint
CollectionDB::getValueID( QString name, QString value, bool autocreate, bool useTempTables )
{
    QStringList values;
    QStringList names;

    if ( useTempTables )
        name.append( "_temp" );

    QString command = QString( "SELECT id FROM '%1' WHERE name LIKE '%2';" )
                      .arg( name )
                      .arg( escapeString( value ) );
    execSql( command, &values, &names );

    //check if item exists. if not, should we autocreate it?
    if ( values.isEmpty() && autocreate )
    {
        command = QString( "INSERT INTO '%1' ( name ) VALUES ( '%2' );" )
                  .arg( name )
                  .arg( escapeString( value ) );

        execSql( command );
        int id = sqlInsertID();
        return id;
    }

    if ( values.isEmpty() )
        return 0;

    return values[0].toUInt();
}


QString
CollectionDB::getValueFromID( QString table, uint id )
{
   QStringList values;

   execSql( QString( "SELECT name FROM %1 WHERE id=%2;" )
                                   .arg( table )
                                   .arg( id ), &values );

    return values[0];
}


void
CollectionDB::retrieveFirstLevel( QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    // apply special user-filter
    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        if ( category2 != 0 ) {
            filterToken.prepend( "AND tags." + category2.lower() + "=" + category2.lower() + ".id " );
            filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
            if ( category3 != 0 ) {
                filterToken.prepend( "AND tags." + category3.lower() + "=" + category3.lower() + ".id " );
                filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
            }
        }
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    QString command = "SELECT DISTINCT " + category1.lower() + ".name";
    command += category1.lower() == "artist" ? ", tags.sampler " : ", '0' ";

    command += "FROM tags, " + category1.lower();
    if( filter != "" ) {
        if ( category2 != 0 ) {
            command += ", " + category2.lower();
            if( category3 != 0 )
                command += ", " + category3.lower();
        }
    }

    command += " WHERE " + category1.lower() + ".id=tags." + category1.lower();
    command += " " + filterToken;
    command += " ORDER BY lower(" + category1.lower() + ".name) DESC;";

    execSql( command, values, names );
}


void
CollectionDB::retrieveSecondLevel( QString itemText, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    // apply special user-filter
    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        if ( category2 != 0 ) {
            filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
            if ( category3 != 0 ) {
                filterToken.prepend( "AND tags." + category3.lower() + "=" + category3.lower() + ".id " );
                filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
            }
        }
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    QString command;
    if ( category2 == 0 )
    {
        QString sorting = category1.lower() == "album" ? "track" : "title";

        command = "SELECT DISTINCT tags.title, tags.url FROM tags";
        if( filter != "" )
            command += ", " + category1.lower();

        command += " WHERE tags.";

        if ( itemText == i18n( "Various Artists" ) )
            command += "sampler = 1";
        else {
            QString id = QString::number( getValueID( category1.lower(), itemText, false ) );
            command += category1.lower() + "=" + id;
        }
        command += " " + filterToken + " ORDER BY tags." + sorting + " DESC;";
    }
    else
    {
        command = "SELECT DISTINCT " + category2.lower() + ".name, '0' ";
        command += "FROM tags, " + category2.lower();
        if( filter != "" ) {
            command += ", " + category1.lower();
            if( category3 != 0 )
                command += ", " + category3.lower();
        }
        command += " WHERE tags." + category2.lower() + "=" + category2.lower() + ".id AND tags.";

        if ( itemText == i18n( "Various Artists" ) )
            command += "sampler = 1";
        else {
            QString id = QString::number( getValueID( category1.lower(), itemText, false ) );
            command += category1.lower() + "=" + id;
        }
        command += " " + filterToken + " ORDER BY lower(" + category2.lower() + ".name) DESC;";
    }

    execSql( command, values, names );
}


void
CollectionDB::retrieveThirdLevel( QString itemText1, QString itemText2, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    // apply special user-filter
    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND tags." + category2.lower() + "=" + category2.lower() + ".id ";
        filterToken += "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
        if ( category3 != 0 )
            filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    QString command;
    if( category3 == 0 ) {
        QString sorting = category2.lower() == "album" ? "track" : "title";

        QString id_sub = QString::number( getValueID( category2.lower(), itemText2, false ) );
        command = "SELECT DISTINCT tags.title, tags.url FROM tags";
        if( filter != "" )
            command += ", " + category1.lower() + ", " + category2.lower();

        command += " WHERE tags." + category2.lower() + " = " + id_sub
                +  " AND tags.";

        if ( itemText1 == i18n( "Various Artists" ) )
            command += "sampler = 1 ";
        else {
            QString id = QString::number( getValueID( category1.lower(), itemText1, false ) );
            command += category1.lower() + "=" + id;
        }

        command += " " + filterToken + " ORDER BY tags." + sorting + " DESC;";
    }
    else {

        QString sub_id = QString::number( getValueID( category2.lower(), itemText2, false ) );
        command = "SELECT DISTINCT " + category3.lower() + ".name, '0' FROM tags, " + category3.lower();
        if( filter != "" )
            command += ", " + category1.lower() + ", " + category2.lower();
        command += " WHERE tags." + category3.lower() + "=" + category3.lower() + ".id AND ";
        command += "tags." + category2.lower() + "=" + sub_id + " AND tags.";

        if ( itemText1 == i18n( "Various Artists" ) )
            command += "sampler = 1";
        else {
            QString id = QString::number( getValueID( category1.lower(), itemText1, false ) );
            command += category1.lower() + "=" + id;
        }

        command += " " + filterToken + " ORDER BY lower(" + category3.lower() + ".name) DESC;";
    }

    execSql( command, values, names );
}


void
CollectionDB::retrieveFourthLevel( QString itemText1, QString itemText2, QString itemText3, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    // apply special user-filter
    if ( filter != "" )
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

    QString id_sub = QString::number( getValueID( category2.lower(), itemText2, false ) );
    QString id_sub2 = QString::number( getValueID( category3.lower(), itemText3, false ) );

    QString command = "SELECT DISTINCT tags.title, tags.url FROM tags";
    if( filter != "" )
        command += ", " + category1.lower() + ", " + category2.lower() + ", " + category3.lower();

    command += " WHERE tags." + category3.lower() + " = " + id_sub2
                +  " AND tags." + category2.lower() + " = " + id_sub
                +  " AND tags.";

    if ( itemText1 == i18n( "Various Artists" ) )
        command += "sampler = 1 ";
    else {
        QString id = QString::number( getValueID( category1.lower(), itemText1, false ) );
        command += category1.lower() + "=" + id;
    }

    command += " " + filterToken + " ORDER BY tags." + sorting + " DESC;";
    execSql( command, values, names );
}


void
CollectionDB::retrieveFirstLevelURLs( QString itemText, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;

    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        if ( category2 != 0 ) {
            filterToken.prepend( "AND tags." + category2.lower() + "=" + category2.lower() + ".id " );
            filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
            if ( category3 != 0 ) {
                filterToken.prepend( "AND tags." + category3.lower() + "=" + category3.lower() + ".id " );
                filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
            }
        }
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }

    //query database for all tracks in our category
    QString command = "SELECT DISTINCT tags.url FROM tags";
    if( filter != "" ) {
        command += ", " + category1.lower();
        if ( category2 != 0 ) {
            command += ", " + category2.lower();
            if( category3 != 0 )
                command += ", " + category3.lower();
        }
    }

    command += " WHERE tags.";

    if ( itemText == i18n( "Various Artists" ) )
        command += "sampler = 1 ";
    else {
        QString id = QString::number( getValueID( category1.lower(), itemText, false ) );
        command += category1.lower() + "=" + id;
    }

    command += " " + filterToken;
    if ( category2 != 0 )
        command += " ORDER BY tags." + category2.lower() + ", tags.track;";
    else
        command += " ORDER BY tags.album, tags.track;";

    execSql( command, values, names );
}


void
CollectionDB::retrieveSecondLevelURLs( QString itemText1, QString itemText2, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( filter != "" )
    {
        filter = escapeString( filter );
        filterToken = "AND " + category1.lower() + ".id=tags." + category1.lower();
        filterToken += " AND tags." + category2.lower() + "=" + category2.lower() + ".id ";
        filterToken += "AND ( " + category1.lower() + ".name LIKE '%" + filter + "%' ";
        filterToken += "OR " + category2.lower() + ".name LIKE '%" + filter + "%' ";
        if ( category3 != 0 ) {
            filterToken.prepend( "AND tags." + category3.lower() + "=" + category3.lower() + ".id " );
            filterToken += "OR " + category3.lower() + ".name LIKE '%" + filter + "%' ";
        }
        filterToken += "OR tags.title LIKE '%" + filter + "%' )";
    }


    QString id_sub = QString::number( getValueID( category2.lower(), itemText2, false ) );

    QString command = "SELECT DISTINCT tags.url FROM tags";
    if( filter != "" ) {
        command += ", " + category1.lower() + ", " + category2.lower();
        if( category3 != 0 )
            command += ", " + category3.lower();
    }

    command += " WHERE  tags." + category2.lower() + "=" + id_sub + " AND tags.";

    if ( itemText1 == i18n( "Various Artists" ) )
        command += "sampler = 1 ";
    else {
        QString id = QString::number( getValueID( category1.lower(), itemText1, false ) );
        command += category1.lower() + "=" + id;
    }

    command += " " + filterToken + " ORDER BY tags." + category2.lower() + ", tags.track;";

    execSql( command, values, names );
}


void
CollectionDB::retrieveThirdLevelURLs( QString itemText1, QString itemText2, QString itemText3, QString category1, QString category2, QString category3, QString filter, QStringList* const values, QStringList* const names )
{
    QString filterToken;
    if ( filter != "" )
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


    QString id_sub = QString::number( getValueID( category2.lower(), itemText2, false ) );
    QString id_sub2 = QString::number( getValueID( category3.lower(), itemText3, false ) );

    QString command = "SELECT DISTINCT tags.url FROM tags";
    if( filter != "" )
        command += ", " + category1.lower() + ", " + category2.lower() + ", " + category3.lower();

    command += " WHERE tags." + category3.lower() + "=" + id_sub2 + " AND"
             + " tags." + category2.lower() + "=" + id_sub + " AND tags.";

    if ( itemText1 == i18n( "Various Artists" ) )
        command += "sampler = 1 ";
    else {
        QString id = QString::number( getValueID( category1.lower(), itemText1, false ) );
        command += category1.lower() + "=" + id;
    }

    command += " " + filterToken + " ORDER BY tags." + category3.lower() + ", tags.track;";

    execSql( command, values, names );
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
    QString keyword = artist + " - " + album;
    kdDebug() << "Querying amazon with artist: " << artist << " and album " << album << endl;

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
CollectionDB::saveCover( const QString& keyword, const QString& url, const QImage& img )
{
    kdDebug() << k_funcinfo << endl;

    //get artist and album
    QStringList values = QStringList::split( " - ", keyword );
    setImageForAlbum( values[0], values[1], url, img );

    emit coverFetched( keyword );
    emit coverFetched();
}


void
CollectionDB::fetcherError()
{
    //this is called when there is an error with a coverfetcher
    emit coverFetcherError();
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
        emit scanDone( true );
    }
}


#include "collectiondb.moc"
