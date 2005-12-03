// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// (c) 2005 Ian Monroe <ian@monroe.nu>
// (c) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>
// See COPYING file for licensing information.

#define DEBUG_PREFIX "CollectionDB"

#include "app.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "config.h"
#include "debug.h"
#include "collectionbrowser.h"    //updateTags()
#include "collectiondb.h"
#include "coverfetcher.h"
#include "enginecontroller.h"
#include "metabundle.h"           //updateTags()
#include "playlist.h"
#include "playlistbrowser.h"
#include "scancontroller.h"
#include "scrobbler.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <qfile.h>
#include <qmap.h>
#include <qmutex.h>
#include <qregexp.h>              //setHTMLLyrics()
#include <qtimer.h>

#include <pthread.h>              //debugging, can be removed later

#include <kapplication.h>
#include <kcharsets.h>            //setHTMLLyrics()
#include <kconfig.h>
#include <kglobal.h>
#include <kinputdialog.h>         //setupCoverFetcher()
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
#include <taglib/flacfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/tbytevector.h>

#include "sqlite/sqlite3.h"

#ifdef USE_MYSQL
#include <mysql/mysql.h>
#endif

#ifdef USE_POSTGRESQL
#include <libpq-fe.h>
#endif

#define DEBUG 0

QMutex* CollectionDB::connectionMutex = new QMutex();
//we don't have to worry about this map leaking memory since ThreadWeaver limits the total
//number of QThreads ever created
QMap<QThread *, DbConnection *> *CollectionDB::threadConnections = new QMap<QThread *, DbConnection *>();

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
        , m_noCover ( locate( "data", "amarok/images/nocover.png" ) )
{
    DEBUG_BLOCK

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


    //<OPEN DATABASE>
    initialize();
    //</OPEN DATABASE>


    // Remove cached "nocover" images, so that a new version actually gets shown
    const QStringList entryList = cacheCoverDir().entryList( "*nocover.png", QDir::Files );
    foreach( entryList )
        cacheCoverDir().remove( *it );


    // TODO Should write to config in dtor, but it crashes...
    KConfig* config = amaroK::config( "Collection Browser" );
    config->writeEntry( "Database Version", DATABASE_VERSION );
    config->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );
    config->writeEntry( "Database Persistent Tables Version", DATABASE_PERSISTENT_TABLES_VERSION );

    setAdminValue( "Database Version", QString::number(DATABASE_VERSION) );
    setAdminValue( "Database Stats Version", QString::number(DATABASE_STATS_VERSION) );
    setAdminValue( "Database Persistent Tables Version", QString::number(DATABASE_PERSISTENT_TABLES_VERSION) );


    startTimer( MONITOR_INTERVAL * 1000 );
    connect( this, SIGNAL( coverRemoved( const QString&, const QString& ) ),
                   SIGNAL( coverChanged( const QString&, const QString& ) ) );
    connect( Scrobbler::instance(), SIGNAL( similarArtistsFetched( const QString&, const QStringList& ) ),
             this,                    SLOT( similarArtistsFetched( const QString&, const QStringList& ) ) );
}

CollectionDB::~CollectionDB()
{
    DEBUG_BLOCK
    if (getDbConnectionType() == DbConnection::sqlite) {
        debug() << "Running VACUUM" << endl;
        query( "VACUUM; ");
    }
    destroy();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////


/**
 * Executes a SQL query on the already opened database
 * @param statement SQL program to execute. Only one SQL statement is allowed.
 * @return          The queried data, or QStringList() on error.
 */
QStringList
CollectionDB::query( const QString& statement )
{
    clock_t start;
    if ( DEBUG )
    {
        debug() << "Query-start: " << statement << endl;
        start = clock();
    }

    DbConnection *dbConn;
    dbConn = getMyConnection();

    QStringList values = dbConn->query( statement );

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
CollectionDB::insert( const QString& statement, const QString& table )
{
    clock_t start;
    if ( DEBUG )
    {
        debug() << "insert-start: " << statement << endl;
        start = clock();
    }

    DbConnection *dbConn;
    dbConn = getMyConnection();

    int id = dbConn->insert( statement, table );

    if ( DEBUG )
    {
        clock_t finish = clock();
        const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
        debug() << "SQL-insert (" << duration << "s): " << statement << endl;
    }
    return id;
}

bool
CollectionDB::isEmpty( )
{
    QStringList values;

    if (getDbConnectionType() == DbConnection::postgresql)
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
CollectionDB::isValid( )
{
    QStringList values1;
    QStringList values2;

    if (getDbConnectionType() == DbConnection::postgresql) {
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


QString
CollectionDB::adminValue( QString noption ) {
    QStringList values;
    values = query (
        QString( "SELECT value FROM admin WHERE noption = '%1';").arg(noption)
    );
    return values.isEmpty() ? "" : values.first();
}


void
CollectionDB::setAdminValue( QString noption, QString value ) {

    QStringList values = query( QString( "SELECT value FROM admin WHERE noption = '%1';").arg( noption ));
    if(values.count() > 0)
    {
        query( QString( "UPDATE admin SET value = '%1' WHERE noption = '%2';" ).arg( value, noption ) );
    }
    else
    {
        insert( QString( "INSERT INTO admin (value, noption) values ( '%1', '%2' );" ).arg( value, noption ),
         NULL );
    }
}



void
CollectionDB::createTables( const bool temporary )
{
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
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    QString albumAutoIncrement = "";
    QString artistAutoIncrement = "";
    QString genreAutoIncrement = "";
    QString yearAutoIncrement = "";
    if ( getDbConnectionType() == DbConnection::postgresql )
    {
    if(!temporary)
    {
        query( QString( "CREATE SEQUENCE album_seq;" ) );
        query( QString( "CREATE SEQUENCE artist_seq;" ) );
        query( QString( "CREATE SEQUENCE genre_seq;" ) );
        query( QString( "CREATE SEQUENCE year_seq;" ) );
    }

        albumAutoIncrement = QString("DEFAULT nextval('album_seq')");
        artistAutoIncrement = QString("DEFAULT nextval('artist_seq')");
        genreAutoIncrement = QString("DEFAULT nextval('genre_seq')");
        yearAutoIncrement = QString("DEFAULT nextval('year_seq')");
    }
    else if ( getDbConnectionType() == DbConnection::mysql )
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
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" )
                    .arg( albumAutoIncrement ) );

    //create artist table
    query( QString( "CREATE %1 TABLE artist%2 ("
                    "id INTEGER PRIMARY KEY %3,"
                    "name " + textColumnType() + ");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" )
                    .arg( artistAutoIncrement ) );

    //create genre table
    query( QString( "CREATE %1 TABLE genre%2 ("
                    "id INTEGER PRIMARY KEY %3,"
                    "name " + textColumnType() +");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" )
                    .arg( genreAutoIncrement ) );

    //create year table
    query( QString( "CREATE %1 TABLE year%2 ("
                    "id INTEGER PRIMARY KEY %3,"
                    "name " + textColumnType() + ");" )
                    .arg( temporary? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" )
                    .arg( yearAutoIncrement ) );

    //create images table
    query( QString( "CREATE %1 TABLE images%2 ("
                    "path " + textColumnType() + ","
                    "artist " + textColumnType() + ","
                    "album " + textColumnType() + ");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    // create directory statistics table
    query( QString( "CREATE %1 TABLE directories%2 ("
                    "dir " + textColumnType() + " UNIQUE,"
                    "changedate INTEGER );" )
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

        //create admin table -- holds the db version, put here other stuff if necessary
        query( QString( "CREATE TABLE admin ("
                    "noption " + textColumnType() + ", "
                    "value " + textColumnType() + ");" ) );

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
CollectionDB::dropTables( const bool temporary )
{
    query( QString( "DROP TABLE tags%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE album%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE artist%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE genre%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE year%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE images%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE directories%1;" ).arg( temporary ? "_temp" : "" ) );
    if ( !temporary )
    {
        query( QString( "DROP TABLE related_artists;" ) );
    }

    if ( getDbConnectionType() == DbConnection::postgresql )
    {
    if (temporary == false) {
        query( QString( "DROP SEQUENCE album_seq;" ) );
        query( QString( "DROP SEQUENCE artist_seq;" ) );
        query( QString( "DROP SEQUENCE genre_seq;" ) );
        query( QString( "DROP SEQUENCE year_seq;" ) );
    }
    }
}


void
CollectionDB::clearTables( const bool temporary )
{
    QString clearCommand = "DELETE FROM";
    if ( getDbConnectionType() == DbConnection::mysql || getDbConnectionType() == DbConnection::postgresql)
    {
        // TRUNCATE TABLE is faster than DELETE FROM TABLE, so use it when supported.
        clearCommand = "TRUNCATE TABLE";
    }

    query( QString( "%1 tags%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 album%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 artist%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 genre%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 year%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 images%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 directories%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    if ( !temporary )
    {
        query( QString( "%1 related_artists;" ).arg( clearCommand ) );
    }
}


void
CollectionDB::moveTempTables( )
{
    insert( "INSERT INTO tags SELECT * FROM tags_temp;", NULL );
    insert( "INSERT INTO album SELECT * FROM album_temp;", NULL );
    insert( "INSERT INTO artist SELECT * FROM artist_temp;", NULL );
    insert( "INSERT INTO genre SELECT * FROM genre_temp;", NULL );
    insert( "INSERT INTO year SELECT * FROM year_temp;", NULL );
    insert( "INSERT INTO images SELECT * FROM images_temp;", NULL );
    insert( "INSERT INTO directories SELECT * FROM directories_temp;", NULL );
}


void
CollectionDB::createStatsTable()
{
    // create music statistics database
    query( QString( "CREATE TABLE statistics ("
                    "url " + textColumnType() + " UNIQUE,"
                    "createdate INTEGER,"
                    "accessdate INTEGER,"
                    "percentage FLOAT,"
                    "rating INTEGER DEFAULT 0,"
                    "playcounter INTEGER );" ) );

    query( "CREATE INDEX url_stats ON statistics( url );" );
    query( "CREATE INDEX percentage_stats ON statistics( percentage );" );
    query( "CREATE INDEX rating_stats ON statistics( rating );" );
    query( "CREATE INDEX playcounter_stats ON statistics( playcounter );" );
}


void
CollectionDB::dropStatsTable()
{
    query( "DROP TABLE statistics;" );
}


void
CollectionDB::createPersistentTables()
{
    // create amazon table
    query(          "CREATE TABLE amazon ( "
            "asin " + textColumnType(20) + ", "
            "locale " + textColumnType(2) + ", "
            "filename " + textColumnType(33) + ", "
            "refetchdate INTEGER );" );

    // create lyrics table
    query( QString( "CREATE TABLE lyrics ("
            "url " + textColumnType() + ","
            "lyrics " + longTextColumnType() + ");" ) );

    // create labels table
    query( QString( "CREATE TABLE label ("
        "url " + textColumnType() + ","
        "label " + textColumnType() + ");" ) );

    query( "CREATE INDEX url_label ON label( url );" );
    query( "CREATE INDEX label_label ON label( label );" );

}


void
CollectionDB::dropPersistentTables()
{
    query( "DROP TABLE amazon;" );
    query( "DROP TABLE lyrics;" );
    query( "DROP TABLE labels;" );
}

uint
CollectionDB::artistID( QString value, bool autocreate, const bool temporary, const bool updateSpelling )
{
    // lookup cache
    if ( m_cacheArtist == value )
        return m_cacheArtistID;

    uint id = IDFromValue( "artist", value, autocreate, temporary, updateSpelling );

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
CollectionDB::albumID( QString value, bool autocreate, const bool temporary, const bool updateSpelling )
{
    // lookup cache
    if ( m_cacheAlbum == value )
        return m_cacheAlbumID;

    uint id = IDFromValue( "album", value, autocreate, temporary, updateSpelling );

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
CollectionDB::genreID( QString value, bool autocreate, const bool temporary, const bool updateSpelling)
{
    return IDFromValue( "genre", value, autocreate, temporary, updateSpelling);
}


QString
CollectionDB::genreValue( uint id )
{
    return valueFromID( "genre", id );
}


uint
CollectionDB::yearID( QString value, bool autocreate, const bool temporary, const bool updateSpelling)
{
    return IDFromValue( "year", value, autocreate, temporary, updateSpelling);
}


QString
CollectionDB::yearValue( uint id )
{
    return valueFromID( "year", id );
}


uint
CollectionDB::IDFromValue( QString name, QString value, bool autocreate, const bool temporary, const bool updateSpelling )
{
    if ( temporary )
        name.append( "_temp" );
//    what the hell is the reason for this?
//    else
//        conn = NULL;

    QStringList values =
        query( QString(
            "SELECT id, name FROM %1 WHERE name LIKE '%2';" )
            .arg( name )
            .arg( CollectionDB::instance()->escapeString( value ) ) );

    if ( updateSpelling && !values.isEmpty() && ( values[1] != value ) )
    {
        query( QString( "UPDATE %1 SET id = %2, name = '%3' WHERE id = %4;" )
                  .arg( name )
                  .arg( values.first() )
                  .arg( CollectionDB::instance()->escapeString( value ) )
                  .arg( values.first() ) );
    }

    //check if item exists. if not, should we autocreate it?
    uint id;
    if ( values.isEmpty() && autocreate )
    {
        id = insert( QString( "INSERT INTO %1 ( name ) VALUES ( '%2' );" )
                        .arg( name )
                        .arg( CollectionDB::instance()->escapeString( value ) ), name );

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
CollectionDB::albumTracks( const QString &artist_id, const QString &album_id, const bool isValue )
{
    if ( isValue)
    {
        return query( QString( "SELECT tags.url FROM tags INNER JOIN artist ON artist.id=tags.artist INNER JOIN album ON "
                        "album.id=tags.album WHERE (album.name = \"%1\" ) AND (artist.name = \"%2\" ) ORDER BY tags.track;" )
                        .arg( album_id )
                        .arg( artist_id ) );
    }

    if (getDbConnectionType() == DbConnection::postgresql) {
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
CollectionDB::addImageToAlbum( const QString& image, QValueList< QPair<QString, QString> > info, const bool temporary )
{
    for ( QValueList< QPair<QString, QString> >::ConstIterator it = info.begin(); it != info.end(); ++it )
    {
        if ( (*it).first.isEmpty() || (*it).second.isEmpty() )
            continue;

        debug() << "Added image for album: " << (*it).first << " - " << (*it).second << ": " << image << endl;
        insert( QString( "INSERT INTO images%1 ( path, artist, album ) VALUES ( '%1', '%2', '%3' );" )
         .arg( temporary ? "_temp" : "" )
         .arg( escapeString( image ) )
         .arg( escapeString( (*it).first ) )
         .arg( escapeString( (*it).second ) ), NULL );
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
CollectionDB::setAlbumImage( const QString& artist, const QString& album, QImage img, const QString& amazonUrl, const QString& asin )
{
    debug() << "Saving cover for: " << artist << " - " << album << endl;

    //show a wait cursor for the duration
    amaroK::OverrideCursor keep;

    // remove existing album covers
    removeAlbumImage( artist, album );

    QCString key = md5sum( artist, album );
    newAmazonReloadDate(asin, AmarokConfig::amazonLocale(), key);
    // Save Amazon product page URL as embedded string, for later retreival
    if ( !amazonUrl.isEmpty() )
        img.setText( "amazon-url", 0, amazonUrl );

    const bool b = img.save( largeCoverDir().filePath( key ), "PNG");
    emit coverChanged( artist, album );
    return b;
}


QString
CollectionDB::findImageByMetabundle( MetaBundle trackInformation, uint width )
{
    if( width == 1 ) width = AmarokConfig::coverPreviewSize();

    QCString widthKey = makeWidthKey( width );
    QCString tagKey = md5sum( trackInformation.artist(), trackInformation.album() );

    //FIXME the cached versions will never be refreshed
    //FIXME This code is NONSENSE. There are no cached images in tagCoverDir
    if ( tagCoverDir().exists( widthKey + tagKey ) )
    {
        // cached version
        return tagCoverDir().filePath( widthKey + tagKey );
    }
    else
    {
        TagLib::File *f = 0;
        TagLib::ID3v2::Tag *tag = 0;
        if ( trackInformation.url().path().endsWith( ".mp3", false ) ) {
            f = new TagLib::MPEG::File( QFile::encodeName( trackInformation.url().path() ) );
            tag = ( (TagLib::MPEG::File*)f )->ID3v2Tag();
        }
        else if ( trackInformation.url().path().endsWith( ".flac", false ) ) {
            f = new TagLib::FLAC::File( QFile::encodeName( trackInformation.url().path() ) );
            tag = ( (TagLib::FLAC::File*)f )->ID3v2Tag();
        }
        if ( tag )
        {
            TagLib::ID3v2::FrameList l = tag->frameListMap()[ "APIC" ];
            if ( !l.isEmpty() )
            {
                debug() << "Found APIC frame(s)" << endl;
                TagLib::ID3v2::Frame *f = l.front();
                TagLib::ID3v2::AttachedPictureFrame *ap = (TagLib::ID3v2::AttachedPictureFrame*)f;

                const TagLib::ByteVector &imgVector = ap->picture();
                debug() << "Size of image: " <<  imgVector.size() << " byte" << endl;

                // ignore APIC frames without picture and those with obviously bogus size
                if( imgVector.size() == 0 || imgVector.size() > 10000000 /*10MB*/ ) {
                    delete f;
                    return QString::null;
                }

                QImage image;
                if( image.loadFromData((const uchar*)imgVector.data(), imgVector.size()) )
                {
                    if ( width > 1 )
                    {
                        image.smoothScale( width, width, QImage::ScaleMin ).save( cacheCoverDir().filePath( widthKey + tagKey ), "PNG" );
                        delete f;
                        return cacheCoverDir().filePath( widthKey + tagKey );
                    } else
                    {
                        image.save( tagCoverDir().filePath( tagKey ), "PNG" );
                        delete f;
                        return tagCoverDir().filePath( tagKey );
                    }
                } // image.isNull
            } // apic list is empty
        } // tag is empty
        if ( f )
            delete f; // destroying f will destroy the tag it generated, according to taglib docs
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
        if ( cacheCoverDir().exists( widthKey + key ) )
            return cacheCoverDir().filePath( widthKey + key );
        else
        {
            // we need to create a scaled version of this cover
            QDir imageDir = largeCoverDir().exists( key ) ? largeCoverDir() : tagCoverDir();

            if ( imageDir.exists( key ) )
                if ( width > 1 )
                {
                    QImage img( imageDir.filePath( key ) );
                    img.smoothScale( width, width, QImage::ScaleMin ).save( cacheCoverDir().filePath( widthKey + key ), "PNG" );

                    return cacheCoverDir().filePath( widthKey + key );
                }
                else
                    return imageDir.filePath( key );
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
            if ( !cacheCoverDir().exists( widthKey + key ) )
            {
                QImage img = QImage( image );
                img.smoothScale( width, width, QImage::ScaleMin ).save( cacheCoverDir().filePath( widthKey + key ), "PNG" );
            }

            return cacheCoverDir().filePath( widthKey + key );
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
    query("DELETE FROM amazon WHERE filename='" + key + "'");

    // remove scaled versions of images
    QStringList scaledList = cacheCoverDir().entryList( widthKey + key );
    if ( scaledList.count() > 0 )
        for ( uint i = 0; i < scaledList.count(); i++ )
            QFile::remove( cacheCoverDir().filePath( scaledList[ i ] ) );

    // remove large, original image
    if ( largeCoverDir().exists( key ) && QFile::remove( largeCoverDir().filePath( key ) ) ) {
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

    if( cacheCoverDir().exists( widthKey + "nocover.png" ) )
        return cacheCoverDir().filePath( widthKey + "nocover.png" );
    else
    {
        m_noCover.smoothScale( width, width, QImage::ScaleMin ).save( cacheCoverDir().filePath( widthKey + "nocover.png" ), "PNG" );
        return cacheCoverDir().filePath( widthKey + "nocover.png" );
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

    //Only report genres that currently have at least one song
    qb.addFilter(QueryBuilder::tabSong, "");

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
    if (getDbConnectionType() == DbConnection::postgresql)
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
    if (getDbConnectionType() == DbConnection::postgresql)
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
CollectionDB::addSong( MetaBundle* bundle, const bool incremental )
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

    command += escapeString( QString::number( albumID( bundle->album(),   true, !incremental, false ) ) ) + ",";
    command += escapeString( QString::number( artistID( bundle->artist(), true, !incremental, false ) ) ) + ",";
    command += escapeString( QString::number( genreID( bundle->genre(),   true, !incremental, false ) ) ) + ",'";
    command += escapeString( QString::number( yearID( QString::number( bundle->year() ), true, !incremental, false ) ) ) + "','";

    command += escapeString( bundle->title() ) + "','";
    command += escapeString( bundle->comment() ) + "', ";
    command += ( !bundle->track() ? "NULL" : escapeString( QString::number( bundle->track() ) ) ) + " , ";
    command += artist == i18n( "Various Artists" ) ? boolT() + "," : boolF() + ",";

    // NOTE any of these may be -1 or -2, this is what we want
    //      see MetaBundle::Undetermined
    command += QString::number( bundle->length() ) + ",";
    command += QString::number( bundle->bitrate() ) + ",";
    command += QString::number( bundle->sampleRate() ) + ")";

    //FIXME: currently there's no way to check if an INSERT query failed or not - always return true atm.
    // Now it might be possible as insert returns the rowid.
    insert( command, NULL);
    return true;
}

QString
CollectionDB::getURL( const QString &artist, const QString &album, const QString &title )
{
    uint artID = artistID( artist, false );
    if( !artID )
        return QString::null;

    uint albID = albumID( album, false );
    if( !albID )
        return QString::null;


    QStringList urls = query( QString(
            "SELECT tags.url "
            "FROM tags "
            "WHERE tags.album = '%1' AND tags.artist = '%2' AND tags.title = '%3';" )
                .arg( albID ).arg( artID ).arg( escapeString( title ) ) );

    if( urls.empty() )
        return QString::null;

    if( urls.size() == 1 )
    {
        return urls.first();
    }

    QString url = urls.first();
    int maxPlayed = -1;
    for( QStringList::iterator it = urls.begin();
            it != urls.end();
            it++ )
    {
        int pc = getPlayCount( *it );
        if( pc > maxPlayed )
        {
            maxPlayed = pc;
            url = *it;
        }
    }

    return url;
}


static void
fillInBundle( QStringList values, MetaBundle &bundle )
{
    //TODO use this whenever possible

    // crash prevention
    while( values.count() < 10 )
        values += "IF YOU CAN SEE THIS THERE IS A BUG!";

    QStringList::ConstIterator it = values.begin();

    bundle.setAlbum     ( *it ); ++it;
    bundle.setArtist    ( *it ); ++it;
    bundle.setGenre     ( *it ); ++it;
    bundle.setTitle     ( *it ); ++it;
    bundle.setYear      ( (*it).toInt() ); ++it;
    bundle.setComment   ( *it ); ++it;
    bundle.setTrack     ( (*it).toInt() ); ++it;
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
                b.setYear      ( (*++it).toInt() );
                b.setComment   (  *++it );
                b.setTrack     ( (*++it).toInt() );
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
            "SELECT playcounter, createdate, percentage, rating FROM statistics "
            "WHERE url = '%1';" )
            .arg( escapeString( url ) ) );

    // check boundaries
    if ( percentage > 100 ) percentage = 100;
    if ( percentage < 1 )   percentage = 1;

    if ( !values.isEmpty() )
    {
        if ( values.first().toInt() )
            // had already been played
            score = ( ( values[2].toDouble() * values.first().toInt() ) + percentage ) / ( values.first().toInt() + 1 );
        else
            // it's the first time track is played
            score = ( ( 50 + percentage ) / 2 );
        // increment playcounter and update accesstime
        if (getDbConnectionType() == DbConnection::postgresql) {
            query( QString( "UPDATE statistics SET percentage=%1, playcounter=%2, accessdate=%3 WHERE url='%4';" )
                            .arg( score )
                            .arg( values[0] + " + 1" )
                            .arg( QDateTime::currentDateTime().toTime_t() )
                            .arg( escapeString( url ) ) );
        }
        else
        {
            query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter, rating ) "
                            "VALUES ( '%6', %2, %3, %4, %5, %1 );" )
                            .arg( values[3] )
                            .arg( values[1] )
                            .arg( QDateTime::currentDateTime().toTime_t() )
                            .arg( score )
                            .arg( values[0] + " + 1" )
                            .arg( escapeString( url ) ) );
        }
    }
    else
    {
        // entry didnt exist yet, create a new one
        score = ( ( 50 + percentage ) / 2 );

        insert( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter, rating ) "
                        "VALUES ( '%4', %2, %3, %1, 1, 0 );" )
                        .arg( score )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( escapeString( url ) ), NULL );
    }

    int iscore = getSongPercentage( url );
    emit scoreChanged( url, iscore );
    return iscore;
}


int
CollectionDB::getSongPercentage( const QString &url )
{
    QStringList values = query( QString( "SELECT round( percentage + 0.4 ) FROM statistics WHERE url = '%1';" )
                                         .arg( escapeString( url ) ) );

    if( values.count() )
        return values.first().toInt();

    return 0;
}

int
CollectionDB::getSongRating( const QString &url )
{
    QStringList values = query( QString( "SELECT rating FROM statistics WHERE url = '%1';" )
                                         .arg( escapeString( url ) ) );

    if( values.count() )
        return values.first().toInt();

    return 0;
}

void
CollectionDB::setSongPercentage( const QString &url , int percentage)
{
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, accessdate, rating FROM statistics WHERE url = '%1';" )
            .arg( escapeString( url ) ));

    // check boundaries
    if ( percentage > 100 ) percentage = 100;
    if ( percentage < 0 )   percentage = 0;

    if ( !values.isEmpty() )
    {
        if (getDbConnectionType() == DbConnection::postgresql) {
            query( QString( "UPDATE statistics SET percentage=%1 WHERE url='%2';" )
                            .arg( percentage )
                            .arg( escapeString( url ) ));
        }
        else
        {
            // entry exists
            query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter, rating ) "
                            "VALUES ( '%6', '%2', '%3', %4, %5, %1 );" )
                            .arg( values[3] )
                            .arg( values[1] )
                            .arg( values[2] )
                            .arg( percentage )
                            .arg( values[0] )
                            .arg( escapeString( url ) ) );
        }
    }
    else
    {
        insert( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter, rating ) "
                         "VALUES ( '%4', %2, %3, %1, 0, 0 );" )
                        .arg( percentage )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( escapeString( url ) ), NULL );
    }

    emit scoreChanged( url, percentage );
}

void
CollectionDB::setSongRating( const QString &url, int rating )
{
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, accessdate, percentage FROM statistics WHERE url = '%1';" )
            .arg( escapeString( url ) ));

    // check boundaries
    if ( rating > 5 ) rating = 5;
    if ( rating < 0 ) rating = 0;

    if ( !values.isEmpty() )
    {
        if (getDbConnectionType() == DbConnection::postgresql) {
            query( QString( "UPDATE statistics SET rating=%1 WHERE url='%2';" )
                            .arg( rating )
                            .arg( escapeString( url ) ));
        }
        else
        {
            // entry exists
            query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, rating, playcounter ) "
                            "VALUES ( '%6', '%2', '%3', %4, %5, %1 );" )
                            .arg( values[0] )
                            .arg( values[1] )
                            .arg( values[2] )
                            .arg( values[3] )
                            .arg( rating )
                            .arg( escapeString( url ) ) );
        }
    }
    else
    {
        insert( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, rating, playcounter ) "
                         "VALUES ( '%4', %2, %3, 0, %1, 0 );" )
                        .arg( rating )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( escapeString( url ) ), NULL );
    }

    emit ratingChanged( url, rating );
}

int
CollectionDB::getPlayCount( const QString &url )
{
    QStringList values = query( QString( "SELECT playcounter FROM statistics WHERE url = '%1';" )
                                         .arg( escapeString( url ) ) );
    if( values.count() )
        return values.first().toInt();
    return 0;
}

QDateTime
CollectionDB::getFirstPlay( const QString &url )
{
    QDateTime dt = QDateTime();
    QStringList values = query( QString( "SELECT createdate FROM statistics WHERE url = '%1';" )
                                         .arg( escapeString( url ) ) );
    if( values.count() )
        dt.setTime_t( values.first().toUInt() );
    return dt;
}

QDateTime
CollectionDB::getLastPlay( const QString &url )
{
    QDateTime dt = QDateTime();
    QStringList values = query( QString( "SELECT accessdate FROM statistics WHERE url = '%1';" )
                                         .arg( escapeString( url ) ) );
    if( values.count() )
        dt.setTime_t( values.first().toUInt() );
    else
        dt.setTime_t( 0 );
    return dt;
}
/*!
 *  @short: exchange url references in the database for a particular file
 *  @note: deletes all items for newURL, changes oldURL->newURL, deletes oldURL.
 *  FIXME: should we check if lyrics etc exist in the newURL and keep them if necessary?
 */
void
CollectionDB::migrateFile( const QString &oldURL, const QString &newURL )
{
    //  Ensure destination is clear.
    query( QString( "DELETE FROM tags WHERE url = '%1';" )
        .arg( escapeString( newURL ) ) );

    query( QString( "DELETE FROM statistics WHERE url = '%1';" )
        .arg( escapeString( newURL ) ) );

    if ( !getLyrics( oldURL ).isEmpty() )
        query( QString( "DELETE FROM lyrics WHERE url = '%1';" )
            .arg( escapeString( newURL ) ) );
    //  Migrate
    query( QString( "UPDATE tags SET url = '%1' WHERE url = '%2';" )
        .arg( escapeString( newURL ) )
        .arg( escapeString( oldURL ) ) );

    query( QString( "UPDATE statistics SET url = '%1' WHERE url = '%2';" )
        .arg( escapeString( newURL ) )
        .arg( escapeString( oldURL ) ) );

    query( QString( "UPDATE lyrics SET url = '%1' WHERE url = '%2';" )
        .arg( escapeString( newURL ) )
        .arg( escapeString( oldURL ) ) );
    //  Clean up.
    query( QString( "DELETE FROM tags WHERE url = '%1';" )
        .arg( escapeString( oldURL ) ) );

    query( QString( "DELETE FROM statistics WHERE url = '%1';" )
        .arg( escapeString( oldURL ) ) );

    query( QString( "DELETE FROM lyrics WHERE url = '%1';" )
        .arg( escapeString( oldURL ) ) );
}

bool
CollectionDB::moveFile( const QString &src, const QString &dest, bool overwrite, bool copy )
{
    if ( copy || isFileInCollection( src ) ){
        if(src == dest){
            debug() << "Source and destination URLs are the same, aborting."  << endl;
            return false;
        }
        else
        {
            // Escape URL.
            KURL srcURL = KURL::fromPathOrURL( src );
            KURL dstURL = KURL::fromPathOrURL( dest );

            // Clean it.
            srcURL.cleanPath();
            dstURL.cleanPath();

            // Make sure it is valid.
            if(!srcURL.isValid() || !dstURL.isValid())
                debug() << "Invalid URL "  << endl;

                // Get just the directory.
                KURL dir = dstURL;
                dir.setFileName(QString::null);

            // Create the directory.
            if(!KStandardDirs::exists(dir.path()))
                if(!KStandardDirs::makeDir(dir.path())) {
                    debug() << "Unable to create directory " << dir.path() << endl;
                }

            if( copy )
            {
                // Copy the file and dirty destination directory
                if ( KIO::NetAccess::file_copy( srcURL, dstURL, -1, overwrite ) ){
                    MetaBundle bundle( dstURL );
                    if( bundle.isValidMedia() )
                    {
                        addSong( &bundle, true /* XXX: incremental? */ );
                        return true;
                    }
                    return false;
                }
                else
                    return false;
            }
            else
            {
                // Move the file and update DB
                if ( KIO::NetAccess::file_move( srcURL, dstURL, -1, overwrite ) ){
                    migrateFile( src, dest );
                    return true;
                }
                else
                    return false;
            }

        }
    }
    else
        return false;
}

void
CollectionDB::updateDirStats( QString path, const long datetime, const bool temporary )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    if (getDbConnectionType() == DbConnection::postgresql) {
    // REPLACE INTO is not valid SQL for postgres, so we need to check whether we
    // should UPDATE() or INSERT()
    QStringList values = query( QString("SELECT * FROM directories%1 WHERE dir='%2';")
        .arg( temporary ? "_temp" : "")
        .arg( escapeString( path ) ) );

    if(values.count() > 0 )
    {
        query( QString( "UPDATE directories%1 SET changedate=%2 WHERE dir='%3';")
        .arg( temporary ? "_temp" : "" )
        .arg( datetime )
        .arg( escapeString( path ) ));
    }
    else
    {
        query( QString( "INSERT INTO directories%1 (dir,changedate) VALUES ('%2','%3');")
        .arg( temporary ? "_temp" : "")
        .arg( escapeString( path ) )
        .arg( datetime ) );
    }
    }
    else
    {
        query( QString( "REPLACE INTO directories%1 ( dir, changedate ) VALUES ( '%2', %3 );" )
                  .arg( temporary ? "_temp" : "" )
                  .arg( escapeString( path ) )
                  .arg( datetime ) );
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

    if (getDbConnectionType() == DbConnection::postgresql) {
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
CollectionDB::checkCompilations( const QString &path, const bool temporary )
{
    QStringList albums;
    QStringList artists;
    QStringList dirs;

    albums = query( QString( "SELECT DISTINCT album.name FROM tags_temp, album%1 AS album WHERE tags_temp.dir = '%2' AND album.id = tags_temp.album;" )
              .arg( temporary ? "_temp" : "" )
              .arg( escapeString( path ) ));

    for ( uint i = 0; i < albums.count(); i++ )
    {
        if ( albums[ i ].isEmpty() ) continue;

        const uint album_id = albumID( albums[ i ], false, temporary, false );
        artists = query( QString( "SELECT DISTINCT artist.name FROM tags_temp, artist%1 AS artist WHERE tags_temp.album = '%2' AND tags_temp.artist = artist.id;" )
                            .arg( temporary ? "_temp" : "" )
                            .arg( album_id ) );
        dirs    = query( QString( "SELECT DISTINCT dir FROM tags_temp WHERE album = '%1';" )
                            .arg( album_id ) );

        if ( artists.count() > dirs.count() )
        {
            debug() << "Detected compilation: " << albums[ i ] << " - " << artists.count() << ":" << dirs.count() << endl;
            query( QString( "UPDATE tags_temp SET sampler = %1 WHERE album = '%2';" )
                            .arg(boolT()).arg( album_id ) );
        }
    }
}


void
CollectionDB::setCompilation( const QString &album, const bool enabled, const bool updateView )
{
    QStringList values = query( QString( "SELECT album.id FROM album WHERE album.name = '%1';" )
              .arg( escapeString( album ) ) );
    if ( values.count() ) {
        query( QString( "UPDATE tags SET sampler = %1 WHERE tags.album = %2;" )
                .arg( enabled ? "1" : "0" )
                .arg( values[0] ) ) ;
    }
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
    command += "year = "   + QString::number( yearID( QString::number( bundle.year() ), true, false, true ) ) + ", ";
    if ( bundle.track() )
        command += "track = " + QString::number( bundle.track() ) + ", ";
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

    emit tagsChanged( bundle );
}


void
CollectionDB::updateURL( const QString &url, const bool updateView )
{
    // don't use the KURL ctor as it checks the db first
    MetaBundle bundle;
    bundle.setPath( url );
    bundle.readTags( TagLib::AudioProperties::Fast );

    updateTags( url, bundle, updateView);
}


void
CollectionDB::setHTMLLyrics( const QString &url, QString lyrics )
{

    // remove breaklines
    lyrics.replace( "\n", QString::null );
    lyrics.replace( "\r", QString::null );
    lyrics.replace( "\t", QString::null );

    // Remove Entilities - Lyrc doesn't use any, but anyway...
    lyrics = KCharsets::resolveEntities( lyrics );

    //<br> and <p> become a breakline
    lyrics.replace( QRegExp("<[bB][rR][^>]*>[ ]*"), "\n" );
    lyrics.replace( QRegExp("<[pP][^>]*>[ ]*"), "\n" );

    //add a breakline right after the artist name (Lyrc uses <b>Ttile</b><br><u>Artist</u>)
    lyrics.replace( QRegExp("<[/][uU][^>]*>"), "\n" );

    // remove all tags
    lyrics.replace( QRegExp("<[^>]*>"), QString::null );

    setLyrics( url, lyrics );
}



void
CollectionDB::setLyrics( const QString &url, const QString &lyrics )
{
    QStringList values = query(QString("SELECT lyrics FROM lyrics WHERE url = '%1';").arg( escapeString( url ) ) );
    if(values.count() > 0)
    {
        query( QString( "UPDATE lyrics SET lyrics = '%1' WHERE url = '%2';" ).arg( escapeString( lyrics ), escapeString( url ) ));
    }
    else
    {
        insert( QString( "INSERT INTO lyrics (url, lyrics) values ( '%1', '%2' );" ).arg( escapeString( url ), escapeString( lyrics ) ),
         NULL);
    }
}


QString
CollectionDB::getHTMLLyrics( const QString &url )
{
    QString lyrics = getLyrics( url );
    lyrics.replace( "\n", "<br />" );
    //FIXME: It should also handle html entities
    return lyrics;
}


QString
CollectionDB::getLyrics( const QString &url )
{
    QStringList values = query( QString( "SELECT lyrics FROM lyrics WHERE url = '%1';" ).arg( escapeString( url ) ));
    return values[0];
}

void CollectionDB::newAmazonReloadDate( const QString& asin, const QString& locale, const QString& md5sum)
{
    QStringList values = query(QString("SELECT filename FROM amazon WHERE filename = '%1'")
        .arg(md5sum));
    if(values.count() > 0)
    {
        query( QString("UPDATE amazon SET asin = '%1', locale = '%2', refetchdate = '%3' WHERE filename = '%4'")
            .arg(asin)
            .arg(locale)
            .arg(QDateTime::currentDateTime().addDays(80).toTime_t())
            .arg(md5sum));
    }
    else
    {
        insert( QString( "INSERT INTO amazon ( asin, locale, filename, refetchdate ) VALUES ( '%1', '%2', '%3', '%4');" )
         .arg(asin)
         .arg(locale)
         .arg(md5sum)
         .arg(QDateTime::currentDateTime().addDays(80).toTime_t()), NULL );
    }
}

QStringList CollectionDB::staleImages()
{
    return query(QString("SELECT asin, locale, filename FROM amazon WHERE refetchdate < %1 ;")
            .arg(QDateTime::currentDateTime().toTime_t() ));
}

void
CollectionDB::applySettings()
{
    bool recreateConnections = false;
    if ( AmarokConfig::databaseEngine().toInt() != getDbConnectionType() )
    {
        if ( AmarokConfig::databaseEngine().toInt() == DbConnection::mysql )
            m_dbConnType = DbConnection::mysql;
        else if ( AmarokConfig::databaseEngine().toInt() == DbConnection::postgresql )
            m_dbConnType = DbConnection::postgresql;
        else m_dbConnType = DbConnection::sqlite;
        recreateConnections = true;
    }
    else if ( AmarokConfig::databaseEngine().toInt() == DbConnection::mysql )
    {
        // Using MySQL, so check if MySQL settings were changed
        const MySqlConfig *config =
            static_cast<const MySqlConfig*> ( m_dbConfig );
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
          static_cast<const PostgresqlConfig*> ( m_dbConfig );
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

DbConnection * CollectionDB::getMyConnection()
{
    //after some thought, to be thread-safe, must lock at the beginning of this function,
    //not only if a new connection is made
    connectionMutex->lock();

    DbConnection *dbConn;
    DbConfig *config;
    QThread *currThread = ThreadWeaver::Thread::getRunning();

    if (threadConnections->contains(currThread))
    {
        QMap<QThread *, DbConnection *>::Iterator it = threadConnections->find(currThread);
        dbConn = it.data();
        connectionMutex->unlock();
        return dbConn;
    }

#ifdef USE_MYSQL
    if ( m_dbConnType == DbConnection::mysql )
    {
        config =
            new MySqlConfig(
                AmarokConfig::mySqlHost(),
                AmarokConfig::mySqlPort(),
                AmarokConfig::mySqlDbName(),
                AmarokConfig::mySqlUser(),
                AmarokConfig::mySqlPassword() );
        dbConn = new MySqlConnection( static_cast<MySqlConfig*> ( config ) );
    }
    else
#endif
#ifdef USE_POSTGRESQL
    if ( m_dbConnType == DbConnection::postgresql )
    {
        config =
            new PostgresqlConfig(
                AmarokConfig::postgresqlConninfo() );
        dbConn = new PostgresqlConnection( static_cast<PostgresqlConfig*> ( config ) );
    }
    else
#endif
    {
        config = new SqliteConfig( "collection.db" );
        dbConn = new SqliteConnection( static_cast<SqliteConfig*> ( config ) );
    }

    threadConnections->insert(currThread, dbConn);

    m_dbConfig = config;

    connectionMutex->unlock();
    return dbConn;

}

void
CollectionDB::destroyConnections()
{
    //do we need or want to delete the actual connection objects as well as clearing them from the QMap?
    //or does QMap's clear function delete them?
    //this situation is not at all likely to happen often, so leaving them might be okay to prevent a
    //thread from having its connection torn out from under it...not likely, but possible
    //and leaving it should not end up eating much memory at all
    connectionMutex->lock();
    threadConnections->clear();
    connectionMutex->unlock();
}

void
CollectionDB::releasePreviousConnection(QThread *currThread)
{
    //if something already exists, delete the object, and erase knowledge of it from the QMap.
    connectionMutex->lock();
    DbConnection *dbConn;
    if (threadConnections->contains(currThread))
    {
        QMap<QThread *, DbConnection *>::Iterator it = threadConnections->find(currThread);
        dbConn = it.data();
        delete dbConn;
        threadConnections->erase(currThread);
    }
    connectionMutex->unlock();
}

bool
CollectionDB::isConnected()
{
    return getMyConnection()->isConnected();
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

    const KURL &url = EngineController::instance()->bundle().url();
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
    QStringList folders = AmarokConfig::collectionFolders();

    if ( folders.isEmpty() ) {
        dropTables(false);
        createTables(false);
    }
    else if( PlaylistBrowser::instance() && !ScanController::instance() )
    {
        emit scanStarted();
        new ScanController( this, false, folders );
    }
}


void
CollectionDB::stopScan() //SLOT
{
    delete ScanController::instance();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::dirDirty( const QString& path )
{
    debug() << k_funcinfo << "Dirty: " << path << endl;

    new ScanController( this, false, path );
}


void
CollectionDB::coverFetcherResult( CoverFetcher *fetcher )
{
    if( fetcher->wasError() ) {
        error() << fetcher->errors() << endl;
        emit coverFetcherError( fetcher->errors().front() );
    }

    else {
        setAlbumImage( fetcher->artist(), fetcher->album(), fetcher->image(), fetcher->amazonURL(), fetcher->asin() );
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
                    .arg( CollectionDB::instance()->escapeString( *it ) ), NULL);

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

    DbConnection *dbConn = getMyConnection( );

    KConfig* config = amaroK::config( "Collection Browser" );
    if(!dbConn->isConnected())
        amaroK::MessageQueue::instance()->addMessage(dbConn->lastError());
    if ( !dbConn->isInitialized() || !isValid() )
    {
        createTables(false);
        createPersistentTables();
        createStatsTable();
    }
    else
    {
        if ( adminValue( "Database Stats Version" ).toInt() != DATABASE_STATS_VERSION )
        {
            debug() << "Different database stats version detected! Stats table will be updated or rebuilt." << endl;
            debug() << "Creating a backup of the database in "
                    << amaroK::saveLocation()+"collection-backup.db" << "." << endl;
            debug() << "Unfortunately, this only works for SQLite databases." << endl;

            bool copied = KIO::NetAccess::file_copy( amaroK::saveLocation()+"collection.db",
                                                     amaroK::saveLocation()+"collection-backup.db",
                                                     -1 /*perms*/, true /*overwrite*/, false /*resume*/ );

            if( !copied )
            {
                debug() << "Backup failed! Perhaps you are not using SQLite, or the volume is not writable." << endl;
                debug() << "Error was: " << KIO::NetAccess::lastErrorString() << endl;
            }

            int prev = adminValue( "Database Stats Version" ).toInt();
            if( !prev && config->readNumEntry( "Database Stats Version", 0 )
                      && config->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION )
                prev = config->readNumEntry( "Database Stats Version", 0 );

            //pre somewhere in the 1.3-1.4 timeframe, the version wasn't stored in the DB, so try to guess it
            const QString q = "SELECT COUNT( %1 ) FROM statistics;";
            if( !prev && query( q.arg( "url" ) ).first().toInt()
                      && query( q.arg( "createdate" ) ).first().toInt()
                      && query( q.arg( "accessdate" ) ).first().toInt()
                      && query( q.arg( "percentage" ) ).first().toInt()
                      && query( q.arg( "playcounter" ) ).first().toInt() )
            {
                prev = 3;
            }

            if( prev == 3 ) //every version from 1.2 forward had a stats version of 3
            {
                debug() << "Updating stats-database!" << endl;
                query( "ALTER TABLE statistics ADD rating INTEGER DEFAULT 0;" );
                query( "CREATE INDEX rating_stats ON statistics( rating );" );
                query( "UPDATE statistics SET rating=0 WHERE " + boolT() + ";" );
            }
            else //it is from before 1.2, or our poor user is otherwise fucked
            {
                debug() << "Rebuilding stats-database!" << endl;
                dropStatsTable();
                createStatsTable();
            }
        }

        QString PersistentVersion = adminValue( "Database Persistent Tables Version" );
        if ( PersistentVersion.isEmpty() ) {
            /* persistent tables didn't have a version on 1.3X and older, but let's be nice and try to
               copy/keep the good information instead of just deleting the tables */
            debug() << "Detected old schema for tables with important data. amaroK will convert the tables, ignore any \"table already exists\" errors." << endl;
            createPersistentTables();
            /* Copy lyrics */
            debug() << "Trying to get lyrics from old db schema." << endl;
            QStringList Lyrics = query( "SELECT url, lyrics FROM tags where tags.lyrics IS NOT NULL;" );
            for (uint i=0; i<Lyrics.count(); i+=2  )
                setHTMLLyrics( Lyrics[i], Lyrics[i+1]  );
        }
        else if ( PersistentVersion == "1" || PersistentVersion == "2" ) {
            createPersistentTables(); /* From 1 to 2 nothing changed. There was just a bug on the code, and
                                         on some cases the table wouldn't be created.
                                         From 2 to 3, lyrics were made plain text, instead of HTML */
            debug() << "Converting Lyrics to Plain Text." << endl;
            QStringList Lyrics = query( "SELECT url, lyrics FROM lyrics;" );
            for (uint i=0; i<Lyrics.count(); i+=2  )
                setHTMLLyrics( Lyrics[i], Lyrics[i+1]  );

        }
        else {
            if ( adminValue( "Database Persistent Tables Version" ).toInt() != DATABASE_PERSISTENT_TABLES_VERSION ) {
                debug() << "Rebuilding persistent tables database!" << endl;
                dropPersistentTables();
                createPersistentTables();
            }
        }

        //remove database file if version is incompatible
        if ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION
             || adminValue( "Database Version" ).toInt() != DATABASE_VERSION )
        {
            debug() << "Rebuilding database!" << endl;
            dropTables(false);
            createTables(false);
        }
    }
}


void
CollectionDB::destroy()
{
    destroyConnections();
}


void
CollectionDB::scanModifiedDirs()
{
    //we check if a job is pending because we don't want to abort incremental collection readings
    if ( !ScanController::instance() && PlaylistBrowser::instance() ) {
        emit scanStarted();
        new ScanController( this, true ); // Incremental scanning mode
    }
}


QDir
CollectionDB::largeCoverDir() //static
{
    return QDir( amaroK::saveLocation( "albumcovers/large/" ) );
}


QDir
CollectionDB::tagCoverDir()  //static
{
    return QDir( amaroK::saveLocation( "albumcovers/tagcover/" ) );
}


QDir
CollectionDB::cacheCoverDir()  //static
{
    return QDir( amaroK::saveLocation( "albumcovers/cache/" ) );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS DbConnection
//////////////////////////////////////////////////////////////////////////////////////////

DbConnection::DbConnection( DbConfig* config )
    : m_initialized( false )
    , m_config( config )
{}


DbConnection::~DbConnection()
{}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS SqliteConnection
//////////////////////////////////////////////////////////////////////////////////////////

SqliteConnection::SqliteConnection( SqliteConfig* config )
    : DbConnection( config )
{

    DEBUG_BLOCK

    const QCString path = QFile::encodeName( amaroK::saveLocation() + "collection.db" );

    // Open database file and check for correctness
    QFile file( path );
    if ( file.open( IO_ReadOnly ) )
    {
        QString format;
        file.readLine( format, 50 );
        if ( !format.startsWith( "SQLite format 3" ) )
        {
            warning() << "Database versions incompatible. Removing and rebuilding database.\n";
        }
        else if ( sqlite3_open( path, &m_db ) != SQLITE_OK )
        {
            warning() << "Database file corrupt. Removing and rebuilding database.\n";
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
    if ( m_initialized )
    {
        if( sqlite3_create_function(m_db, "rand", 0, SQLITE_UTF8, NULL, sqlite_rand, NULL, NULL) != SQLITE_OK )
            m_initialized = false;
        if( sqlite3_create_function(m_db, "power", 2, SQLITE_UTF8, NULL, sqlite_power, NULL, NULL) != SQLITE_OK )
            m_initialized = false;
    }

    //optimization for speeding up SQLite
    query( "PRAGMA default_synchronous = OFF;" );
}


SqliteConnection::~SqliteConnection()
{
    if ( m_db ) sqlite3_close( m_db );
}


QStringList SqliteConnection::query( const QString& statement )
{

    QStringList values;
    int error;
    const char* tail;
    sqlite3_stmt* stmt;

    //compile SQL program to virtual machine
    error = sqlite3_prepare( m_db, statement.utf8(), statement.length(), &stmt, &tail );

    if ( error != SQLITE_OK )
    {
        Debug::error() << k_funcinfo << " sqlite3_compile error:" << endl;
        Debug::error() << sqlite3_errmsg( m_db ) << endl;
        Debug::error() << "on query: " << statement << endl;
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
                    Debug::error() << "Busy-counter has reached maximum. Aborting this sql statement!\n";
                    break;
                }
                ::usleep( 100000 ); // Sleep 100 msec
                debug() << "sqlite3_step: BUSY counter: " << busyCnt << endl;
            }
            if ( error == SQLITE_MISUSE )
                debug() << "sqlite3_step: MISUSE" << endl;
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
            Debug::error() << k_funcinfo << "sqlite_step error.\n";
            Debug::error() << sqlite3_errmsg( m_db ) << endl;
            Debug::error() << "on query: " << statement << endl;
            values = QStringList();
        }
    }

    return values;
}


int SqliteConnection::insert( const QString& statement, const QString& /* table */ )
{
    int error;
    const char* tail;
    sqlite3_stmt* stmt;
    //compile SQL program to virtual machine
    error = sqlite3_prepare( m_db, statement.utf8(), statement.length(), &stmt, &tail );

    if ( error != SQLITE_OK )
    {
        Debug::error() << k_funcinfo << " sqlite3_compile error:" << endl;
        Debug::error() << sqlite3_errmsg( m_db ) << endl;
        Debug::error() << "on insert: " << statement << endl;
    }
    else
    {
        int busyCnt = 0;
        //execute virtual machine by iterating over rows
        while ( true )
        {
            error = sqlite3_step( stmt );

            if ( error == SQLITE_BUSY )
            {
                if ( busyCnt++ > 20 ) {
                    Debug::error() << "Busy-counter has reached maximum. Aborting this sql statement!\n";
                    break;
                }
                ::usleep( 100000 ); // Sleep 100 msec
                debug() << "sqlite3_step: BUSY counter: " << busyCnt << endl;
            }
            if ( error == SQLITE_MISUSE )
                debug() << "sqlite3_step: MISUSE" << endl;
            if ( error == SQLITE_DONE || error == SQLITE_ERROR )
                break;
        }
        //deallocate vm ressources
        sqlite3_finalize( stmt );

        if ( error != SQLITE_DONE )
        {
            Debug::error() << k_funcinfo << "sqlite_step error.\n";
            Debug::error() << sqlite3_errmsg( m_db ) << endl;
            Debug::error() << "on insert: " << statement << endl;
        }
    }
    return sqlite3_last_insert_rowid( m_db );
}


// this implements a RAND() function compatible with the MySQL RAND() (0-param-form without seed)
void SqliteConnection::sqlite_rand(sqlite3_context *context, int /*argc*/, sqlite3_value ** /*argv*/)
{
    sqlite3_result_double( context, static_cast<double>(KApplication::random()) / (RAND_MAX+1.0) );
}

// this implements a POWER() function compatible with the MySQL POWER()
void SqliteConnection::sqlite_power(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    Q_ASSERT( argc==2 );
    if( sqlite3_value_type(argv[0])==SQLITE_NULL || sqlite3_value_type(argv[1])==SQLITE_NULL ) {
        sqlite3_result_null(context);
        return;
    }
    double a = sqlite3_value_double(argv[0]);
    double b = sqlite3_value_double(argv[1]);
    sqlite3_result_double( context, pow(a,b) );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS MySqlConnection
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_MYSQL
MySqlConnection::MySqlConnection( MySqlConfig* config )
    : DbConnection( config )
    , m_connected( false )
{
    DEBUG_BLOCK

    debug() << k_funcinfo << endl;
    m_db = mysql_init(NULL);
    if (m_db)
    {
        if ( config->username().isEmpty() )
            pApp->slotConfigAmarok("MySql");

        if ( mysql_real_connect( m_db, config->host().latin1(),
                                              config->username().latin1(),
                                              config->password().latin1(),
                                              config->database().latin1(),
                                              config->port(),
                                              NULL, CLIENT_COMPRESS ) )
        {
            m_initialized = true;
            m_connected = true;
        }
        else
        {

            if ( mysql_real_connect(
                    m_db,
                    config->host().latin1(),
                    config->username().latin1(),
                    config->password().latin1(),
                    NULL,
                    config->port(),
                    NULL, CLIENT_COMPRESS))
            {
                if ( mysql_query(m_db,
                    QString( "CREATE DATABASE " + config->database() ).latin1() ) )
                    { m_connected = true; }
                else
                    { setMysqlError(); }
            }
            else
                setMysqlError();

        }
    }
    else
        error() << "Failed to allocate/initialize MySql struct\n";
}


MySqlConnection::~MySqlConnection()
{
    if ( m_db ) mysql_close( m_db );
}


QStringList MySqlConnection::query( const QString& statement )
{
    QStringList values;

    if ( !mysql_query( m_db, statement.utf8() ) )
    {
        MYSQL_RES* result;
        if ( ( result = mysql_use_result( m_db ) ) )
        {
            int number = mysql_field_count( m_db );
            MYSQL_ROW row;
            while ( ( row = mysql_fetch_row( result ) ) )
            {
                for ( int i = 0; i < number; i++ )
                {
                  values << QString::fromUtf8( (const char*)row[i] );
                }
            }
            mysql_free_result( result );
        }
        else
        {
            if ( mysql_field_count( m_db ) != 0 )
            {
                debug() << "MYSQL QUERY FAILED: " << mysql_error( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
                values = QStringList();
            }
        }
    }
    else
    {
        debug() << "MYSQL QUERY FAILED: " << mysql_error( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
        values = QStringList();
    }

    return values;
}


int MySqlConnection::insert( const QString& statement, const QString& /* table */ )
{
    mysql_query( m_db, statement.utf8() );
    return mysql_insert_id( m_db );
}


void
MySqlConnection::setMysqlError()
{
    m_error = i18n("MySQL reported the following error:<br>") + mysql_error(m_db)
            + i18n("<p>You can configure MySQL in the Collection section under Settings->Configure amaroK</p>");
}
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS PostgresqlConnection
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_POSTGRESQL
PostgresqlConnection::PostgresqlConnection( PostgresqlConfig* config )
      : DbConnection( config )
      , m_connected( false )
{
    debug() << k_funcinfo << endl;

    if ( config->conninfo().isEmpty() )
        pApp->slotConfigAmarok("Postgresql");

    m_db = PQconnectdb( config->conninfo().latin1() );
    if (!m_db)
    {
        debug() << "POSTGRESQL CONNECT FAILED: " << PQerrorMessage( m_db ) << "\n";
        error() << "Failed to allocate/initialize Postgresql struct\n";
        setPostgresqlError();
        return;
    }

    if (PQstatus(m_db) != CONNECTION_OK)
    {
        debug() << "POSTGRESQL CONNECT FAILED: " << PQerrorMessage( m_db ) << "\n";
        error() << "Failed to allocate/initialize Postgresql struct\n";
        setPostgresqlError();
        PQfinish(m_db);
        m_db = NULL;
        return;
    }

    m_initialized = true;
    m_connected = true;
}


PostgresqlConnection::~PostgresqlConnection()
{
    if ( m_db ) PQfinish( m_db );
}


QStringList PostgresqlConnection::query( const QString& statement )
{
    QStringList values;
    PGresult* result;
    ExecStatusType status;

    result = PQexec(m_db, statement.utf8());
    if (result == NULL)
    {
        debug() << "POSTGRESQL QUERY FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
        return values;
    }

    status = PQresultStatus(result);
    if ((status != PGRES_COMMAND_OK) && (status != PGRES_TUPLES_OK))
    {
        debug() << "POSTGRESQL QUERY FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
        PQclear(result);
        return values;
    }

    int cols = PQnfields( result );
    int rows = PQntuples( result );
    QMap<int, bool> discardCols;
    for(int col=0; col< cols; col++) {
        if (QString(PQfname(result, col)) == QString("__discard") || QString(PQfname(result, col)) == QString("__random"))
        {
            discardCols[col] = true;
        }
    }

    for(int row=0; row< rows; row++)
    {
        for(int col=0; col< cols; col++)
        {
            if (discardCols[col]) continue;

            values << QString::fromUtf8(PQgetvalue(result, row, col));
        }
    }

    PQclear(result);

    return values;
}


int PostgresqlConnection::insert( const QString& statement, const QString& table )
{
    PGresult* result;
    ExecStatusType status;
    QString curvalSql;
    int id;

    result = PQexec(m_db, statement.utf8());
    if (result == NULL)
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << statement << "\n";
        return 0;
    }

    status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK)
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << statement << "\n";
        PQclear(result);
        return 0;
    }
    PQclear(result);

    if (table == NULL) return 0;

    QString _table = table;
    if (table.find("_temp") > 0) _table = table.left(table.find("_temp"));

    curvalSql = QString("SELECT currval('%1_seq');").arg(_table);
    result = PQexec(m_db, curvalSql.utf8());
    if (result == NULL)
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << curvalSql << "\n";
        return 0;
    }

    status = PQresultStatus(result);
    if (status != PGRES_TUPLES_OK)
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << curvalSql << "\n";
        PQclear(result);
        return 0;
    }

    if ((PQnfields( result ) != 1) || (PQntuples( result ) != 1))
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << curvalSql << "\n";
        PQclear(result);
        return 0;
    }

    id = QString::fromUtf8(PQgetvalue(result, 0, 0)).toInt();
    PQclear(result);

    return id;
}


void PostgresqlConnection::setPostgresqlError()
{
    m_error = i18n("Postgresql reported the following error:<br>") + PQerrorMessage(m_db)
            + i18n("<p>You can configure Postgresql in the Collection section under Settings->Configure amaroK</p>");
}
#endif



//////////////////////////////////////////////////////////////////////////////////////////
// CLASS SqliteConfig
//////////////////////////////////////////////////////////////////////////////////////////

SqliteConfig::SqliteConfig( const QString& dbfile )
    : m_dbfile( dbfile )
{
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS MySqlConfig
//////////////////////////////////////////////////////////////////////////////////////////

MySqlConfig::MySqlConfig(
    const QString& host,
    const int port,
    const QString& database,
    const QString& username,
    const QString& password )
    : m_host( host ),
      m_port( port ),
      m_database( database ),
      m_username( username ),
      m_password( password )
{
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS PostgresqlConfig
//////////////////////////////////////////////////////////////////////////////////////////

PostgresqlConfig::PostgresqlConfig(
    const QString& conninfo )
    : m_conninfo( conninfo )
{}

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS QueryBuilder
//////////////////////////////////////////////////////////////////////////////////////////

QueryBuilder::QueryBuilder()
    : m_OR( false )
{
    clear();
}


void
QueryBuilder::linkTables( int tables )
{

    m_tables = tableName( tabSong );

    if ( !(tables & tabSong ) )
    {
        // check if only one table is selected (does somebody know a better way to check that?)
        if (tables == tabAlbum || tables==tabArtist || tables==tabGenre || tables == tabYear || tables == tabStats)
            m_tables = tableName(tables);
        else
            tables |= tabSong;
    }


    if ( tables & tabSong )
    {
        if ( tables & tabAlbum )
            m_tables += " INNER JOIN " + tableName( tabAlbum) + " ON album.id=tags.album";
        if ( tables & tabArtist )
            m_tables += " INNER JOIN " + tableName( tabArtist) + " ON artist.id=tags.artist";
        if ( tables & tabGenre )
            m_tables += " INNER JOIN " + tableName( tabGenre) + " ON genre.id=tags.genre";
        if ( tables & tabYear )
            m_tables += " INNER JOIN " + tableName( tabYear) + " ON year.id=tags.year";
        if ( tables & tabStats )
            m_tables += " INNER JOIN " + tableName( tabStats) + " ON statistics.url=tags.url";
        if ( tables & tabLyrics )
            m_tables += " INNER JOIN " + tableName( tabLyrics) + " ON lyrics.url=tags.url";

    }
}


void
QueryBuilder::addReturnValue( int table, int value )
{
    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ",";
    if ( table & tabStats && value & valScore ) m_values += "round(";

    if ( value == valDummy )
        m_values += "''";
    else
    {
        m_values += tableName( table ) + ".";
        m_values += valueName( value );
    }

    if ( table & tabStats && value & valScore ) m_values += " + 0.4 )";

    m_linkTables |= table;
    m_returnValues++;
}

void
QueryBuilder::addReturnFunctionValue( int function, int table, int value)
{
    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ",";
    m_values += functionName( function ) + "(";
    m_values += tableName( table ) + ".";
    m_values += valueName( value )+ ")";
    m_values += " AS ";
    m_values += functionName( function )+tableName( table )+valueName( value );

    m_linkTables |= table;
    m_returnValues++;
}

uint
QueryBuilder::countReturnValues()
{
    return m_returnValues;
}

void
QueryBuilder::addURLFilters( const QStringList& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";

        for ( uint i = 0; i < filter.count(); i++ )
        {
                m_where += "OR tags.url = '" + CollectionDB::instance()->escapeString( filter[i] ) + "' ";
        }

        m_where += " ) ";
    }

    m_linkTables |= tabSong;
}

void
QueryBuilder::setGoogleFilter( int defaultTables, QString query )
{
    if( query.contains( "\"" ) % 2 == 1 ) query += "\""; //make an even number of "s

    //something like thingy"bla"stuff -> thingy "bla" stuff
    bool odd = false;
    for( int pos = query.find( "\"" );
         pos >= 0 && pos <= (int)query.length();
         pos = query.find( "\"", pos + 1 ) )
    {
        query = query.insert( odd ? ++pos : pos++, " " );
        odd = !odd;
    }
    query = query.simplifyWhiteSpace();

    int x; //position in string of the end of the next element
    bool OR = false, minus = false; //whether the next element is to be OR, and/or negated
    QString tmp, s = "", field = ""; //the current element, a tempstring, and the field: of the next element
    QStringList tmpl; //list of elements of which at least one has to match (OR)
    QValueList<QStringList> allof; //list of all the tmpls, of which all have to match
    while( !query.isEmpty() )  //seperate query into parts which all have to match
    {
        if( query.startsWith( " " ) )
            query = query.mid( 1 ); //cuts off the first character
        if( query.startsWith( "\"" ) ) //take stuff in "s literally (basically just ends up ignoring spaces)
        {
            query = query.mid( 1 );
            x = query.find( "\"" );
        }
        else
            x = query.find( " " );
        if( x < 0 )
            x = query.length();
        s = query.left( x ); //get the element
        query = query.mid( x + 1 ); //move on

        if( !field.isEmpty() || ( s != "-" && s != "AND" && s != "OR" &&
                                  !s.endsWith( ":" ) && !s.endsWith( ":>" ) && !s.endsWith( ":<" ) ) )
        {
            if( !OR && !tmpl.isEmpty() ) //add the OR list to the AND list
            {
                allof += tmpl;
                tmpl.clear();
            }
            else
                OR = false;
            tmp = field + s;
            if( minus )
            {
                tmp = "-" + tmp;
                minus = false;
            }
            tmpl += tmp;
            tmp = field = "";
        }
        else if( s.endsWith( ":" ) || s.endsWith( ":>" ) || s.endsWith( ":<" ) )
            field = s;
        else if( s == "OR" )
            OR = true;
        else if( s == "-" )
            minus = true;
        else
            OR = false;
    }
    if( !tmpl.isEmpty() )
        allof += tmpl;

    const uint allofcount = allof.count();
    for( uint i = 0; i < allofcount; ++i ) //check each part for matchiness
    {
        beginOR();
        uint count = allof[i].count();
        for( uint ii = 0; ii < count; ++ii )
        {
            field = QString::null;
            s = allof[i][ii];
            bool neg = s.startsWith( "-" );
            if ( neg )
                s = s.mid( 1 ); //cut off the -
            x = s.find( ":" ); //where the field ends and the thing-to-match begins
            if( x > 0 )
            {
                field = s.left( x ).lower();
                s = s.mid( x + 1 );
            }

            int mode = modeNormal;
            if( !field.isEmpty() && s.startsWith( ">" ) )
            {
                s = s.mid( 1 );
                mode = modeGreater;
            }
            else if( !field.isEmpty() && s.startsWith( "<" ) )
            {
                s = s.mid( 1 );
                mode = modeLess;
            }

            int table = -1, value = -1;
            if( field == "artist" )
                table = tabArtist;
            else if( field == "album" )
                table = tabAlbum;
            else if( field == "title" )
                table = tabSong;
            else if( field == "genre" )
                table = tabGenre;
            else if( field == "year" )
                table = tabYear;
            else if( field == "score" )
            {
                table = tabStats;
                value = valScore;
            }
            else if( field == "rating" )
            {
                table = tabStats;
                value = valRating;
            }
            else if( field == "directory" )
            {
                table = tabSong;
                value = valDirectory;
            }
            else if( field == "length" )
            {
                table = tabSong;
                value = valLength;
            }
            else if( field == "playcount" )
            {
                table = tabStats;
                value = valPlayCounter;
            }
            else if( field == "samplerate" )
            {
                table = tabSong;
                value = valSamplerate;
            }
            else if( field == "track" )
            {
                table = tabSong;
                value = valTrack;
            }
            else if( field == "filename" || field == "url" )
            {
                table = tabSong;
                value = valURL;
            }
            else if( field == "filetype" )
            {
                table = tabSong;
                value = valURL;
                mode = modeEndMatch;
                s = "." + s;
            }
            else if( field == "bitrate" )
            {
                table = tabSong;
                value = valBitrate;
            }
            else if( field == "comment" )
            {
                table = tabSong;
                value = valComment;
            }
            else if( field == "lyrics" )
            {
                table = tabLyrics;
                value = valLyrics;
            }

            if( neg )
            {
                if( value >= 0 )
                    excludeFilter( table, value, s, mode );
                else
                    excludeFilter( table >= 0 ? table : defaultTables, s );
            }
            else
            {
                if( value >= 0 )
                    addFilter( table, value, s, mode );
                else
                    addFilter( table >= 0 ? table : defaultTables, s );
            }
        }
        endOR();
    }
}

void
QueryBuilder::addFilter( int tables, const QString& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";

        if (CollectionDB::instance()->getType() == DbConnection::postgresql) {
            if ( tables & tabAlbum ) m_where += "OR album.name ~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
            if ( tables & tabArtist ) m_where += "OR artist.name ~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
            if ( tables & tabGenre ) m_where += "OR genre.name ~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
            if ( tables & tabYear ) m_where += "OR year.name ~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
            if ( tables & tabSong ) m_where += "OR tags.title ~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
        }
        else
        {
            if ( tables & tabAlbum ) m_where += "OR album.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabArtist ) m_where += "OR artist.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabGenre ) m_where += "OR genre.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabYear ) m_where += "OR year.name LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabSong ) m_where += "OR tags.title LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        }

        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::addFilter( int tables, int value, const QString& filter, int mode )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";

        QString m, s;
        if (CollectionDB::instance()->getType() == DbConnection::postgresql)
        {
            m = mode == modeLess ? "<" : mode == modeGreater ? ">" : "~*";
            if( mode == modeEndMatch )
                s = m + " '" + CollectionDB::instance()->escapeString( filter ) + "$' ";
            else
                s = m + " '" + CollectionDB::instance()->escapeString( filter ) + "' ";
        }
        else
        {
            m = mode == modeLess ? "<" : mode == modeGreater ? ">" : "LIKE";
            s = m + " '" + ( m == "LIKE" ? "%" : "" ) + CollectionDB::instance()->escapeString( filter ) + ( mode == modeNormal ? "%" : "" ) + "' ";
        }

        m_where += QString( "OR %1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;

        if ( ( value & valName ) && mode == modeNormal && filter == i18n( "Unknown" ) )
            m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::addFilters( int tables, const QStringList& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + " ";

        for ( uint i = 0; i < filter.count(); i++ )
        {
            m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";
            if (CollectionDB::instance()->getType() == DbConnection::postgresql) {
                if ( tables & tabAlbum ) m_where += "OR album.name ~* '" + CollectionDB::instance()->escapeString( filter[i] ) + "' ";
                if ( tables & tabArtist ) m_where += "OR artist.name ~* '" + CollectionDB::instance()->escapeString( filter[i] ) + "' ";
                if ( tables & tabGenre ) m_where += "OR genre.name ~* '" + CollectionDB::instance()->escapeString( filter[i] ) + "' ";
                if ( tables & tabYear ) m_where += "OR year.name ~* '" + CollectionDB::instance()->escapeString( filter[i] ) + "' ";
                if ( tables & tabSong ) m_where += "OR tags.title ~* '" + CollectionDB::instance()->escapeString( filter[i] ) + "' ";
            }
            else
            {
                if ( tables & tabAlbum ) m_where += "OR album.name LIKE '%" + CollectionDB::instance()->escapeString( filter[i] ) + "%' ";
                if ( tables & tabArtist ) m_where += "OR artist.name LIKE '%" + CollectionDB::instance()->escapeString( filter[i] ) + "%' ";
                if ( tables & tabGenre ) m_where += "OR genre.name LIKE '%" + CollectionDB::instance()->escapeString( filter[i] ) + "%' ";
                if ( tables & tabYear ) m_where += "OR year.name LIKE '%" + CollectionDB::instance()->escapeString( filter[i] ) + "%' ";
                if ( tables & tabSong ) m_where += "OR tags.title LIKE '%" + CollectionDB::instance()->escapeString( filter[i] ) + "%' ";
            }
            m_where += " ) ";
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
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + " ";
        if (CollectionDB::instance()->getType() == DbConnection::postgresql) {
          if ( tables & tabAlbum ) m_where += "AND album.name !~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
          if ( tables & tabArtist ) m_where += "AND artist.name !~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
          if ( tables & tabGenre ) m_where += "AND genre.name !~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
          if ( tables & tabYear ) m_where += "AND year.name !~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
          if ( tables & tabSong ) m_where += "AND tags.title !~* '" + CollectionDB::instance()->escapeString( filter ) + "' ";
        }
        else
        {
            if ( tables & tabAlbum ) m_where += "AND album.name NOT LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabArtist ) m_where += "AND artist.name NOT LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabGenre ) m_where += "AND genre.name NOT LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabYear ) m_where += "AND year.name NOT LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
            if ( tables & tabSong ) m_where += "AND tags.title NOT LIKE '%" + CollectionDB::instance()->escapeString( filter ) + "%' ";
        }
        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::excludeFilter( int tables, int value, const QString& filter, int mode )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + " ";

        QString m, s;
        if (CollectionDB::instance()->getType() == DbConnection::postgresql)
        {
            m = mode == modeLess ? ">=" : mode == modeGreater ? "<=" : "!~*";
            if( mode == modeEndMatch )
                s = m + " '" + CollectionDB::instance()->escapeString( filter ) + "$' ";
            else
                s = m + " '" + CollectionDB::instance()->escapeString( filter ) + "' ";
        }
        else
        {
            m = mode == modeLess ? ">=" : mode == modeGreater ? "<=" : "NOT LIKE";
            s = m + " '" + ( m == "NOT LIKE" ? "%" : "" ) + CollectionDB::instance()->escapeString( filter ) + ( mode == modeNormal ? "%" : "" ) + "' ";
        }

        m_where += QString( "AND %1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;

        if ( ( value & valName ) && mode == modeNormal && filter == i18n( "Unknown" ) )
            m_where += QString( "AND %1.%2 <> '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::addMatch( int tables, const QString& match )
{
    if ( !match.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";
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
QueryBuilder::addMatch( int tables, int value, const QString& match )
{
    if ( !match.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";
        m_where += QString( "OR %1.%2 LIKE '" ).arg( tableName( tables ) ).arg( valueName( value ) ) + CollectionDB::instance()->escapeString( match ) + "' ";

        if ( ( value & valName ) && match == i18n( "Unknown" ) )
            m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

        m_where += " ) ";
    }

    m_linkTables |= tables;
}


void
QueryBuilder::addMatches( int tables, const QStringList& match )
{
    if ( !match.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";

        for ( uint i = 0; i < match.count(); i++ )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabArtist ) m_where += "OR artist.name LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabGenre ) m_where += "OR genre.name LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabYear ) m_where += "OR year.name LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabSong ) m_where += "OR tags.title LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";
            if ( tables & tabStats ) m_where += "OR statistics.url LIKE '" + CollectionDB::instance()->escapeString( match[i] ) + "' ";

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
QueryBuilder::excludeMatch( int tables, const QString& match )
{
    if ( !match.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + " ";
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
QueryBuilder::exclusiveFilter( int tableMatching, int tableNotMatching, int value )
{
    m_join += " LEFT JOIN ";
    m_join += tableName( tableNotMatching );
    m_join += " ON ";

    m_join += tableName( tableMatching ) + ".";
    m_join += valueName( value );
    m_join+= " = ";
    m_join += tableName( tableNotMatching ) + ".";
    m_join += valueName( value );

    m_where += " AND ";
    m_where += tableName( tableNotMatching ) + ".";
    m_where += valueName( value );
    m_where += " IS null ";
}


void
QueryBuilder::setOptions( int options )
{
    if ( options & optNoCompilations || options & optOnlyCompilations )
        m_linkTables |= tabSong;

    if ( options & optNoCompilations ) m_where += QString("AND tags.sampler = %1 ").arg(CollectionDB::instance()->boolF());
    if ( options & optOnlyCompilations ) m_where += QString("AND tags.sampler = %1 ").arg(CollectionDB::instance()->boolT());

    if (CollectionDB::instance()->getType() == DbConnection::postgresql && options & optRemoveDuplicates && options & optRandomize) {
            m_values = "DISTINCT " + CollectionDB::instance()->randomFunc() + " AS __random "+ m_values;
            if ( !m_sort.isEmpty() ) m_sort += ",";
            m_sort += CollectionDB::instance()->randomFunc() + " ";
    } else {
            if ( options & optRemoveDuplicates ) m_values = "DISTINCT " + m_values;

            if ( options & optRandomize )
            {
                if ( !m_sort.isEmpty() ) m_sort += ",";
                m_sort += CollectionDB::instance()->randomFunc() + " ";
            }
    }
}


void
QueryBuilder::sortBy( int table, int value, bool descending )
{
    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valRating || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate || value & valPercentage ||
         table & tabYear )
        b = false;

    if ( !m_sort.isEmpty() ) m_sort += ",";
    if ( b ) m_sort += "LOWER( ";
    if ( table & tabYear ) m_sort += "(";

    m_sort += tableName( table ) + ".";
    m_sort += valueName( value );

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        if ( table & tabYear ) m_sort += ")";
    }
    else
    {
        if ( table & tabYear ) m_sort += "+0)";
    }

    if ( b ) m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        if (!m_values.isEmpty()) m_values += ",";
        if ( b ) m_values += "LOWER( ";
        m_values += tableName( table ) + ".";
        m_values += valueName( value );
        if ( b ) m_values += ")";
        m_values += " as __discard ";
    }

    m_linkTables |= table;
}

void
QueryBuilder::sortByFunction( int function, int table, int value, bool descending )
{
    // This function should be used with the equivalent addReturnFunctionValue (with the same function on same values)
    // since it uses the "func(table.value) AS functablevalue" definition.

    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valRating || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate || value & valPercentage ||
         table & tabYear )
        b = false;

    if ( !m_sort.isEmpty() ) m_sort += ",";
    //m_sort += functionName( function ) + "(";
    if ( b ) m_sort += "LOWER( ";
    if ( table & tabYear ) m_sort += "(";

    QString columnName = functionName( function )+tableName( table )+valueName( value );
    m_sort += columnName;

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        if ( table & tabYear ) m_sort += ")";
    }
    else
    {
        if ( table & tabYear ) m_sort += "+0)";
    }

    if ( b ) m_sort += " ) ";
    //m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        if (m_values.find(columnName) == -1)
        {
            if (!m_values.isEmpty()) m_values += ",";
            if ( b ) m_values += "LOWER( ";
            m_values += tableName( table ) + ".";
            m_values += valueName( value );
            if ( b ) m_values += ")";
            m_values += " as __discard ";
        }
    }

    m_linkTables |= table;
}

void
QueryBuilder::groupBy( int table, int value )
{
    if ( !m_group.isEmpty() ) m_group += ",";
    m_group += tableName( table ) + ".";
    m_group += valueName( value );

    m_linkTables |= table;
}


void
QueryBuilder::setLimit( int startPos, int length )
{
    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        m_limit = QString( " OFFSET %1 LIMIT %2 " ).arg( startPos ).arg( length );
    }
    else
    {
        m_limit = QString( " LIMIT %1, %2 " ).arg( startPos ).arg( length );
    }
}


void
QueryBuilder::initSQLDrag()
{
    clear();
    addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBitrate );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valSamplerate );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
}


void
QueryBuilder::buildQuery()
{
    if ( m_query.isEmpty() )
    {
        linkTables( m_linkTables );

        m_query = "SELECT " + m_values + " FROM " + m_tables + " " + m_join + " WHERE " + CollectionDB::instance()->boolT() + " " + m_where;
        // GROUP BY must be before ORDER BY for sqlite
        if ( !m_group.isEmpty() ) m_query += " GROUP BY " + m_group;
        if ( !m_sort.isEmpty() ) m_query += " ORDER BY " + m_sort;
        m_query += m_limit;
        m_query += ";";
    }
}

// get the builded SQL-Query (used in smartplaylisteditor soon)
QString
QueryBuilder::getQuery()
{
    if ( m_query.isEmpty())
    {
        buildQuery();
    }
    return m_query;
}

QStringList
QueryBuilder::run()
{
    buildQuery();
    //debug() << m_query << endl;
    return CollectionDB::instance()->query( m_query );
}


void
QueryBuilder::clear()
{
    m_query = "";
    m_values = "";
    m_tables = "";
    m_join = "";
    m_where = "";
    m_sort = "";
    m_group = "";
    m_limit = "";

    m_linkTables = 0;
    m_returnValues = 0;
}


QString
QueryBuilder::tableName( int table )
{
    QString tables;

    if (CollectionDB::instance()->getType() != DbConnection::postgresql)
    {
        if ( table & tabSong )   tables += ",tags";
    }
    if ( table & tabArtist ) tables += ",artist";
    if ( table & tabAlbum )  tables += ",album";
    if ( table & tabGenre )  tables += ",genre";
    if ( table & tabYear )   tables += ",year";
    if ( table & tabStats )  tables += ",statistics";
    if ( table & tabLyrics )  tables += ",lyrics";
    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        if ( table & tabSong )   tables += ",tags";
    }

    // when there are multiple tables involved, we always need table tags for linking them
    return tables.mid( 1 );
}


QString
QueryBuilder::valueName( int value )
{
    QString values;

    if ( value & valID )          values += "id";
    if ( value & valName )        values += "name";
    if ( value & valURL )         values += "url";
    if ( value & valDirectory )   values += "dir";
    if ( value & valTitle )       values += "title";
    if ( value & valTrack )       values += "track";
    if ( value & valScore )       values += "percentage";
    if ( value & valRating )      values += "rating";
    if ( value & valComment )     values += "comment";
    if ( value & valLyrics )      values += "lyrics";
    if ( value & valBitrate )     values += "bitrate";
    if ( value & valLength )      values += "length";
    if ( value & valSamplerate )  values += "samplerate";
    if ( value & valPlayCounter ) values += "playcounter";
    if ( value & valAccessDate )  values += "accessdate";
    if ( value & valCreateDate )  values += "createdate";
    if ( value & valPercentage )  values += "percentage";
    if ( value & valArtistID )    values += "artist";
    if ( value & valAlbumID )     values += "album";
    if ( value & valGenreID )     values += "genre";
    if ( value & valYearID )      values += "year";

    return values;
}

QString
QueryBuilder::functionName( int value )
{
    QString function;

    if ( value & funcCount )     function += "Count";
    if ( value & funcMax )       function += "Max";
    if ( value & funcMin )       function += "Min";
    if ( value & funcAvg )       function += "Avg";
    if ( value & funcSum )       function += "Sum";

    return function;
}


#include "collectiondb.moc"
