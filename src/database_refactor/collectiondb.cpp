// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// (c) 2005 Ian Monroe <ian@monroe.nu>
// See COPYING file for licensing information.

#define DEBUG_PREFIX "CollectionDB"

#include "app.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "config.h"
#include "debug.h"
#include "collectionbrowser.h"    //updateTags()
#include "collectiondb.h"
#include "collectionreader.h"
#include "coverfetcher.h"
#include "enginecontroller.h"
#include "metabundle.h"           //updateTags()
#include "playlist.h"
#include "playlistbrowser.h"
#include "pluginmanager.h"
#include "scrobbler.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <qfile.h>
#include <qimage.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kinputdialog.h>         //setupCoverFetcher()
#include <kio/job.h>
#include <klineedit.h>            //setupCoverFetcher()
#include <klocale.h>
#include <kmdcodec.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kio/netaccess.h>

#include <cmath>                 //DbConnection::sqlite_power()
#include <ctime>                 //query()
#include <unistd.h>              //usleep()

#include <taglib/mpegfile.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/tbytevector.h>


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionDB
//////////////////////////////////////////////////////////////////////////////////////////

CollectionDB* CollectionDB::instance()
{
    static CollectionDB db;
    return &db;
}


CollectionDB::CollectionDB()
        : EngineObserver( EngineController::instance() )
        , m_cacheDir( amaroK::saveLocation() )
        , m_coverDir( amaroK::saveLocation() )
{
    DEBUG_BLOCK

    // create cover dir, if it doesn't exist.
    if( !m_coverDir.exists( "albumcovers", false ) )
        m_coverDir.mkdir( "albumcovers", false );
    m_coverDir.cd( "albumcovers" );

     // create image cache dir, if it doesn't exist.
    if( !m_cacheDir.exists( "albumcovers/cache", false ) )
        m_cacheDir.mkdir( "albumcovers/cache", false );
    m_cacheDir.cd( "albumcovers/cache" );

    // Load DBEngine plugin
    QString query = "[X-KDE-Amarok-plugintype] == 'dbengine' and [X-KDE-Amarok-name] != '%1'";
    KTrader::OfferList offers = PluginManager::query( query.arg( "sqlite-dbengine" ) );
    m_dbEngine = (DBEngine*) PluginManager::createFromService( offers.first() );

    //<OPEN DATABASE>
    initialize();
    //</OPEN DATABASE>

    // TODO: Should write to config in dtor, but it crashes...
    KConfig* config = amaroK::config( "Collection Browser" );
    config->writeEntry( "Database Version", DATABASE_VERSION );
    config->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );

    startTimer( MONITOR_INTERVAL * 1000 );

    connect( Scrobbler::instance(), SIGNAL( similarArtistsFetched( const QString&, const QStringList& ) ),
             this,                  SLOT( similarArtistsFetched( const QString&, const QStringList& ) ) );
}


CollectionDB::~CollectionDB()
{
    DEBUG_FUNC_INFO

    destroy();

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
    return m_dbConnPool->getDbConnection();
}


void
CollectionDB::returnStaticDbConnection( DbConnection *conn )
{
    m_dbConnPool->putDbConnection( conn );
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
        debug() << "Query-start: " << statement << endl;

    clock_t start = clock();

    DbConnection *dbConn;
    if ( conn != NULL )
    {
        dbConn = conn;
    }
    else
    {
        dbConn = m_dbConnPool->getDbConnection();
    }

    QStringList values = dbConn->query( statement );

    if ( conn == NULL )
    {
        m_dbConnPool->putDbConnection( dbConn );
    }

    if ( DEBUG )
    {
        clock_t finish = clock();
        const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
        debug() << "SQL-query (" << duration << "s): " << statement << endl;
    }
    return values;
}


/**
 * Executes a SQL insert on the already opened database
 * @param statement SQL statement to execute. Only one SQL statement is allowed.
 * @return          The rowid of the inserted item.
 */
int
CollectionDB::insert( const QString& statement, const QString& table, DbConnection *conn )
{
    if ( DEBUG )
        debug() << "insert-start: " << statement << endl;

    clock_t start = clock();

    DbConnection *dbConn;
    if ( conn != NULL )
    {
        dbConn = conn;
    }
    else
    {
        dbConn = m_dbConnPool->getDbConnection();
    }

    int id = dbConn->insert( statement, table );

    if ( conn == NULL )
    {
        m_dbConnPool->putDbConnection( dbConn );
    }

    if ( DEBUG )
    {
        clock_t finish = clock();
        const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
        debug() << "SQL-insert (" << duration << "s): " << statement << endl;
    }
    return id;
}


bool
CollectionDB::isEmpty()
{
    QStringList values;

    if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql)
    {
        values = query( "SELECT COUNT( url ) FROM tags OFFSET 0 LIMIT 1;" );
    }
    else
    {
        values = query( "SELECT COUNT( url ) FROM tags LIMIT 0, 1;" );
    }

    return values.isEmpty() ? true : values.first() == "0";
}


bool
CollectionDB::isValid()
{
    QStringList values1;
    QStringList values2;

    if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql) {
        values1 = query( "SELECT COUNT( url ) FROM tags OFFSET 0 LIMIT 1;" );
        values2 = query( "SELECT COUNT( url ) FROM statistics OFFSET 0 LIMIT 1;" );
    }
    else
    {
        values1 = query( "SELECT COUNT( url ) FROM tags LIMIT 0, 1;" );
        values2 = query( "SELECT COUNT( url ) FROM statistics LIMIT 0, 1;" );
    }

    //TODO? this returns true if value1 or value2 is not empty. Shouldn't this be and (&&)???
    return !values1.isEmpty() || !values2.isEmpty();
}


void
CollectionDB::createTables( DbConnection *conn )
{
    DEBUG_FUNC_INFO

    //create tag table
    query( QString( "CREATE %1 TABLE tags%2 ("
                    "url " + textColumnType() + ","
                    "dir " + textColumnType() + ","
                    "createdate INTEGER,"
                    "album INTEGER,"
                    "artist INTEGER,"
                    "genre INTEGER,"
                    "title " + textColumnType() + ","
                    "year INTEGER,"
                    "comment " + textColumnType() + ","
                    "track NUMERIC(4),"
                    "bitrate INTEGER,"
                    "length INTEGER,"
                    "samplerate INTEGER,"
                    "sampler BOOL );" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );

    QString albumAutoIncrement = "";
    QString artistAutoIncrement = "";
    QString genreAutoIncrement = "";
    QString yearAutoIncrement = "";
    if ( m_dbConnPool->getDbConnectionType() == DbConnection::postgresql )
    {
        query( QString( "CREATE SEQUENCE album_seq;" ), conn );
        query( QString( "CREATE SEQUENCE artist_seq;" ), conn );
        query( QString( "CREATE SEQUENCE genre_seq;" ), conn );
        query( QString( "CREATE SEQUENCE year_seq;" ), conn );

        albumAutoIncrement = QString("DEFAULT nextval('album_seq')");
        artistAutoIncrement = QString("DEFAULT nextval('artist_seq')");
        genreAutoIncrement = QString("DEFAULT nextval('genre_seq')");
        yearAutoIncrement = QString("DEFAULT nextval('year_seq')");
    }
    else if ( m_dbConnPool->getDbConnectionType() == DbConnection::mysql )
    {
        albumAutoIncrement = "AUTO_INCREMENT";
        artistAutoIncrement = "AUTO_INCREMENT";
        genreAutoIncrement = "AUTO_INCREMENT";
        yearAutoIncrement = "AUTO_INCREMENT";
    }
    //create album table
    query( QString( "CREATE %1 TABLE album%2 ("
                    "id INTEGER PRIMARY KEY %3,"
                    "name " + textColumnType() + ");" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" )
                    .arg( albumAutoIncrement ), conn );

    //create artist table
    query( QString( "CREATE %1 TABLE artist%2 ("
                    "id INTEGER PRIMARY KEY %3,"
                    "name " + textColumnType() + ");" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" )
                    .arg( artistAutoIncrement ), conn );

    //create genre table
    query( QString( "CREATE %1 TABLE genre%2 ("
                    "id INTEGER PRIMARY KEY %3,"
                    "name " + textColumnType() +");" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" )
                    .arg( genreAutoIncrement ), conn );

    //create year table
    query( QString( "CREATE %1 TABLE year%2 ("
                    "id INTEGER PRIMARY KEY %3,"
                    "name " + textColumnType() + ");" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" )
                    .arg( yearAutoIncrement ), conn );

    //create images table
    query( QString( "CREATE %1 TABLE images%2 ("
                    "path " + textColumnType() + ","
                    "artist " + textColumnType() + ","
                    "album " + textColumnType() + ");" )
                    .arg( conn ? "TEMPORARY" : "" )
                    .arg( conn ? "_temp" : "" ), conn );

    // create directory statistics table
    query( QString( "CREATE %1 TABLE directories%2 ("
                    "dir " + textColumnType() + " UNIQUE,"
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
                        "artist " + textColumnType() + ","
                        "suggestion " + textColumnType() + ","
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
    DEBUG_FUNC_INFO

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

    if ( m_dbConnPool->getDbConnectionType() == DbConnection::postgresql )
    {
        if (conn == NULL) {
            query( QString( "DROP SEQUENCE album_seq;" ), conn );
            query( QString( "DROP SEQUENCE artist_seq;" ), conn );
            query( QString( "DROP SEQUENCE genre_seq;" ), conn );
            query( QString( "DROP SEQUENCE year_seq;" ), conn );
	}
    }
}


void
CollectionDB::clearTables( DbConnection *conn )
{
    DEBUG_FUNC_INFO

    QString clearCommand = "DELETE FROM";
    if ( m_dbConnPool->getDbConnectionType() == DbConnection::mysql )
    {
        // TRUNCATE TABLE is faster than DELETE FROM TABLE, so use it when supported.
        clearCommand = "TRUNCATE TABLE";
    }

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
    insert( "INSERT INTO tags SELECT * FROM tags_temp;", NULL, conn );
    insert( "INSERT INTO album SELECT * FROM album_temp;", NULL, conn );
    insert( "INSERT INTO artist SELECT * FROM artist_temp;", NULL, conn );
    insert( "INSERT INTO genre SELECT * FROM genre_temp;", NULL, conn );
    insert( "INSERT INTO year SELECT * FROM year_temp;", NULL, conn );
    insert( "INSERT INTO images SELECT * FROM images_temp;", NULL, conn );
    insert( "INSERT INTO directories SELECT * FROM directories_temp;", NULL, conn );
}


void
CollectionDB::createStatsTable()
{
    DEBUG_FUNC_INFO

    // create music statistics database
    query( QString( "CREATE TABLE statistics ("
                    "url " + textColumnType() + " UNIQUE,"
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
    DEBUG_FUNC_INFO

    query( "DROP TABLE statistics;" );
}


uint
CollectionDB::artistID( QString value, bool autocreate, const bool temporary, const bool updateSpelling, DbConnection *conn )
{
    // lookup cache
    if ( m_cacheArtist == value )
        return m_cacheArtistID;

    uint id = IDFromValue( "artist", value, autocreate, temporary, updateSpelling, conn );

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
CollectionDB::albumID( QString value, bool autocreate, const bool temporary, const bool updateSpelling, DbConnection *conn )
{
    // lookup cache
    if ( m_cacheAlbum == value )
        return m_cacheAlbumID;

    uint id = IDFromValue( "album", value, autocreate, temporary, updateSpelling, conn );

    // cache values
    m_cacheAlbum = value;
    m_cacheAlbumID = id;

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
CollectionDB::genreID( QString value, bool autocreate, const bool temporary, const bool updateSpelling, DbConnection *conn )
{
    return IDFromValue( "genre", value, autocreate, temporary, updateSpelling, conn );
}


QString
CollectionDB::genreValue( uint id )
{
    return valueFromID( "genre", id );
}


uint
CollectionDB::yearID( QString value, bool autocreate, const bool temporary, const bool updateSpelling, DbConnection *conn )
{
    return IDFromValue( "year", value, autocreate, temporary, updateSpelling, conn );
}


QString
CollectionDB::yearValue( uint id )
{
    return valueFromID( "year", id );
}


uint
CollectionDB::IDFromValue( QString name, QString value, bool autocreate, const bool temporary, const bool updateSpelling, DbConnection *conn )
{
    if ( temporary )
        name.append( "_temp" );
    else
        conn = NULL;

    QStringList values =
        query( QString(
            "SELECT id, name FROM %1 WHERE name LIKE '%2';" )
            .arg( name )
            .arg( CollectionDB::instance()->escapeString( value ) ), conn );

    if ( updateSpelling && !values.isEmpty() && ( values[1] != value ) )
    {
        query( QString( "UPDATE %1 SET id = %2, name = '%3' WHERE id = %4;" )
                  .arg( name )
                  .arg( values.first() )
                  .arg( CollectionDB::instance()->escapeString( value ) )
                  .arg( values.first() ), conn );
    }

    //check if item exists. if not, should we autocreate it?
    uint id;
    if ( values.isEmpty() && autocreate )
    {
        id = insert( QString( "INSERT INTO %1 ( name ) VALUES ( '%2' );" )
                        .arg( name )
                        .arg( CollectionDB::instance()->escapeString( value ) ), name, conn );

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
CollectionDB::albumSongCount( const QString &artist_id, const QString &album_id )
{
    QStringList values =
        query( QString(
            "SELECT COUNT( url ) FROM tags WHERE album = %1 AND artist = %2;" )
            .arg( album_id )
            .arg( artist_id ) );
    return values.first();
}

bool
CollectionDB::albumIsCompilation( const QString &album_id )
{
    QStringList values =
        query( QString(
            "SELECT sampler FROM tags WHERE sampler=%1 AND album=%2" )
            .arg( CollectionDB::instance()->boolT() )
            .arg( album_id ) );

    return (values.count() != 0);
}

QStringList
CollectionDB::albumTracks( const QString &artist_id, const QString &album_id )
{
    if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql) {
        return query( QString( "SELECT tags.url, tags.track AS __discard FROM tags, year WHERE tags.album = %1 AND "
                               "( tags.sampler = %2 OR tags.artist = %3 ) AND year.id = tags.year "
                               "ORDER BY tags.track;" )
                      .arg( album_id )
                      .arg( boolT() )
                      .arg( artist_id ) );
    }
    else
    {
        return query( QString( "SELECT tags.url FROM tags, year WHERE tags.album = %1 AND "
                              "( tags.sampler = 1 OR tags.artist = %2 ) AND year.id = tags.year "
                              "ORDER BY tags.track;" )
                        .arg( album_id )
                        .arg( artist_id ) );
    }
}


void
CollectionDB::addImageToAlbum( const QString& image, QValueList< QPair<QString, QString> > info, DbConnection *conn )
{
    for ( QValueList< QPair<QString, QString> >::ConstIterator it = info.begin(); it != info.end(); ++it )
    {
        if ( (*it).first.isEmpty() || (*it).second.isEmpty() )
            continue;

        debug() << "Added image for album: " << (*it).first << " - " << (*it).second << ": " << image << endl;
        insert( QString( "INSERT INTO images%1 ( path, artist, album ) VALUES ( '%1', '%2', '%3' );" )
         .arg( conn ? "_temp" : "" )
         .arg( escapeString( image ) )
         .arg( escapeString( (*it).first ) )
         .arg( escapeString( (*it).second ) ), NULL, conn );
    }
}

QImage
CollectionDB::fetchImage(const KURL& url, QString &/*tmpFile*/)
{
    if(url.protocol() != "file")
    {
        QString tmpFile;
        KIO::NetAccess::download( url, tmpFile, 0); //TODO set 0 to the window, though it probably doesn't really matter
        return QImage(tmpFile);
    }
    else
    {
        return QImage( url.path() );
    }

}
bool
CollectionDB::setAlbumImage( const QString& artist, const QString& album, const KURL& url )
{
    QString tmpFile;
    bool success = setAlbumImage( artist, album, fetchImage(url, tmpFile) );
    KIO::NetAccess::removeTempFile( tmpFile ); //only removes file if it was created with NetAccess
    return success;
}


bool
CollectionDB::setAlbumImage( const QString& artist, const QString& album, QImage img, const QString& amazonUrl )
{
    debug() << "Saving cover for: " << artist << " - " << album << endl;

    //show a wait cursor for the duration
    amaroK::OverrideCursor keep;

    // remove existing album covers
    removeAlbumImage( artist, album );

    QDir largeCoverDir( amaroK::saveLocation( "albumcovers/large/" ) );
    QCString key = md5sum( artist, album );

    // Save Amazon product page URL as embedded string, for later retreival
    if ( !amazonUrl.isEmpty() )
        img.setText( "amazon-url", 0, amazonUrl );

    return img.save( largeCoverDir.filePath( key ), "PNG");
}


QString
CollectionDB::findImageByMetabundle( MetaBundle trackInformation, uint width )
{
    if( width == 1 ) width = AmarokConfig::coverPreviewSize();

    QCString widthKey = makeWidthKey( width );
    QCString tagKey = md5sum( trackInformation.artist(), trackInformation.album() );
    QDir tagCoverDir( amaroK::saveLocation( "albumcovers/tagcover/" ) );

    //FIXME: the cached versions will never be refreshed
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
                debug() << "Found APIC frame(s)" << endl;
                TagLib::ID3v2::Frame *f = l.front();
                TagLib::ID3v2::AttachedPictureFrame *ap = (TagLib::ID3v2::AttachedPictureFrame*)f;

                const TagLib::ByteVector &imgVector = ap->picture();
                debug() << "Size of image: " <<  imgVector.size() << " byte" << endl;

                // ignore APIC frames without picture and those with obviously bogus size
                if( imgVector.size() == 0 || imgVector.size() > 10000000 /*10MB*/ )
                    return QString();

                QImage image;
                if( image.loadFromData((const uchar*)imgVector.data(), imgVector.size()) )
                {
                    if ( width > 1 )
                    {
                        image.smoothScale( width, width, QImage::ScaleMin ).save( m_cacheDir.filePath( widthKey + tagKey ), "PNG" );
                        return m_cacheDir.filePath( widthKey + tagKey );
                    } else
                    {
                        image.save( tagCoverDir.filePath( tagKey ), "PNG" );
                        return tagCoverDir.filePath( tagKey );
                    }
                } // image.isNull
            } // apic list is empty
        } // tag is empty
    } // caching

    return QString();
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
            QDir largeCoverDir( amaroK::saveLocation( "albumcovers/large/" ) );
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
CollectionDB::albumImage( const QString &artist, const QString &album, uint width )
{
    QString s;
    // we aren't going to need a 1x1 size image. this is just a quick hack to be able to show full size images.
    if ( width == 1 ) width = AmarokConfig::coverPreviewSize();

    s = findImageByArtistAlbum( artist, album, width );
    if ( s == notAvailCover( width ) )
        return findImageByArtistAlbum( "", album, width );

    return s;
}


QString
CollectionDB::albumImage( const uint artist_id, const uint album_id, const uint width )
{
    return albumImage( artistValue( artist_id ), albumValue( album_id ), width );
}


QString
CollectionDB::albumImage( MetaBundle trackInformation, uint width )
{
    QString path = findImageByMetabundle( trackInformation, width );
    if( path.isEmpty() )
        path =albumImage( trackInformation.artist(), trackInformation.album(), width );

    return path;
}


QCString
CollectionDB::makeWidthKey( uint width )
{
    return QString::number( width ).local8Bit() + "@";
}

// get image from path
QString
CollectionDB::getImageForAlbum( const QString& artist, const QString& album, uint width )
{
    if ( width == 1 ) width = AmarokConfig::coverPreviewSize();
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

        if ( width > 1 )
        {
            if ( !m_cacheDir.exists( widthKey + key ) )
            {
                QImage img = QImage( image );
                img.smoothScale( width, width, QImage::ScaleMin ).save( m_cacheDir.filePath( widthKey + key ), "PNG" );
            }

            return m_cacheDir.filePath( widthKey + key );
        }
        else //large image
        {
            return image;
        }
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
    QDir largeCoverDir( amaroK::saveLocation( "albumcovers/large/" ) );

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
        qb.excludeMatch( QueryBuilder::tabArtist, i18n( "Unknown" ) );
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
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
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
        qb.excludeMatch( QueryBuilder::tabGenre, i18n( "Unknown" ) );
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
        qb.excludeMatch( QueryBuilder::tabYear, i18n( "Unknown" ) );
    if ( !withCompilations )
        qb.setOptions( QueryBuilder::optNoCompilations );

    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
    return qb.run();
}


QStringList
CollectionDB::albumListOfArtist( const QString &artist, bool withUnknown, bool withCompilations )
{
    if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql)
    {
        return query( "SELECT DISTINCT album.name, lower( album.name ) AS __discard FROM tags, album, artist WHERE "
                      "tags.album = album.id AND tags.artist = artist.id "
                      "AND artist.name = '" + escapeString( artist ) + "' " +
                      ( withUnknown ? QString::null : "AND album.name <> '' " ) +
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) +
                      " ORDER BY lower( album.name );" );
    }
    else
    {
        return query( "SELECT DISTINCT album.name FROM tags, album, artist WHERE "
                      "tags.album = album.id AND tags.artist = artist.id "
                      "AND artist.name = '" + escapeString( artist ) + "' " +
                      ( withUnknown ? QString::null : "AND album.name <> '' " ) +
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) +
                      " ORDER BY lower( album.name );" );
    }
}


QStringList
CollectionDB::artistAlbumList( bool withUnknown, bool withCompilations )
{
    if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql)
    {
        return query( "SELECT DISTINCT artist.name, album.name, lower( album.name ) AS __discard FROM tags, album, artist WHERE "
                      "tags.album = album.id AND tags.artist = artist.id " +
                      ( withUnknown ? QString::null : "AND album.name <> '' AND artist.name <> '' " ) +
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) +
                      " ORDER BY lower( album.name );" );
    }
    else
    {
        return query( "SELECT DISTINCT artist.name, album.name FROM tags, album, artist WHERE "
                      "tags.album = album.id AND tags.artist = artist.id " +
                      ( withUnknown ? QString::null : "AND album.name <> '' AND artist.name <> '' " ) +
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) +
                      " ORDER BY lower( album.name );" );
    }
}


bool
CollectionDB::addSong( MetaBundle* bundle, const bool incremental, DbConnection *conn )
{
    if ( !QFileInfo( bundle->url().path() ).isReadable() ) return false;

    QString command = "INSERT INTO tags_temp "
                      "( url, dir, createdate, album, artist, genre, year, title, comment, track, sampler, length, bitrate, samplerate ) "
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
    command += QString::number( QFileInfo( bundle->url().path() ).lastModified().toTime_t() ) + ",";

    command += escapeString( QString::number( albumID( bundle->album(),   true, !incremental, false, conn ) ) ) + ",";
    command += escapeString( QString::number( artistID( bundle->artist(), true, !incremental, false, conn ) ) ) + ",";
    command += escapeString( QString::number( genreID( bundle->genre(),   true, !incremental, false, conn ) ) ) + ",'";
    command += escapeString( QString::number( yearID( bundle->year(),     true, !incremental, false, conn ) ) ) + "','";

    command += escapeString( bundle->title() ) + "','";
    command += escapeString( bundle->comment() ) + "', ";
    command += ( bundle->track().isEmpty() ? "NULL" : escapeString( bundle->track() ) ) + " , ";
    command += artist == i18n( "Various Artists" ) ? boolT() + "," : boolF() + ",";

    // NOTE any of these may be -1 or -2, this is what we want
    //      see MetaBundle::Undetermined
    command += QString::number( bundle->length() ) + ",";
    command += QString::number( bundle->bitrate() ) + ",";
    command += QString::number( bundle->sampleRate() ) + ")";

    //FIXME: currently there's no way to check if an INSERT query failed or not - always return true atm.
    // Now it might be possible as insert returns the rowid.
    insert( command, NULL, conn );
    return true;
}


static void
fillInBundle( QStringList values, MetaBundle &bundle )
{
    //TODO use this whenever possible

    // crash prevention
    while( values.count() != 10 )
        values += "IF YOU CAN SEE THIS THERE IS A BUG!";

    QStringList::ConstIterator it = values.begin();

    bundle.setAlbum     ( *it ); ++it;
    bundle.setArtist    ( *it ); ++it;
    bundle.setGenre     ( *it ); ++it;
    bundle.setTitle     ( *it ); ++it;
    bundle.setYear      ( *it ); ++it;
    bundle.setComment   ( *it ); ++it;
    bundle.setTrack     ( *it ); ++it;
    bundle.setBitrate   ( (*it).toInt() ); ++it;
    bundle.setLength    ( (*it).toInt() ); ++it;
    bundle.setSampleRate( (*it).toInt() );
}

bool
CollectionDB::bundleForUrl( MetaBundle* bundle )
{
    QStringList values = query( QString(
            "SELECT album.name, artist.name, genre.name, tags.title, "
            "year.name, tags.comment, tags.track, tags.bitrate, tags.length, "
            "tags.samplerate "
            "FROM tags, album, artist, genre, year "
            "WHERE album.id = tags.album AND artist.id = tags.artist AND "
            "genre.id = tags.genre AND year.id = tags.year AND tags.url = '%1';" )
                .arg( escapeString( bundle->url().path() ) ) );

    if ( !values.empty() )
        fillInBundle( values, *bundle );

    return !values.isEmpty();
}


QValueList<MetaBundle>
CollectionDB::bundlesByUrls( const KURL::List& urls )
{
    typedef QValueList<MetaBundle> BundleList;
    BundleList bundles;
    QStringList paths;
    QueryBuilder qb;

    for( KURL::List::ConstIterator it = urls.begin(), end = urls.end(), last = urls.fromLast(); it != end; ++it )
    {
        // non file stuff won't exist in the db, but we still need to
        // re-insert it into the list we return, just with no tags assigned
        paths += (*it).protocol() == "file" ? (*it).path() : (*it).url();

        if( paths.count() == 50 || it == last )
        {
            qb.clear();

            qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBitrate );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valSamplerate );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            qb.addURLFilters( paths );
            qb.setOptions( QueryBuilder::optRemoveDuplicates );

            const QStringList values = qb.run();

            BundleList buns50;
            MetaBundle b;
            foreach( values )
            {
                b.setAlbum     (    *it );
                b.setArtist    (  *++it );
                b.setGenre     (  *++it );
                b.setTitle     (  *++it );
                b.setYear      (  *++it );
                b.setComment   (  *++it );
                b.setTrack     (  *++it );
                b.setBitrate   ( (*++it).toInt() );
                b.setLength    ( (*++it).toInt() );
                b.setSampleRate( (*++it).toInt() );
                b.setPath      (  *++it );

                buns50.append( b );
            }

            // we get no guarantee about the order that the database
            // will return our values, and sqlite indeed doesn't return
            // them in the desired order :( (MySQL does though)
            foreach( paths ) {
                for( BundleList::Iterator jt = buns50.begin(), end = buns50.end(); jt != end; ++jt )
                    if ( (*jt).url().path() == (*it) ) {
                        bundles += *jt;
                        buns50.remove( jt );
                        goto success;
                    }

                // if we get here, we didn't find an entry
                debug() << "No bundle recovered for: " << *it << endl;
                b = MetaBundle();
                b.setUrl( KURL::fromPathOrURL(*it) );
                bundles += b;

            success: ;
            }

            paths.clear();
        }
    }

    return bundles;
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
CollectionDB::addSongPercentage( const QString &url, int percentage )
{
    float score;
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, percentage FROM statistics "
            "WHERE url = '%1';" )
            .arg( escapeString( url ) ) );

    // check boundaries
    if ( percentage > 100 ) percentage = 100;
    if ( percentage < 1 )   percentage = 1;

    if ( !values.isEmpty() )
    {
        // entry exists, increment playcounter and update accesstime
        score = ( ( values[2].toDouble() * values.first().toInt() ) + percentage ) / ( values.first().toInt() + 1 );

        if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql) {
            query( QString( "UPDATE statistics SET percentage=%1, playcounter=%2+1 WHERE url='%3';" )
                            .arg( score )
                            .arg( values[0] + " + 1" )
                            .arg( escapeString( url ) ) );
        }
        else
        {
            query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                            "VALUES ( '%1', %2, %3, %4, %5 );" )
                            .arg( escapeString( url ) )
                            .arg( values[1] )
                            .arg( QDateTime::currentDateTime().toTime_t() )
                            .arg( score )
                            .arg( values[0] + " + 1" ) );
        }
    }
    else
    {
        // entry didn't exist yet, create a new one
        score = ( ( 50 + percentage ) / 2 );

        insert( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                        "VALUES ( '%1', %2, %3, %4, 1 );" )
                        .arg( escapeString( url ) )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( score ), NULL );
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
        if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql) {
            query( QString( "UPDATE statistics SET percentage=%1 WHERE url='%2';" )
                            .arg( percentage )
                            .arg( escapeString( url ) ) );
        }
        else
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
    }
    else
    {
        insert( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter ) "
                         "VALUES ( '%1', %2, %3, %4, 0 );" )
                        .arg( escapeString( url ) )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( percentage ), NULL );
    }

    emit scoreChanged( url, percentage );
}


void
CollectionDB::updateDirStats( QString path, const long datetime, DbConnection *conn )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql) {
        query( QString( "UPDATE directories%1 SET changedate=%2 WHERE dir='%3';")
                        .arg( conn ? "_temp" : "" )
                        .arg( datetime )
                        .arg( escapeString( path ) ), conn );
    }
    else
    {
        query( QString( "REPLACE INTO directories%1 ( dir, changedate ) VALUES ( '%3', %2 );" )
                  .arg( conn ? "_temp" : "" )
                  .arg( datetime )
                  .arg( escapeString( path ) ),
                  conn );
    }
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
        query( QString( "SELECT changedate FROM directories WHERE dir = '%1';" )
                  .arg( escapeString( path ) ) );

    return !values.isEmpty();
}


bool
CollectionDB::isFileInCollection( const QString &url  )
{
    QStringList values =
        query( QString( "SELECT url FROM tags WHERE url = '%1';" )
                  .arg( escapeString( url ) ) );

    return !values.isEmpty();
}


void
CollectionDB::removeSongs( const KURL::List& urls )
{
    for( KURL::List::ConstIterator it = urls.begin(), end = urls.end(); it != end; ++it )
    {
        query( QString( "DELETE FROM tags WHERE url = '%1';" )
            .arg( escapeString( (*it).path() ) ) );
    }
}


QStringList
CollectionDB::similarArtists( const QString &artist, uint count )
{
    QStringList values;

    if (m_dbConnPool->getDbConnectionType() == DbConnection::postgresql) {
        values = query( QString( "SELECT suggestion FROM related_artists WHERE artist = '%1' OFFSET 0 LIMIT %2;" )
                                 .arg( escapeString( artist ) ).arg( count ) );
    }
    else
    {
        values = query( QString( "SELECT suggestion FROM related_artists WHERE artist = '%1' LIMIT 0, %2;" )
                                 .arg( escapeString( artist ) ).arg( count ) );
    }

    if ( values.isEmpty() )
        Scrobbler::instance()->similarArtists( artist );

    return values;
}


void
CollectionDB::checkCompilations( const QString &path, const bool temporary, DbConnection *conn )
{
    QStringList albums;
    QStringList artists;
    QStringList dirs;

    albums = query( QString( "SELECT DISTINCT album.name FROM tags_temp, album%1 AS album WHERE tags_temp.dir = '%2' AND album.id = tags_temp.album;" )
              .arg( temporary ? "_temp" : "" )
              .arg( escapeString( path ) ), conn );

    for ( uint i = 0; i < albums.count(); i++ )
    {
        if ( albums[ i ].isEmpty() ) continue;

        const uint album_id = albumID( albums[ i ], false, temporary, false, conn );
        artists = query( QString( "SELECT DISTINCT artist.name FROM tags_temp, artist%1 AS artist WHERE tags_temp.album = '%2' AND tags_temp.artist = artist.id;" )
                            .arg( temporary ? "_temp" : "" )
                            .arg( album_id ), conn );
        dirs    = query( QString( "SELECT DISTINCT dir FROM tags_temp WHERE album = '%1';" )
                            .arg( album_id ), conn );

        if ( artists.count() > dirs.count() )
        {
            debug() << "Detected compilation: " << albums[ i ] << " - " << artists.count() << ":" << dirs.count() << endl;
            query( QString( "UPDATE tags_temp SET sampler = %1 WHERE album = '%2';" )
                            .arg(boolT()).arg( album_id ), conn );
        }
    }
}


void
CollectionDB::setCompilation( const QString &album, const bool enabled, const bool updateView )
{
    query( QString( "UPDATE tags, album SET tags.sampler = %1 WHERE tags.album = album.id AND album.name = '%2';" )
              .arg( enabled ? "1" : "0" )
              .arg( escapeString( album ) ) );

    // Update the Collection-Browser view,
    // using QTimer to make sure we don't manipulate the GUI from a thread
    if ( updateView )
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
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
    command += "artist = " + QString::number( artistID( bundle.artist(), true, false, true ) ) + ", ";
    command += "album = "  + QString::number( albumID( bundle.album(), true, false, true ) ) + ", ";
    command += "genre = "  + QString::number( genreID( bundle.genre(), true, false, true ) ) + ", ";
    command += "year = "   + QString::number( yearID( bundle.year(), true, false, true ) ) + ", ";
    if ( !bundle.track().isEmpty() )
        command += "track = " + bundle.track() + ", ";
    command += "comment = '" + escapeString( bundle.comment() ) + "' ";
    command += "WHERE url = '" + escapeString( url ) + "';";

    query( command );

    if ( EngineController::instance()->bundle().url() == bundle.url() )
    {
        debug() << "Current song edited, updating widgets: " << bundle.title() << endl;
        EngineController::instance()->currentTrackMetaDataChanged( bundle );
    }

    // Update the Collection-Browser view,
    // using QTimer to make sure we don't manipulate the GUI from a thread
    if ( updateView )
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
}


void
CollectionDB::updateURL( const QString &url, const bool updateView )
{
    // don't use the KURL ctor as it checks the db first
    MetaBundle bundle;
    bundle.setPath( url );
    bundle.readTags( TagLib::AudioProperties::Fast );

    updateTags( url, bundle, updateView );
}


void
CollectionDB::applySettings()
{
    bool recreateConnections = false;
    if ( AmarokConfig::databaseEngine().toInt() != m_dbConnPool->getDbConnectionType() )
    {
        recreateConnections = true;
    }
    else if ( AmarokConfig::databaseEngine().toInt() == DbConnection::mysql )
    {
        // Using MySQL, so check if MySQL settings were changed
        const MySqlConfig *config =
            static_cast<const MySqlConfig*> ( m_dbConnPool->getDbConfig() );
        if ( AmarokConfig::mySqlHost() != config->host() )
        {
            recreateConnections = true;
        }
        else if ( AmarokConfig::mySqlPort() != config->port() )
        {
            recreateConnections = true;
        }
        else if ( AmarokConfig::mySqlDbName() != config->database() )
        {
            recreateConnections = true;
        }
        else if ( AmarokConfig::mySqlUser() != config->username() )
        {
            recreateConnections = true;
        }
        else if ( AmarokConfig::mySqlPassword() != config->password() )
        {
            recreateConnections = true;
        }
    }
    else if ( AmarokConfig::databaseEngine().toInt() == DbConnection::postgresql )
    {
      const PostgresqlConfig *config =
          static_cast<const PostgresqlConfig*> ( m_dbConnPool->getDbConfig() );
      if ( AmarokConfig::postgresqlConninfo() != config->conninfo() )
      {
          recreateConnections = true;
      }
    }
    if ( recreateConnections )
    {
        debug()
            << "Database engine settings changed: "
            << "recreating DbConnections" << endl;
        // If Database engine was changed, recreate DbConnections.
        destroy();
        initialize();
        CollectionView::instance()->renderView();
        emit databaseEngineChanged();
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

QCString
CollectionDB::md5sum( const QString& artist, const QString& album, const QString& file )
{
    KMD5 context( artist.lower().local8Bit() + album.lower().local8Bit() + file.local8Bit() );
//     debug() << "MD5 SUM for " << artist << ", " << album << ": " << context.hexDigest() << endl;
    return context.hexDigest();
}


void CollectionDB::engineTrackEnded( int finalPosition, int trackLength )
{
    //This is where percentages are calculated
    //TODO statistics are not calculated when currentTrack doesn't exist

    // Don't update statistics if song has been played for less than 15 seconds
    // if ( finalPosition < 15000 ) return;

    const KURL url = EngineController::instance()->bundle().url();
    if ( url.path().isEmpty() ) return;

    // sanity check
    if ( finalPosition > trackLength || finalPosition <= 0 )
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
    debug() << "Fetching cover for " << artist << " - " << album << endl;

    CoverFetcher* fetcher = new CoverFetcher( parent, artist, album );
    connect( fetcher, SIGNAL(result( CoverFetcher* )), SLOT(coverFetcherResult( CoverFetcher* )) );
    fetcher->setUserCanEditQuery( !noedit );
    fetcher->startFetch();
    #endif
}


void
CollectionDB::scanMonitor()  //SLOT
{
    scanModifiedDirs();
}


void
CollectionDB::startScan()  //SLOT
{
    QStringList folders = MountPointManager::instance()->collectionFolders();

    if ( folders.isEmpty() ) {
        dropTables();
        createTables();
    }
    else if( PlaylistBrowser::instance() )
    {
        emit scanStarted();

        ThreadWeaver::instance()->queueJob( new CollectionReader( this, folders ) );
    }
}


void
CollectionDB::stopScan() //SLOT
{
    ThreadWeaver::instance()->abortAllJobsNamed( "CollectionReader" );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::dirDirty( const QString& path )
{
    debug() << k_funcinfo << "Dirty: " << path << endl;

    ThreadWeaver::instance()->queueJob( new CollectionReader( this, path ) );
}


void
CollectionDB::coverFetcherResult( CoverFetcher *fetcher )
{
    if( fetcher->wasError() ) {
        error() << fetcher->errors() << endl;
        emit coverFetcherError( fetcher->errors().front() );
    }

    else {
        setAlbumImage( fetcher->artist(), fetcher->album(), fetcher->image(), fetcher->amazonURL() );
        emit coverFetched( fetcher->artist(), fetcher->album() );
    }
}

/**
 * This query is fairly slow with sqlite, and often happens just
 * after the OSD is shown. Threading it restores responsivity.
 */
class SimilarArtistsInsertionJob : public ThreadWeaver::DependentJob
{
    virtual bool doJob()
    {
        CollectionDB::instance()->query( QString( "DELETE FROM related_artists WHERE artist = '%1';" ).arg( escapedArtist ) );

        const QString sql = "INSERT INTO related_artists ( artist, suggestion, changedate ) VALUES ( '%1', '%2', 0 );";
        foreach( suggestions )
            CollectionDB::instance()->insert( sql
                    .arg( escapedArtist )
                    .arg( CollectionDB::instance()->escapeString( *it ) ), NULL );

        return true;
    }

    virtual void completeJob() { emit CollectionDB::instance()->similarArtistsFetched( artist ); }

    const QString artist;
    const QString escapedArtist;
    const QStringList suggestions;

public:
    SimilarArtistsInsertionJob( CollectionDB *parent, const QString &s, const QStringList &list )
            : ThreadWeaver::DependentJob( parent, "SimilarArtistsInsertionJob" )
            , artist( s )
            , escapedArtist( parent->escapeString( s ) )
            , suggestions( list )
    {}
};

void
CollectionDB::similarArtistsFetched( const QString& artist, const QStringList& suggestions )
{
    debug() << "Received similar artists\n";

    ThreadWeaver::instance()->queueJob( new SimilarArtistsInsertionJob( this, artist, suggestions ) );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////


void
CollectionDB::initialize()
{
    m_dbConnPool = new DbConnectionPool();
    DbConnection *dbConn = m_dbConnPool->getDbConnection();
    m_dbConnPool->putDbConnection( dbConn );

    KConfig* config = amaroK::config( "Collection Browser" );
    if(!dbConn->isConnected())
        amaroK::MessageQueue::instance()->addMessage(dbConn->lastError());
    if ( !dbConn->isInitialized() || !isValid() )
    {
        createTables();
        createStatsTable();
    }
    else
    {
        //remove database file if version is incompatible
        if ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION )
        {
            debug() << "Rebuilding database!" << endl;
            dropTables();
            createTables();
        }
        if ( config->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION )
        {
            debug() << "Rebuilding stats-database!" << endl;
            dropStatsTable();
            createStatsTable();
        }
    }

    m_dbConnPool->createDbConnections();
}


void
CollectionDB::destroy()
{
    delete m_dbConnPool;
}


void
CollectionDB::scanModifiedDirs()
{
    //we check if a job is pending because we don't want to abort incremental collection readings
    if ( !ThreadWeaver::instance()->isJobPending( "CollectionReader" ) && PlaylistBrowser::instance() ) {
        emit scanStarted();
        ThreadWeaver::instance()->onlyOneJob( new IncrementalCollectionReader( this ) );
    }
}


void
CollectionDB::customEvent( QCustomEvent *e )
{
    if ( e->type() == (int)CollectionReader::JobFinishedEvent )
        emit scanDone( static_cast<ThreadWeaver::Job*>(e)->wasSuccessful() );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS DbConnectionPool
//////////////////////////////////////////////////////////////////////////////////////////

DbConnectionPool::DbConnectionPool() : m_semaphore( POOL_SIZE )
{
#ifdef USE_MYSQL
    if ( AmarokConfig::databaseEngine().toInt() == DbConnection::mysql )
        m_dbConnType = DbConnection::mysql;
    else
#endif
#ifdef USE_POSTGRESQL
    if ( AmarokConfig::databaseEngine().toInt() == DbConnection::postgresql )
        m_dbConnType = DbConnection::postgresql;
    else
#endif
        m_dbConnType = DbConnection::sqlite;

    m_semaphore += POOL_SIZE;
    DbConnection *dbConn;

#ifdef USE_MYSQL
    if ( m_dbConnType == DbConnection::mysql )
    {
        m_dbConfig =
            new MySqlConfig(
                AmarokConfig::mySqlHost(),
                AmarokConfig::mySqlPort(),
                AmarokConfig::mySqlDbName(),
                AmarokConfig::mySqlUser(),
                AmarokConfig::mySqlPassword() );
        dbConn = new MySqlConnection( static_cast<MySqlConfig*> ( m_dbConfig ) );
    }
    else
#endif
#ifdef USE_POSTGRESQL
    if ( m_dbConnType == DbConnection::postgresql )
    {
        m_dbConfig =
            new PostgresqlConfig(
                AmarokConfig::postgresqlConninfo() );
        dbConn = new PostgresqlConnection( static_cast<PostgresqlConfig*> ( m_dbConfig ) );
    }
    else
#endif
    {
        m_dbConfig = new SqliteConfig( "collection.db" );
        dbConn = new SqliteConnection( static_cast<SqliteConfig*> ( m_dbConfig ) );
    }
    enqueue( dbConn );
    m_semaphore--;
    debug() << "Available db connections: " << m_semaphore.available() << endl;
}


DbConnectionPool::~DbConnectionPool()
{
    m_semaphore += POOL_SIZE;
    DbConnection *conn;
    bool vacuum = true;

    while ( ( conn = dequeue() ) != 0 )
    {
        if ( m_dbConnType == DbConnection::sqlite && vacuum )
        {
            vacuum = false;
            debug() << "Running VACUUM" << endl;
            conn->query( "VACUUM; ");
        }

        delete conn;
    }

    delete m_dbConfig;
}


void DbConnectionPool::createDbConnections()
{
    for ( int i = 0; i < POOL_SIZE - 1; i++ )
    {
        DbConnection *dbConn;

#ifdef USE_MYSQL
        if ( m_dbConnType == DbConnection::mysql )
            dbConn = new MySqlConnection( static_cast<MySqlConfig*> ( m_dbConfig ) );
        else
#endif
#ifdef USE_POSTGRESQL
        if ( m_dbConnType == DbConnection::postgresql )
            dbConn = new PostgresqlConnection( static_cast<PostgresqlConfig*> ( m_dbConfig ) );
        else
#endif
            dbConn = new SqliteConnection( static_cast<SqliteConfig*> ( m_dbConfig ) );
        enqueue( dbConn );
        m_semaphore--;
    }
    debug() << "Available db connections: " << m_semaphore.available() << endl;
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


#include "collectiondb.moc"
