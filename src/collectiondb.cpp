// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// See COPYING file for licensing information.

#include "config.h"

#ifdef USE_MYSQL
#include "app.h"
#endif

#include "amarok.h"
#include "amarokconfig.h"
#include "collectionbrowser.h"    //updateTags()
#include "collectiondb.h"
#include "collectionreader.h"
#include "coverfetcher.h"
#include "enginecontroller.h"
#include "metabundle.h"           //updateTags()
#include "playlist.h"
#include "playlistbrowser.h"
#include "scrobbler.h"
#include "statusbar.h"
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
#include <unistd.h>               //usleep()

#include <mpegfile.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <attachedpictureframe.h>
#include <tbytevector.h>

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionDB
//////////////////////////////////////////////////////////////////////////////////////////

CollectionDB* CollectionDB::instance()
{
    static CollectionDB db;
    return &db;
}


CollectionDB::CollectionDB()
        : m_isScanning( false )
        , m_cacheDir( KGlobal::dirs()->saveLocation( "data", "amarok/" ) )
        , m_coverDir( KGlobal::dirs()->saveLocation( "data", "amarok/" ) )
{
    kdDebug() << k_funcinfo << endl;

    EngineController::instance()->attach( this );

    // create cover dir, if it doesn't exist.
    if( !m_coverDir.exists( "albumcovers", false ) )
        m_coverDir.mkdir( "albumcovers", false );
    m_coverDir.cd( "albumcovers" );

     // create image cache dir, if it doesn't exist.
    if( !m_cacheDir.exists( "albumcovers/cache", false ) )
        m_cacheDir.mkdir( "albumcovers/cache", false );
    m_cacheDir.cd( "albumcovers/cache" );

    //<OPEN DATABASE>

    DbConnection *dbConn = m_dbConnPool.getDbConnection();
    bool initialized = dbConn->isInitialized();
    m_dbConnPool.putDbConnection( dbConn );
    
    KConfig* config = amaroK::config( "Collection Browser" );
    if ( !initialized || !isValid() )
    {
        createTables();
        createStatsTable();
    }
    else
    {
        //remove database file if version is incompatible
        if ( config->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION )
        {
            kdDebug() << "Rebuilding database!" << endl;
            dropTables();
            createTables();
            startScan();
        }
        if ( config->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION )
        {
            kdDebug() << "Rebuilding stats-database!" << endl;
            dropStatsTable();
            createStatsTable();
        }
    }
        
    m_dbConnPool.createDbConnections();

#ifdef USE_MYSQL

#else
    //optimization for speeding up SQLite
    query( "PRAGMA default_synchronous = OFF;" );
#endif
    //</OPEN DATABASE>

    // TODO: Should write to config in dtor, but it crashes...
    config->writeEntry( "Database Version", DATABASE_VERSION );
    config->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );
    
    startTimer( MONITOR_INTERVAL * 1000 );

    connect( Scrobbler::instance(), SIGNAL( similarArtistsFetched( const QString&, const QStringList& ) ),
             this,                  SLOT( similarArtistsFetched( const QString&, const QStringList& ) ) );

    // This is used when the collection folders were changed in the first-run wizard
//     connect( kapp, SIGNAL( sigScanCollection() ), this, SLOT( startScan() ) );
}


CollectionDB::~CollectionDB()
{
    kdDebug() << k_funcinfo << endl;

//     This crashes so it's done at the end of ctor.
//     KConfig* const config = amaroK::config( "Collection Browser" );
//     config->writeEntry( "Database Version", DATABASE_VERSION );
//     config->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////


DbConnection
*CollectionDB::getStaticDbConnection()
{
    return m_dbConnPool.getDbConnection();
}


void
CollectionDB::returnStaticDbConnection( DbConnection *conn )
{
    m_dbConnPool.putDbConnection( conn );
}


/**
 * Executes a SQL query on the already opened database
 * @param statement SQL program to execute. Only one SQL statement is allowed.
 * @return          The queried data, or QStringList() on error.
 */
QStringList
CollectionDB::query( const QString& statement, DbConnection *conn )
{
    if ( DEBUG )
        kdDebug() << "query-start: " << statement << endl;

    QStringList values;

    clock_t start = clock();

    DbConnection *dbConn;
    if ( conn != NULL )
    {
        dbConn = conn;
    }
    else
    {
        dbConn = m_dbConnPool.getDbConnection();
    }

#ifdef USE_MYSQL
    if (!mysql::mysql_query(dbConn->db(), statement.utf8()))
    {
        mysql::MYSQL_RES* result;
        if ((result = mysql::mysql_use_result(dbConn->db())))
        {
            int number = mysql::mysql_field_count(dbConn->db());
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
            if (mysql::mysql_field_count(dbConn->db()) != 0)
            {
                kdDebug() << "MYSQL QUERY FAILED: " << mysql::mysql_error(dbConn->db()) << "\n" << "FAILED QUERY: " << statement << "\n";
                values = QStringList();
            }
        }
    }
    else
    {
        kdDebug() << "MYSQL QUERY FAILED: " << mysql::mysql_error(dbConn->db()) << "\n" << "FAILED QUERY: " << statement << "\n";
        values = QStringList();
    }
#else
    int error;
    const char* tail;
    sqlite3_stmt* stmt;

    //compile SQL program to virtual machine
    error = sqlite3_prepare( dbConn->db(), statement.utf8(), statement.length(), &stmt, &tail );

    if ( error != SQLITE_OK )
    {
        kdError() << k_funcinfo << "[CollectionDB] sqlite3_compile error:" << endl;
        kdError() << sqlite3_errmsg( dbConn->db() ) << endl;
        kdError() << "on query: " << statement << endl;
        values = QStringList();
    }
    else
    {
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
            }
        }
        //deallocate vm ressources
        sqlite3_finalize( stmt );

        if ( error != SQLITE_DONE )
        {
            kdError() << k_funcinfo << "sqlite_step error.\n";
            kdError() << sqlite3_errmsg( dbConn->db() ) << endl;
            kdError() << "on query: " << statement << endl;
            values = QStringList();
        }
    }
#endif

    if ( conn == NULL )
    {
        m_dbConnPool.putDbConnection( dbConn );
    }

    if ( DEBUG )
    {
        clock_t finish = clock();
        const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
        kdDebug() << "[CollectionDB] SQL-query (" << duration << "s): " << statement << endl;
    }
    return values;
}


/**
 * Returns the rowid of the most recently inserted row
 * @return          int rowid
 */
int
CollectionDB::sqlInsertID( DbConnection *conn )
{
    int id;
    DbConnection *dbConn;
    if ( conn != NULL )
    {
        dbConn = conn;
    }
    else
    {
        dbConn = m_dbConnPool.getDbConnection();
    }
#ifdef USE_MYSQL
    id = mysql::mysql_insert_id( dbConn->db() );
#else
    id = sqlite3_last_insert_rowid( dbConn->db() );
#endif
    if ( conn == NULL )
    {
        m_dbConnPool.putDbConnection( dbConn );
    }
    return id;
}


bool
CollectionDB::isEmpty()
{
    QStringList values = query( "SELECT COUNT( url ) FROM tags LIMIT 0, 1;" );

    return values.isEmpty() ? true : values.first() == "0";
}


bool
CollectionDB::isValid()
{
    QStringList values1 = query( "SELECT COUNT( url ) FROM tags LIMIT 0, 1;" );
    QStringList values2 = query( "SELECT COUNT( url ) FROM statistics LIMIT 0, 1;" );

    //TODO? this returns true if value1 or value2 is not empty. Shouldn't this be and (&&)???
    return !values1.isEmpty() || !values2.isEmpty();
}


#ifdef USE_MYSQL
#define AUTO_INCREMENT "AUTO_INCREMENT"
#else
#define AUTO_INCREMENT ""
#endif

void
CollectionDB::createTables( DbConnection *conn )
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
                    "track NUMERIC(4),"
                    "bitrate INTEGER,"
                    "length INTEGER,"
                    "samplerate INTEGER,"
                    "sampler BOOL );" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );

    //create album table
    query( QString( "CREATE %1 TABLE album%2 ("
                    "id INTEGER PRIMARY KEY " AUTO_INCREMENT ","
                    "name VARCHAR(255) );" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );

    //create artist table
    query( QString( "CREATE %1 TABLE artist%2 ("
                    "id INTEGER PRIMARY KEY " AUTO_INCREMENT ","
                    "name VARCHAR(255) );" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );

    //create genre table
    query( QString( "CREATE %1 TABLE genre%2 ("
                    "id INTEGER PRIMARY KEY " AUTO_INCREMENT ","
                    "name VARCHAR(255) );" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );

    //create year table
    query( QString( "CREATE %1 TABLE year%2 ("
                    "id INTEGER PRIMARY KEY " AUTO_INCREMENT ","
                    "name VARCHAR(4) );" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );

    //create images table
    query( QString( "CREATE %1 TABLE images%2 ("
                    "path VARCHAR(255),"
                    "artist VARCHAR(255),"
                    "album VARCHAR(255) );" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );

    // create directory statistics table
    query( QString( "CREATE %1 TABLE directories%2 ("
                    "dir VARCHAR(255) UNIQUE,"
                    "changedate INTEGER );" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );


    //create indexes
    query( QString( "CREATE INDEX album_idx%1 ON album%2( name );" )
                    .arg( conn ? "_temp" : "" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "CREATE INDEX artist_idx%1 ON artist%2( name );" )
                    .arg( conn ? "_temp" : "" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "CREATE INDEX genre_idx%1 ON genre%2( name );" )
                    .arg( conn ? "_temp" : "" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "CREATE INDEX year_idx%1 ON year%2( name );" )
                    .arg( conn ? "_temp" : "" ).arg( conn ? "_temp" : "" ), conn );

    if ( !conn )
    {
        // create related artists cache
        query( QString( "CREATE TABLE related_artists ("
                        "artist VARCHAR(255),"
                        "suggestion VARCHAR(255),"
                        "changedate INTEGER );" ) );

        query( "CREATE INDEX url_tag ON tags( url );" );
        query( "CREATE INDEX album_tag ON tags( album );" );
        query( "CREATE INDEX artist_tag ON tags( artist );" );
        query( "CREATE INDEX genre_tag ON tags( genre );" );
        query( "CREATE INDEX year_tag ON tags( year );" );
        query( "CREATE INDEX sampler_tag ON tags( sampler );" );

        query( "CREATE INDEX images_album ON images( album );" );
        query( "CREATE INDEX images_artist ON images( artist );" );

        query( "CREATE INDEX directories_dir ON directories( dir );" );
        query( "CREATE INDEX related_artists_artist ON related_artists( artist );" );
    }
}


void
CollectionDB::dropTables( DbConnection *conn )
{
    kdDebug() << k_funcinfo << endl;

    query( QString( "DROP TABLE tags%1;" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "DROP TABLE album%1;" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "DROP TABLE artist%1;" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "DROP TABLE genre%1;" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "DROP TABLE year%1;" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "DROP TABLE images%1;" ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "DROP TABLE directories%1;" ).arg( conn ? "_temp" : "" ), conn );
    if ( !conn )
    {
        query( QString( "DROP TABLE related_artists;" ) );
    }
}


void
CollectionDB::clearTables( DbConnection *conn )
{
    kdDebug() << k_funcinfo << endl;

#ifdef USE_MYSQL
    // TRUNCATE TABLE is faster than DELETE FROM TABLE, so use it when supported.
    QString clearCommand = "TRUNCATE TABLE";
#else
    QString clearCommand = "DELETE FROM";
#endif

    query( QString( "%1 tags%2;" ).arg( clearCommand ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "%1 album%2;" ).arg( clearCommand ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "%1 artist%2;" ).arg( clearCommand ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "%1 genre%2;" ).arg( clearCommand ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "%1 year%2;" ).arg( clearCommand ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "%1 images%2;" ).arg( clearCommand ).arg( conn ? "_temp" : "" ), conn );
    query( QString( "%1 directories%2;" ).arg( clearCommand ).arg( conn ? "_temp" : "" ), conn );
    if ( !conn )
    {
        query( QString( "%1 related_artists;" ).arg( clearCommand ) );
    }
}


void
CollectionDB::moveTempTables( DbConnection *conn )
{
    query( "INSERT INTO tags SELECT * FROM tags_temp;", conn );
    query( "INSERT INTO album SELECT * FROM album_temp;", conn );
    query( "INSERT INTO artist SELECT * FROM artist_temp;", conn );
    query( "INSERT INTO genre SELECT * FROM genre_temp;", conn );
    query( "INSERT INTO year SELECT * FROM year_temp;", conn );
    query( "INSERT INTO images SELECT * FROM images_temp;", conn );
    query( "INSERT INTO directories SELECT * FROM directories_temp;", conn );
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


uint
CollectionDB::artistID( QString value, bool autocreate, const bool temporary, DbConnection *conn )
{
    // lookup cache
    if ( m_cacheArtist == value )
        return m_cacheArtistID;

    uint id = IDFromValue( "artist", value, autocreate, temporary, conn );

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
CollectionDB::albumID( QString value, bool autocreate, const bool temporary, DbConnection *conn )
{
    // lookup cache
    if ( m_cacheAlbum == value )
        return m_cacheAlbumID;

    uint id = IDFromValue( "album", value, autocreate, temporary, conn );

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
CollectionDB::genreID( QString value, bool autocreate, const bool temporary, DbConnection *conn )
{
    return IDFromValue( "genre", value, autocreate, temporary, conn );
}


QString
CollectionDB::genreValue( uint id )
{
    return valueFromID( "genre", id );
}


uint
CollectionDB::yearID( QString value, bool autocreate, const bool temporary, DbConnection *conn )
{
    return IDFromValue( "year", value, autocreate, temporary, conn );
}


QString
CollectionDB::yearValue( uint id )
{
    return valueFromID( "year", id );
}


uint
CollectionDB::IDFromValue( QString name, QString value, bool autocreate, const bool temporary, DbConnection *conn )
{
    if ( temporary )
        name.append( "_temp" );
    else
        conn = NULL;

    QStringList values =
        query( QString(
            "SELECT id FROM %1 WHERE name LIKE '%2';" )
            .arg( name )
            .arg( CollectionDB::instance()->escapeString( value ) ), conn );

    uint id;
    //check if item exists. if not, should we autocreate it?
    if ( values.isEmpty() && autocreate )
    {
        query( QString( "INSERT INTO %1 ( name ) VALUES ( '%2' );" )
                        .arg( name )
                        .arg( CollectionDB::instance()->escapeString( value ) ), conn );

        id = sqlInsertID( conn );

        return id;
    }

    return values.isEmpty() ? 0 : values.first().toUInt();
}


QString
CollectionDB::valueFromID( QString table, uint id )
{
    QStringList values =
        query( QString(
            "SELECT name FROM %1 WHERE id=%2;" )
            .arg( table )
            .arg( id ) );


    return values.isEmpty() ? 0 : values.first();
}


QString
CollectionDB::escapeString( QString string )
{
    string.replace( "'", "''" );
    return string;
}


QString
CollectionDB::albumSongCount( const QString &artist_id, const QString &album_id )
{
    QStringList values =
        query( QString(
            "SELECT COUNT( url ) FROM tags WHERE album = %1 AND artist = %2;" )
            .arg( album_id )
            .arg( artist_id ) );
    return values.first();
}


void
CollectionDB::addImageToAlbum( const QString& image, QValueList< QPair<QString, QString> > info, DbConnection *conn )
{
    for ( QValueList< QPair<QString, QString> >::ConstIterator it = info.begin(); it != info.end(); ++it )
    {
        if ( (*it).first.isEmpty() || (*it).second.isEmpty() )
            continue;

        kdDebug() << "Added image for album: " << (*it).first << " - " << (*it).second << ": " << image << endl;
        query( QString( "INSERT INTO images%1 ( path, artist, album ) VALUES ( '%1', '%2', '%3' );" )
         .arg( conn ? "_temp" : "" )
         .arg( escapeString( image ) )
         .arg( escapeString( (*it).first ) )
         .arg( escapeString( (*it).second ) ), conn );
    }
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
    kdDebug() << "[CollectionDB] Saving cover for: " << artist << " - " << album << endl;

    //show a wait cursor for the duration
    amaroK::OverrideCursor keep;

    // remove existing album covers
    removeAlbumImage( artist, album );

    QDir largeCoverDir( KGlobal::dirs()->saveLocation( "data", "amarok/albumcovers/large/" ) );
    QCString key = md5sum( artist, album );

    // Save Amazon product page URL as embedded string, for later retreival
    if ( !amazonUrl.isEmpty() )
        img.setText( "amazon-url", 0, amazonUrl );

    return img.save( largeCoverDir.filePath( key ), "PNG");
}


QString
CollectionDB::findImageByMetabundle( MetaBundle trackInformation, uint width ) 
{
    QCString widthKey = makeWidthKey( width );
    QCString tagKey = md5sum( trackInformation.url().path(), trackInformation.artist() ); //what's more unique than the file name?
    QDir tagCoverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/albumcovers/tagcover/" ) );

    //FIXME: the cashed versions will never be refreshed
    if ( tagCoverDir.exists( widthKey + tagKey ) )
    {
        // cached version 
        return tagCoverDir.filePath( widthKey + tagKey );
    } else
    {
        // look into the tag
        TagLib::MPEG::File f( QFile::encodeName( trackInformation.url().path() ) );
        TagLib::ID3v2::Tag *tag = f.ID3v2Tag();
  
        if ( tag )
        {
            TagLib::ID3v2::FrameList l = f.ID3v2Tag()->frameListMap()[ "APIC" ];
            if ( !l.isEmpty() )
            {
                kdDebug() << "Found APIC frame(s)" << endl;
                TagLib::ID3v2::Frame *f = l.front();
                TagLib::ID3v2::AttachedPictureFrame *ap = (TagLib::ID3v2::AttachedPictureFrame*)f;
                  
                const TagLib::ByteVector &imgVector = ap->picture();
                kdDebug() << "Size of image: " <<  imgVector.size() << " byte" << endl;
        
                QByteArray imgData;
                const char *tempCString = imgVector.data();
        
                // is there a better way to do this?
                imgData.setRawData ( tempCString , imgVector.size() );
                QImage image = QImage( imgData ); 
        
                // if we don't reset, the whole system get's meesed up
                imgData.resetRawData ( tempCString , imgVector.size() );
                if (! image.isNull() )
                {
                    if ( width > 1 )
                    {
                        image.smoothScale( width, width, QImage::ScaleMin ).save( tagCoverDir.filePath( widthKey + tagKey ), "PNG" );
                        return tagCoverDir.filePath( widthKey + tagKey ) ;
                    } else
                    {
                        image.save( tagCoverDir.filePath( tagKey ), "PNG" );
                        return tagCoverDir.filePath( tagKey );
                    }    
                } // image.isNull
            } // apic list is empty 
        } // tag is empty
    } // caching
 
    return QString::null;
}


QString
CollectionDB::findImageByArtistAlbum( const QString &artist, const QString &album, uint width )
{
    QCString widthKey = makeWidthKey( width );
  
    if ( artist.isEmpty() && album.isEmpty() )
        return notAvailCover( width );
    else
    {
        QCString key = md5sum( artist, album );

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
                    img.smoothScale( width, width, QImage::ScaleMin ).save( m_cacheDir.filePath( widthKey + key ), "PNG" );
    
                    return m_cacheDir.filePath( widthKey + key );
                }
                else
                    return largeCoverDir.filePath( key );
        }

        // no amazon cover found, let's try to find a cover in the song's directory
        return getImageForAlbum( artist, album, width );
    }
}


QString
CollectionDB::albumImage( const uint artist_id, const uint album_id, const uint width )
{
    return albumImage( artistValue( artist_id ), albumValue( album_id ), width );
}


QString
CollectionDB::albumImage( const QString &artist, const QString &album, uint width ) 
{
    // we aren't going to need a 1x1 size image. this is just a quick hack to be able to show full size images.
    if ( width == 1) width = AmarokConfig::coverPreviewSize();
    return findImageByArtistAlbum( artist, album, width );
}


QString
CollectionDB::albumImage( MetaBundle trackInformation, uint width ) 
{
    // we aren't going to need a 1x1 size image. this is just a quick hack to be able to show full size images.
    if ( width == 1) width = AmarokConfig::coverPreviewSize();
  
    QString path = findImageByMetabundle( trackInformation, width );
    if ( path.isNull() )
      path = findImageByArtistAlbum( trackInformation.artist(), trackInformation.album(), width );

    return path;
}


QCString 
CollectionDB::makeWidthKey( uint width ) 
{
    return QString::number( width ).local8Bit() + "@";
} 


QString
CollectionDB::getImageForAlbum( const QString& artist, const QString& album, uint width )
{
    if ( !width ) width = AmarokConfig::coverPreviewSize();
    QCString widthKey = QString::number( width ).local8Bit() + "@";

    if ( album.isEmpty() )
        return notAvailCover( width );

    QStringList values =
        query( QString(
            "SELECT path FROM images WHERE artist LIKE '%1' AND album LIKE '%2' ORDER BY path;" )
            .arg( escapeString( artist ) )
            .arg( escapeString( album ) ) );

    if ( !values.isEmpty() )
    {
        QString image( values.first() );
        uint matches = 0;
        uint maxmatches = 0;
        for ( uint i = 0; i < values.count(); i++ )
        {
            matches = values[i].contains( "front", false ) + values[i].contains( "cover", false ) + values[i].contains( "folder", false );
            if ( matches > maxmatches )
            {
                image = values[i];
                maxmatches = matches;
            }
        }

        QCString key = md5sum( artist, album, image );
        if ( !m_cacheDir.exists( widthKey + key ) )
        {
            QImage img = QImage( image );
            img.smoothScale( width, width, QImage::ScaleMin ).save( m_cacheDir.filePath( widthKey + key ), "PNG" );
        }

        return m_cacheDir.filePath( widthKey + key );
    }

    return notAvailCover( width );
}


bool
CollectionDB::removeAlbumImage( const QString &artist, const QString &album )
{
    QCString widthKey = "*@";
    QCString key = md5sum( artist, album );

    // remove scaled versions of images
    QStringList scaledList = m_cacheDir.entryList( widthKey + key );
    if ( scaledList.count() > 0 )
        for ( uint i = 0; i < scaledList.count(); i++ )
            QFile::remove( m_cacheDir.filePath( scaledList[ i ] ) );

    // remove large, original image
    QDir largeCoverDir( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/albumcovers/large/" ) );

    if ( largeCoverDir.exists( key ) && QFile::remove( largeCoverDir.filePath( key ) ) ) {
        emit coverRemoved( artist, album );
        return true;
    }

    return false;
}

bool
CollectionDB::removeAlbumImage( const uint artist_id, const uint album_id )
{
    return removeAlbumImage( artistValue( artist_id ), albumValue( album_id ) );
}


QString
CollectionDB::notAvailCover( int width )
{
    if ( !width ) width = AmarokConfig::coverPreviewSize();
    QString widthKey = QString::number( width ) + "@";

    if( m_cacheDir.exists( widthKey + "nocover.png" ) )
        return m_cacheDir.filePath( widthKey + "nocover.png" );
    else
    {
        QImage nocover( locate( "data", "amarok/images/nocover.png" ) );
        nocover.smoothScale( width, width, QImage::ScaleMin ).save( m_cacheDir.filePath( widthKey + "nocover.png" ), "PNG" );
        return m_cacheDir.filePath( widthKey + "nocover.png" );
    }
}


QStringList
CollectionDB::artistList( bool withUnknowns, bool withCompilations )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );

    if ( !withUnknowns )
        qb.excludeMatch( QueryBuilder::tabArtist, "" );
    if ( !withCompilations )
        qb.setOptions( QueryBuilder::optNoCompilations );

    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    return qb.run();
}


QStringList
CollectionDB::albumList( bool withUnknowns, bool withCompilations )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );

    if ( !withUnknowns )
        qb.excludeMatch( QueryBuilder::tabAlbum, "" );
    if ( !withCompilations )
        qb.setOptions( QueryBuilder::optNoCompilations );

    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    return qb.run();
}


QStringList
CollectionDB::genreList( bool withUnknowns, bool withCompilations )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );

    if ( !withUnknowns )
        qb.excludeMatch( QueryBuilder::tabGenre, "" );
    if ( !withCompilations )
        qb.setOptions( QueryBuilder::optNoCompilations );

    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.sortBy( QueryBuilder::tabGenre, QueryBuilder::valName );
    return qb.run();
}


QStringList
CollectionDB::yearList( bool withUnknowns, bool withCompilations )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );

    if ( !withUnknowns )
        qb.excludeMatch( QueryBuilder::tabYear, "" );
    if ( !withCompilations )
        qb.setOptions( QueryBuilder::optNoCompilations );

    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
    return qb.run();
}


QStringList
CollectionDB::albumListOfArtist( const QString &artist, bool withUnknown, bool withCompilations )
{
    return query( "SELECT DISTINCT album.name FROM tags, album, artist WHERE "
                  "tags.album = album.id AND tags.artist = artist.id "
                  "AND artist.name = '" + escapeString( artist ) + "' " +
                  ( withUnknown ? QString::null : "AND album.name <> '' " ) +
                  ( withCompilations ? QString::null : "AND tags.sampler = 0 " ) +
                  "ORDER BY lower( album.name );" );
}


QStringList
CollectionDB::artistAlbumList( bool withUnknown, bool withCompilations )
{
    return query( "SELECT DISTINCT artist.name, album.name FROM tags, album, artist WHERE "
                  "tags.album = album.id AND tags.artist = artist.id " +
                  ( withUnknown ? QString::null : "AND album.name <> '' AND artist.name <> '' " ) +
                  ( withCompilations ? QString::null : "AND tags.sampler = 0 " ) +
                  "ORDER BY lower( album.name );" );
}


bool
CollectionDB::addSong( MetaBundle* bundle, const bool temporary, DbConnection *conn )
{
    if ( !QFileInfo( bundle->url().path() ).isReadable() ) return false;

    QString command = "INSERT INTO tags_temp "
                      "( url, dir, createdate, album, artist, genre, year, title, comment, track, sampler, length ) "
                      "VALUES ('";

    QString artist = bundle->artist();
    QString title = bundle->title();
    if ( title.isEmpty() )
    {
        title = bundle->url().fileName();
        if ( bundle->url().fileName().find( '-' ) > 0 )
        {
            if ( artist.isEmpty() ) artist = bundle->url().fileName().section( '-', 0, 0 ).stripWhiteSpace();
            title = bundle->url().fileName().section( '-', 1 ).stripWhiteSpace();
            title = title.left( title.findRev( '.' ) ).stripWhiteSpace();
            if ( title.isEmpty() ) title = bundle->url().fileName();
        }
    }
    bundle->setArtist( artist );
    bundle->setTitle( title );

    command += escapeString( bundle->url().path() ) + "','";
    command += escapeString( bundle->url().directory() ) + "',";
    command += "'" + QString::number( QFileInfo( bundle->url().path() ).created().toTime_t() ) + "',";

    command += escapeString( QString::number( albumID( bundle->album(), true, !temporary, conn ) ) ) + ",";
    command += escapeString( QString::number( artistID( bundle->artist(), true, !temporary, conn ) ) ) + ",";
    command += escapeString( QString::number( genreID( bundle->genre(), true, !temporary, conn ) ) ) + ",'";
    command += escapeString( QString::number( yearID( bundle->year(), true, !temporary, conn ) ) ) + "','";

    command += escapeString( bundle->title() ) + "','";
    command += escapeString( bundle->comment() ) + "','";
    command += escapeString( bundle->track() ) + "', ";
    command += artist == i18n( "Various Artists" ) ? "1" : "0";
    command += ", 0);";

    //FIXME: currently there's no way to check if an INSERT query failed or not - always return true atm.
    query( command, conn );
    return true;
}


bool
CollectionDB::getMetaBundleForUrl( const QString& url , MetaBundle* bundle )
{
    QStringList values =
        query( QString(
            "SELECT album.name, artist.name, genre.name, tags.title, "
            "year.name, tags.comment, tags.track, tags.bitrate, tags.length, "
            "tags.samplerate "
            "FROM tags, album, artist, genre, year "
            "WHERE album.id = tags.album AND artist.id = tags.artist AND "
            "genre.id = tags.genre AND year.id = tags.year AND tags.url = '%1';" )
            .arg( escapeString( url ) ) );

    if ( !values.isEmpty() )
    {
        bundle->setAlbum( values[0] );
        bundle->setArtist( values[1] );
        bundle->setGenre( values[2] );
        bundle->setTitle( values[3] );
        bundle->setYear( values[4] );
        bundle->setComment( values[5] );
        bundle->setTrack( values[6] );
        bundle->setBitrate( values[7].toInt() );
        bundle->setLength( values[8].toInt() );
        bundle->setSampleRate( values[9].toInt() );

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
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, percentage FROM statistics "
            "WHERE url = '%1';" )
            .arg( escapeString( url ) ) );

    if ( !values.isEmpty() )
    {
        // entry exists, increment playcounter and update accesstime
        score = ( ( values[2].toDouble() * values.first().toInt() ) + percentage ) / ( values.first().toInt() + 1 );

        query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                        "VALUES ( '%1', %2, %3, %4, %5 );" )
                        .arg( escapeString( url ) )
                        .arg( values[1] )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( score )
                        .arg( values[0] + " + 1" ) );
    }
    else
    {
        // entry didnt exist yet, create a new one
        score = ( ( 50 + percentage ) / 2 );

        query( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                        "VALUES ( '%1', %2, %3, %4, 1 );" )
                        .arg( escapeString( url ) )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( score ) );
    }

    int iscore = getSongPercentage( url );
    emit scoreChanged( url, iscore );
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
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, accessdate FROM statistics WHERE url = '%1';" )
            .arg( escapeString( url ) ) );

    // check boundaries
    if ( percentage > 100 ) percentage = 100;
    if ( percentage < 1 )   percentage = 1;

    if ( !values.isEmpty() )
    {
        // entry exists
        query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                        "VALUES ( '%1', '%2', '%3', %4, %5 );" )
                        .arg( escapeString( url ) )
                        .arg( values[1] )
                        .arg( values[2] )
                        .arg( percentage )
                        .arg( values[0] ) );
    }
    else
    {
        query( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                        "VALUES ( '%1', %2, %3, %4, 0 );" )
                        .arg( escapeString( url ) )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( percentage ) );
    }

    emit scoreChanged( url, percentage );
}


void
CollectionDB::updateDirStats( QString path, const long datetime, DbConnection *conn )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    query( QString( "REPLACE INTO directories%1 ( dir, changedate ) VALUES ( '%2', %3 );" )
                    .arg( conn ? "_temp" : "" )
                    .arg( escapeString( path ) )
                    .arg( datetime ), conn );
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

    QStringList values =
        query( QString(
            "SELECT changedate FROM directories WHERE dir = '%1';" )
            .arg( escapeString( path ) ) );

    return !values.isEmpty();
}


bool
CollectionDB::isFileInCollection( const QString &url  )
{
    QStringList values =
        query( QString(
            "SELECT url FROM tags WHERE url = '%1';" )
            .arg( escapeString( url ) ) );

    return !values.isEmpty();
}


QStringList
CollectionDB::similarArtists( const QString &artist, uint count )
{
    QStringList values =
        query( QString(
            "SELECT suggestion FROM related_artists WHERE artist = '%1' "
            "LIMIT 0, %2;" )
            .arg( escapeString( artist ) ).arg( count ) );
    if ( values.isEmpty() )
        Scrobbler::instance()->similarArtists( artist );

    return values;
}


void
CollectionDB::checkCompilations( const QString &path, DbConnection *conn )
{
    QStringList albums;
    QStringList artists;
    QStringList dirs;

    albums = query( QString( "SELECT DISTINCT album_temp.name FROM tags_temp, album_temp WHERE tags_temp.dir = '%1' AND album_temp.id = tags_temp.album;" )
              .arg( escapeString( path ) ), conn );

    for ( uint i = 0; i < albums.count(); i++ )
    {
        if ( albums[ i ].isEmpty() ) continue;

        const uint album_id = albumID( albums[ i ], FALSE, TRUE, conn );
        artists = query( QString( "SELECT DISTINCT artist_temp.name FROM tags_temp, artist_temp WHERE tags_temp.album = '%1' AND tags_temp.artist = artist_temp.id;" )
                            .arg( album_id ), conn );
        dirs    = query( QString( "SELECT DISTINCT dir FROM tags_temp WHERE album = '%1';" )
                            .arg( album_id ), conn );

        if ( artists.count() > dirs.count() )
        {
            kdDebug() << "Detected compilation: " << albums[ i ] << " - " << artists.count() << ":" << dirs.count() << endl;
            query( QString( "UPDATE tags_temp SET sampler = 1 WHERE album = '%1';" )
                      .arg( album_id ), conn );
        }
    }
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
CollectionDB::updateTags( const QString &url, const MetaBundle &bundle, const bool updateView )
{
    QString command = "UPDATE tags SET ";
    command += "title = '" + escapeString( bundle.title() ) + "', ";
    command += "artist = " + QString::number( artistID( bundle.artist(), true ) ) + ", ";
    command += "album = "  + QString::number( albumID( bundle.album(), true ) ) + ", ";
    command += "genre = "  + QString::number( genreID( bundle.genre(), true ) ) + ", ";
    command += "year = "   + QString::number( yearID( bundle.year(), true ) ) + ", ";
    if ( !bundle.track().isEmpty() )
        command += "track = " + bundle.track() + ", ";
    command += "comment = '" + escapeString( bundle.comment() ) + "' ";
    command += "WHERE url = '" + escapeString( url ) + "';";

    query( command );

    if ( EngineController::instance()->bundle().url() == bundle.url() )
    {
        kdDebug() << "Current song edited, updating widgets: " << bundle.title() << endl;
        EngineController::instance()->currentTrackMetaDataChanged( bundle );
    }

    if ( updateView )    //update the collection browser
      CollectionView::instance()->renderView();
}


void
CollectionDB::updateURL( const QString &url, const bool updateView )
{
    const MetaBundle bundle = MetaBundle( url );
    updateTags( url, bundle, updateView );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

QCString
CollectionDB::md5sum( const QString& artist, const QString& album, const QString& file )
{
    KMD5 context( artist.lower().local8Bit() + album.lower().local8Bit() + file.local8Bit() );
//     kdDebug() << "MD5 SUM for " << artist << ", " << album << ": " << context.hexDigest() << endl;
    return context.hexDigest();
}


void CollectionDB::engineTrackEnded( int finalPosition, int trackLength )
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
    addSongPercentage( url.path(), pct );
}


void
CollectionDB::timerEvent( QTimerEvent* )
{
    if ( AmarokConfig::monitorChanges() )
        scanMonitor();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::fetchCover( QWidget* parent, const QString& artist, const QString& album, bool noedit ) //SLOT
{
    #ifdef AMAZON_SUPPORT
    kdDebug() << "[CollectionDB] Fetching cover for " << artist << " - " << album << endl;

    CoverFetcher* fetcher = new CoverFetcher( parent, artist, album );
    connect( fetcher, SIGNAL(result( CoverFetcher* )), SLOT(coverFetcherResult( CoverFetcher* )) );
    fetcher->setUserCanEditQuery( !noedit );
    fetcher->startFetch();
    #endif
}


void
CollectionDB::scanMonitor()  //SLOT
{
    if ( !m_isScanning )
        scanModifiedDirs( AmarokConfig::scanRecursively(), AmarokConfig::importPlaylists() );
}


void
CollectionDB::startScan()  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    if ( AmarokConfig::collectionFolders().isEmpty() )
    {
        dropTables();
        createTables();
    }
    else if ( !m_isScanning )
    {
        scan( AmarokConfig::collectionFolders(), AmarokConfig::scanRecursively(),
              AmarokConfig::importPlaylists() );

        amaroK::StatusBar::instance()->message( i18n( "Building Collection..." ) );
    }
}


void
CollectionDB::stopScan() //SLOT
{
    CollectionReader::stop();
    m_isScanning = false;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::dirDirty( const QString& path )
{
    kdDebug() << k_funcinfo << "Dirty: " << path << endl;

    ThreadWeaver::instance()->queueJob( new CollectionReader( this, PlaylistBrowser::instance(), path, false, false, true ) );
}


void
CollectionDB::coverFetcherResult( CoverFetcher *fetcher )
{
    if ( fetcher->error() ) {
        kdError() << fetcher->errorMessage() << endl;
        emit coverFetcherError( fetcher->errorMessage() );
    }
    else {
        setAlbumImage( fetcher->artist(), fetcher->album(), fetcher->image(), fetcher->amazonURL() );
        emit coverFetched( fetcher->artist(), fetcher->album() );
    }
}


void
CollectionDB::similarArtistsFetched( const QString& artist, const QStringList& suggestions )
{
    kdDebug() << "Similar artists received" << endl;
    query( QString( "DELETE FROM related_artists WHERE artist = '%1';" ).arg( escapeString( artist ) ) );

    for ( uint i = 0; i < suggestions.count(); i++ )
        query( QString( "INSERT INTO related_artists ( artist, suggestion, changedate ) VALUES ( '%1', '%2', 0 );" )
                  .arg( escapeString( artist ) )
                  .arg( escapeString( suggestions[ i ] ) ) );

    emit similarArtistsFetched( artist );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////


void
CollectionDB::scan( const QStringList& folders, bool recursively, bool importPlaylists )
{
    kdDebug() << k_funcinfo << endl;

    if ( !folders.isEmpty() )
    {
        m_isScanning = true;
        emit scanStarted();

        ThreadWeaver::instance()->onlyOneJob( new CollectionReader( this, PlaylistBrowser::instance(), folders, recursively, importPlaylists, false ) );
    }
}


void
CollectionDB::scanModifiedDirs( bool recursively, bool importPlaylists )
{
    kdDebug() << k_funcinfo << endl;

    m_isScanning = true;
    emit scanStarted();

    ThreadWeaver::instance()->onlyOneJob( new CollectionReader( this, PlaylistBrowser::instance(), QStringList(),
                                            recursively, importPlaylists, true ) );
}


void
CollectionDB::customEvent( QCustomEvent *e )
{
//    kdDebug() << k_funcinfo << endl;
    CollectionReader::ProgressEvent* p = dynamic_cast<CollectionReader::ProgressEvent*>( e );

    if ( p )
        switch ( p->state() )
        {
            case CollectionReader::ProgressEvent::Stop:
                emit scanDone( p->value() > 0 );
                m_isScanning = false;
                break;
        }
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS DbConnection
//////////////////////////////////////////////////////////////////////////////////////////

DbConnection::DbConnection()
{
#ifdef USE_MYSQL
    m_db = mysql::mysql_init(NULL);

    if ( m_db )
    {
        if (AmarokConfig::mySqlUser().isEmpty())
            pApp->slotConfigAmarok(5);

        if (mysql::mysql_real_connect(m_db, AmarokConfig::mySqlHost().latin1(),
                                            AmarokConfig::mySqlUser().latin1(),
                                            AmarokConfig::mySqlPassword().latin1(),
                                            AmarokConfig::mySqlDbName().latin1(),
                                            AmarokConfig::mySqlPort(),
                                            NULL, CLIENT_COMPRESS))
        {
            m_initialized = true;
        }
        else
        {
            m_initialized = false;
            if (mysql::mysql_real_connect(
                m_db, AmarokConfig::mySqlHost().latin1(),
                AmarokConfig::mySqlUser().latin1(),
                AmarokConfig::mySqlPassword().latin1(),
                NULL,
                AmarokConfig::mySqlPort(),
                NULL, CLIENT_COMPRESS))
            {
                if ( !mysql::mysql_query(
                        m_db,
                        QString("CREATE DATABASE " + AmarokConfig::mySqlDbName()).latin1()) )
                    kdError() << "Failed to create database " << AmarokConfig::mySqlDbName() << "\n";
            }
        }
    }
    else
        kdError() << "Failed to allocate/initialize MySql struct\n";
#else
    QCString path = ( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" )
                  + "collection.db" ).local8Bit();

    // Open database file and check for correctness
    m_initialized = false;
    QFile file( path );
    if ( file.open( IO_ReadOnly ) )
    {
        QString format;
        file.readLine( format, 50 );
        if ( !format.startsWith( "SQLite format 3" ) )
        {
            kdWarning() << "Database versions incompatible. Removing and rebuilding database.\n";
        }
        else if ( sqlite3_open( path, &m_db ) != SQLITE_OK )
        {
            kdWarning() << "Database file corrupt. Removing and rebuilding database.\n";
            sqlite3_close( m_db );
        }
        else
            m_initialized = true;
    }
    if ( !m_initialized )
    {
        // Remove old db file; create new
        QFile::remove( path );
        if ( sqlite3_open( path, &m_db ) == SQLITE_OK )
        {
            m_initialized = true;
        }
    }
#endif
}


DbConnection::~DbConnection()
{
#ifdef USE_MYSQL
    if (m_db) mysql::mysql_close( m_db );
#else
    sqlite3_close( m_db );
#endif
}


#ifdef USE_MYSQL
mysql::MYSQL* DbConnection::db()
{
    return m_db;
}
#else
sqlite3* DbConnection::db()
{
    return m_db;
}
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS DbConnectionPool
//////////////////////////////////////////////////////////////////////////////////////////

DbConnectionPool::DbConnectionPool() : m_semaphore( POOL_SIZE )
{
    m_semaphore += POOL_SIZE;
    enqueue( new DbConnection() );
    m_semaphore--;
    kdDebug() << "available db connections: " << m_semaphore.available() << endl;
}


DbConnectionPool::~DbConnectionPool()
{
    m_semaphore += POOL_SIZE;
    DbConnection *conn;
    while ( ( conn = dequeue() ) != 0 )
    {
        delete conn;
    }
}


void DbConnectionPool::createDbConnections()
{
    for ( int i = 0; i < POOL_SIZE - 1; i++ )
    {
        enqueue( new DbConnection() );
        m_semaphore--;
    }
    kdDebug() << "available db connections: " << m_semaphore.available() << endl;
}


DbConnection *DbConnectionPool::getDbConnection()
{
    m_semaphore++;
    return dequeue();
}


void DbConnectionPool::putDbConnection( const DbConnection *conn )
{
    enqueue( conn );
    m_semaphore--;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS QueryBuilder
//////////////////////////////////////////////////////////////////////////////////////////

QueryBuilder::QueryBuilder()
{
    clear();
}


void
QueryBuilder::linkTables( int tables )
{
    if ( tables & tabAlbum ) m_tables += ",album";
    if ( tables & tabArtist ) m_tables += ",artist";
    if ( tables & tabGenre ) m_tables += ",genre";
    if ( tables & tabYear ) m_tables += ",year";
    if ( tables & tabStats ) m_tables += ",statistics";

    // when there are multiple tables involved, we always need table tags for linking them
    m_tables = m_tables.mid( 1 );
    if ( m_tables.contains( ',' ) ) tables |= tabSong;

    if ( tables & tabSong )
    {
        m_tables = "tags," + m_tables;
        if ( tables & tabAlbum ) m_where += "AND album.id=tags.album ";
        if ( tables & tabArtist ) m_where += "AND artist.id=tags.artist ";
        if ( tables & tabGenre ) m_where += "AND genre.id=tags.genre ";
        if ( tables & tabYear ) m_where += "AND year.id=tags.year ";
        if ( tables & tabStats ) m_where += "AND statistics.url=tags.url ";
    }
}


void
QueryBuilder::addReturnValue( int table, int value )
{
    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ",";
    if ( table & tabStats && value & valScore ) m_values += "round(";

    if ( table & tabAlbum ) m_values += "album.";
    if ( table & tabArtist ) m_values += "artist.";
    if ( table & tabGenre ) m_values += "genre.";
    if ( table & tabYear ) m_values += "year.";
    if ( table & tabSong ) m_values += "tags.";
    if ( table & tabStats ) m_values += "statistics.";

    if ( value & valID ) m_values += "id";
    if ( value & valName ) m_values += "name";
    if ( value & valURL ) m_values += "url";
    if ( value & valTitle ) m_values += "title";
    if ( value & valTrack ) m_values += "track";
    if ( value & valScore ) m_values += "percentage";

    if ( table & tabStats && value & valScore ) m_values += " + 0.4 )";

    m_linkTables |= table;
    m_returnValues++;
}


uint
QueryBuilder::countReturnValues()
{
    return m_returnValues;
}


void
QueryBuilder::addFilter( int tables, const QString& filter, int mode )
{
    if ( !filter.isEmpty() )
    {
        m_where += "AND ( 0 ";

#ifdef USE_MYSQL
        if ( mode == modeFuzzy )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabArtist ) m_where += "OR artist.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabGenre ) m_where += "OR genre.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabYear ) m_where += "OR year.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabSong ) m_where += "OR MATCH ( tags.title ) AGAINST ( '" + CollectionDB::instance()->escapeString( filter ) + "' ) ";
        }
        else
        {
#endif

        if ( tables & tabAlbum ) m_where += "OR album.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        if ( tables & tabArtist ) m_where += "OR artist.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        if ( tables & tabGenre ) m_where += "OR genre.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        if ( tables & tabYear ) m_where += "OR year.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        if ( tables & tabSong ) m_where += "OR tags.title LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";

#ifdef USE_MYSQL
        }
#endif

        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::addMatch( int tables, const QString& match )
{
    if ( !match.isEmpty() )
    {
        m_where += "AND ( 0 ";
        if ( tables & tabAlbum ) m_where += "OR album.name LIKE '" + CollectionDB::instance()->escapeString( match ) + "' ";
        if ( tables & tabArtist ) m_where += "OR artist.name LIKE '" + CollectionDB::instance()->escapeString( match ) + "' ";
        if ( tables & tabGenre ) m_where += "OR genre.name LIKE '" + CollectionDB::instance()->escapeString( match ) + "' ";
        if ( tables & tabYear ) m_where += "OR year.name LIKE '" + CollectionDB::instance()->escapeString( match ) + "' ";
        if ( tables & tabSong ) m_where += "OR tags.title LIKE '" + CollectionDB::instance()->escapeString( match ) + "' ";

        if ( match == i18n( "Unknown" ) )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
            if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
            if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
            if ( tables & tabYear ) m_where += "OR year.name = '' ";
        }
        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::addMatches( int tables, const QStringList& match )
{
    if ( !match.isEmpty() )
    {
        m_where += "AND ( 0 ";

        for ( uint i = 0; i < match.count(); i++ )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabArtist ) m_where += "OR artist.name LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabGenre ) m_where += "OR genre.name LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabYear ) m_where += "OR year.name LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabSong ) m_where += "OR tags.title LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";

            if ( match[i] == i18n( "Unknown" ) )
            {
                if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
                if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
                if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
                if ( tables & tabYear ) m_where += "OR year.name = '' ";
            }
        }

        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::excludeFilter( int tables, const QString& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += "AND ( 1 ";
        if ( tables & tabAlbum ) m_where += "AND album.name <> '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        if ( tables & tabArtist ) m_where += "AND artist.name <> '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        if ( tables & tabGenre ) m_where += "AND genre.name <> '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        if ( tables & tabYear ) m_where += "AND year.name <> '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        if ( tables & tabSong ) m_where += "AND tags.title <> '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::excludeMatch( int tables, const QString& match )
{
    if ( !match.isEmpty() )
    {
        m_where += "AND ( 1 ";
        if ( tables & tabAlbum ) m_where += "AND album.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
        if ( tables & tabArtist ) m_where += "AND artist.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
        if ( tables & tabGenre ) m_where += "AND genre.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
        if ( tables & tabYear ) m_where += "AND year.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
        if ( tables & tabSong ) m_where += "AND tags.title <> '" + CollectionDB::instance()->escapeString( match ) + "' ";

        if ( match == i18n( "Unknown" ) )
        {
            if ( tables & tabAlbum ) m_where += "AND album.name <> '' ";
            if ( tables & tabArtist ) m_where += "AND artist.name <> '' ";
            if ( tables & tabGenre ) m_where += "AND genre.name <> '' ";
            if ( tables & tabYear ) m_where += "AND year.name <> '' ";
        }
        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::setOptions( int options )
{
    if ( options & optNoCompilations || options & optOnlyCompilations )
        m_linkTables |= tabSong;

    if ( options & optNoCompilations ) m_where += "AND tags.sampler = 0 ";
    if ( options & optOnlyCompilations ) m_where += "AND tags.sampler = 1 ";
    if ( options & optRemoveDuplicates ) m_values = "DISTINCT " + m_values;

    if ( options & optRandomize )
    {
        if ( !m_sort.isEmpty() ) m_sort += ",";
        m_sort +=

#ifdef USE_MYSQL
            "RAND() ";
#else
            "random() ";
#endif
    }
}


void
QueryBuilder::sortBy( int table, int value, bool descending )
{
    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore ) b = false;

    if ( !m_sort.isEmpty() ) m_sort += ",";
    if ( b ) m_sort += "LOWER( ";

    if ( table & tabAlbum ) m_sort += "album.";
    if ( table & tabArtist ) m_sort += "artist.";
    if ( table & tabGenre ) m_sort += "genre.";
    if ( table & tabYear ) m_sort += "year.";
    if ( table & tabSong ) m_sort += "tags.";
    if ( table & tabStats ) m_sort += "statistics.";

    if ( value & valID ) m_sort += "id";
    if ( value & valName ) m_sort += "name";
    if ( value & valURL ) m_sort += "url";
    if ( value & valTitle ) m_sort += "title";
    if ( value & valTrack ) m_sort += "track";
    if ( value & valScore ) m_sort += "percentage";

    if ( b ) m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";
}


void
QueryBuilder::setLimit( int startPos, int length )
{
    m_limit = QString( "LIMIT %1, %2 " ).arg( startPos ).arg( length );
}


QStringList
QueryBuilder::run()
{
    linkTables( m_linkTables );

    QString cmd = "SELECT " + m_values + " FROM " + m_tables + " WHERE 1 " + m_where;
    if ( !m_sort.isEmpty() ) cmd += " ORDER BY " + m_sort;
    cmd += m_limit;

    kdDebug() << cmd << endl;

    return CollectionDB::instance()->query( cmd );
}


void
QueryBuilder::clear()
{
    m_values = "";
    m_tables = "";
    m_where = "";
    m_sort = "";
    m_limit = "";

    m_linkTables = 0;
    m_returnValues = 0;
}


#include "collectiondb.moc"
