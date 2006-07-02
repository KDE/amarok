// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// (c) 2005 Ian Monroe <ian@monroe.nu>
// (c) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>
// (c) 2005 Isaiah Damron <xepo@trifault.net>
// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// (c) 2006 Jonas Hurrelmann <j@outpo.st>
// (c) 2006 Shane King <kde@dontletsstart.com>
// (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>
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
#include "expression.h"
#include "mediabrowser.h"
#include "metabundle.h"           //updateTags()
#include "organizecollectiondialog.h"
#include "playlist.h"
#include "playlistloader.h"
#include "playlistbrowser.h"
#include "podcastbundle.h"        //addPodcast
#include "qstringx.h"
#include "scancontroller.h"
#include "scriptmanager.h"
#include "scrobbler.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <qbuffer.h>
#include <qcheckbox.h>
#include <qfile.h>
#include <qmap.h>
#include <qmutex.h>
#include <qregexp.h>              //setHTMLLyrics()
#include <qtimer.h>
#include <qpainter.h>             //createDragPixmap()
#include <qpalette.h>
#include <pthread.h>              //debugging, can be removed later

#include <kcharsets.h>            //setHTMLLyrics()
#include <kcombobox.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kinputdialog.h>         //setupCoverFetcher()
#include <klineedit.h>            //setupCoverFetcher()
#include <klocale.h>
#include <kmdcodec.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kio/job.h>
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
    #include <mysql/mysql_version.h>
#endif

#ifdef USE_POSTGRESQL
    #include <libpq-fe.h>
#endif

#undef HAVE_INOTIFY  // NOTE Disabled for now, due to stability issues

#ifdef HAVE_INOTIFY
    #include <linux/inotify.h>
    #include "inotify/inotify-syscalls.h"
#endif

using amaroK::QStringx;

#define DEBUG 0

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS INotify
//////////////////////////////////////////////////////////////////////////////////////////

INotify* INotify::s_instance = 0;

INotify::INotify( CollectionDB *parent, int fd )
    : DependentJob( parent, "INotify" )
    , m_parent( parent )
    , m_fd( fd )
{
    s_instance = this;
}


INotify::~INotify()
{}


bool
INotify::watchDir( QString directory )
{
#ifdef HAVE_INOTIFY
    int wd = inotify_add_watch( m_fd, directory.local8Bit(), IN_CLOSE_WRITE | IN_DELETE | IN_MOVE |
                                                             IN_MODIFY | IN_ATTRIB );
    if ( wd < 0 )
        debug() << "Could not add INotify watch for: " << directory << endl;

    return ( wd >= 0 );
#else
    Q_UNUSED(directory);
#endif

    return false;
}


bool
INotify::doJob()
{
#ifdef HAVE_INOTIFY
    DEBUG_BLOCK

    const QStringList values = m_parent->query( "SELECT dir FROM directories;" );
    foreach( values )
    {
        const QString dir = *it;
        watchDir( dir );
    }

    /* size of the event structure, not counting name */
    const int EVENT_SIZE = ( sizeof( struct inotify_event ) );
    /* reasonable guess as to size of 1024 events */
    const int BUF_LEN = 1024 * ( EVENT_SIZE + 16 );

    while ( 1 )
    {
        char buf[BUF_LEN];
        int len, i = 0;
        len = read( m_fd, buf, BUF_LEN );
        if ( len < 0 )
        {
            debug() << "Read from INotify failed" << endl;
            return false;
        }
        else
        {
            if ( !len )
            {
                /* BUF_LEN too small? */
            }
            else
            {
                while ( i < len )
                {
                    struct inotify_event *event;
                    event = (struct inotify_event *) &buf[i];

                    i += EVENT_SIZE + event->len;
                }

                QTimer::singleShot( 0, m_parent, SLOT( scanMonitor() ) );
            }
        }
    }
#endif

    // this shouldn't happen
    return false;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionDB
//////////////////////////////////////////////////////////////////////////////////////////

QMutex* CollectionDB::connectionMutex = new QMutex();
//we don't have to worry about this map leaking memory since ThreadWeaver limits the total
//number of QThreads ever created
QMap<QThread *, DbConnection *> *CollectionDB::threadConnections = new QMap<QThread *, DbConnection *>();

CollectionDB* CollectionDB::instance()
{
    static CollectionDB db;
    return &db;
}


CollectionDB::CollectionDB()
        : EngineObserver( EngineController::instance() )
        , m_autoScoring( true )
        , m_noCover( locate( "data", "amarok/images/nocover.png" ) )
        , m_scanInProgress( false )
        , m_rescanRequired( false )
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
    // The asterisk is for also deleting the shadow-caches.
    const QStringList entryList = cacheCoverDir().entryList( "*nocover.png*", QDir::Files );
    foreach( entryList )
        cacheCoverDir().remove( *it );

    // TODO Should write to config in dtor, but it crashes...
    KConfig* config = amaroK::config( "Collection Browser" );
    config->writeEntry( "Database Version", DATABASE_VERSION );
    config->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );
    config->writeEntry( "Database Persistent Tables Version", DATABASE_PERSISTENT_TABLES_VERSION );
    config->writeEntry( "Database Podcast Tables Version", DATABASE_PODCAST_TABLES_VERSION );
    config->writeEntry( "Database ATF Version", DATABASE_ATF_VERSION );

    setAdminValue( "Database Version", QString::number( DATABASE_VERSION ) );
    setAdminValue( "Database Stats Version", QString::number( DATABASE_STATS_VERSION ) );
    setAdminValue( "Database Persistent Tables Version", QString::number( DATABASE_PERSISTENT_TABLES_VERSION ) );
    setAdminValue( "Database Podcast Tables Version", QString::number( DATABASE_PODCAST_TABLES_VERSION ) );
    setAdminValue( "Database ATF Version", QString::number( DATABASE_ATF_VERSION ) );

    KConfig* atfconfig = amaroK::config( "Collection" );
    m_atfEnabled = atfconfig->readBoolEntry( "AdvancedTagFeatures", false );
    if( m_atfEnabled )
    {
        connect( this, SIGNAL(fileMoved(const QString&, const QString&, const QString&)),
                 this, SLOT(atfMigrateStatisticsUrl(const QString&, const QString&, const QString&)) );
        //enable once 1.4.1's out the door, requires stats table to have uniqueid column first
        //connect( this, SIGNAL(uniqueIdChanged(const QString&, const QString&, const QString&)),
                 //this, SLOT(atfMigrateStatisticsUniqueId(const QString&, const QString&, const QString&)) );
    }

    connect( qApp, SIGNAL( aboutToQuit() ), this, SLOT( disableAutoScoring() ) );

#ifdef HAVE_INOTIFY
    // Try to initialize inotify, if not available use the old timer approach.
    int inotify_fd = inotify_init();
    if ( inotify_fd < 0 )
#endif
    {
        debug() << "INotify not available, using QTimer!" << endl;
        startTimer( MONITOR_INTERVAL * 1000 );
    }
#ifdef HAVE_INOTIFY
    else
    {
        debug() << "INotify enabled!" << endl;
        ThreadWeaver::instance()->onlyOneJob( new INotify( this, inotify_fd ) );
    }
#endif

    connect( this, SIGNAL( coverRemoved( const QString&, const QString& ) ),
                   SIGNAL( coverChanged( const QString&, const QString& ) ) );
    connect( Scrobbler::instance(), SIGNAL( similarArtistsFetched( const QString&, const QStringList& ) ),
             this,                    SLOT( similarArtistsFetched( const QString&, const QStringList& ) ) );

    // If we're supposed to monitor dirs for changes, make sure we run it once
    // on startup, since inotify can't inform us about old events
//     QTimer::singleShot( 0, this, SLOT( scanMonitor() ) );
}


CollectionDB::~CollectionDB()
{
    DEBUG_BLOCK

#ifdef HAVE_INOTIFY
    if ( INotify::instance()->fd() >= 0 )
        close( INotify::instance()->fd() );
#endif

    if ( getDbConnectionType() == DbConnection::sqlite )
    {
        debug() << "Running VACUUM" << endl;
        query( "VACUUM;" );
    }

    destroy();
}


inline QString
CollectionDB::exactCondition( const QString &right )
{
    if ( DbConnection::mysql == instance()->getDbConnectionType() )
        return QString( "= BINARY '" + instance()->escapeString( right ) + "'" );
    else
        return QString( "= '" + instance()->escapeString( right ) + "'" );
}


QString
CollectionDB::likeCondition( const QString &right, bool anyBegin, bool anyEnd )
{
    QString escaped = right;
    escaped.replace( '/', "//" ).replace( '%', "/%" ).replace( '_', "/_" );
    escaped = instance()->escapeString( escaped );

    QString ret;
    if ( DbConnection::postgresql == instance()->getDbConnectionType() )
        ret = " ILIKE "; //case-insensitive according to locale
    else
        ret = " LIKE ";

    ret += "'";
    if ( anyBegin )
            ret += "%";
    ret += escaped;
    if ( anyEnd )
            ret += "%";
    ret += "'";

    //Use / as the escape character
    ret += " ESCAPE '/' ";

    return ret;
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
    if ( statement.stripWhiteSpace().isEmpty() )
        return QStringList();

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

    values = query( "SELECT COUNT( url ) FROM tags LIMIT 1 OFFSET 0;" );

    return values.isEmpty() ? true : values.first() == "0";
}


bool
CollectionDB::isValid( )
{
    QStringList values1;
    QStringList values2;
    QStringList values3;
    QStringList values4;

    values1 = query( "SELECT COUNT( url ) FROM tags LIMIT 1 OFFSET 0;" );
    values2 = query( "SELECT COUNT( url ) FROM statistics LIMIT 1 OFFSET 0;" );
    values3 = query( "SELECT COUNT( url ) FROM podcastchannels LIMIT 1 OFFSET 0;" );
    values4 = query( "SELECT COUNT( url ) FROM podcastepisodes LIMIT 1 OFFSET 0;" );

    //It's valid as long as we've got _some_ tables that have something in.
    return !( values1.isEmpty() && values2.isEmpty() && values3.isEmpty() && values4.isEmpty() );
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
    DEBUG_BLOCK

    //create tag table
    query( QString( "CREATE %1 TABLE tags%2 ("
                    "url " + textColumnType() + ","
                    "dir " + textColumnType() + ","
                    "createdate INTEGER,"
                    "modifydate INTEGER,"
                    "album INTEGER,"
                    "artist INTEGER,"
                    "genre INTEGER,"
                    "title " + textColumnType() + ","
                    "year INTEGER,"
                    "comment " + textColumnType() + ","
                    "track NUMERIC(4),"
                    "composer " + textColumnType() + ","
                    "discnumber INTEGER,"
                    "bitrate INTEGER,"
                    "length INTEGER,"
                    "samplerate INTEGER,"
                    "filesize INTEGER,"
                    "filetype INTEGER,"
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
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" )
                    .arg( yearAutoIncrement ) );

    //create images table
    query( QString( "CREATE %1 TABLE images%2 ("
                    "path " + textColumnType() + ","
                    "artist " + textColumnType() + ","
                    "album " + textColumnType() + ");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create embed table
    query( QString( "CREATE %1 TABLE embed%2 ("
                    "url " + textColumnType() + ","
                    "hash " + textColumnType() + ","
                    "description " + textColumnType() + ");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    // create directory statistics table
    query( QString( "CREATE %1 TABLE directories%2 ("
                    "dir " + textColumnType() + " UNIQUE,"
                    "changedate INTEGER );" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create uniqueid table
    query( QString( "CREATE %1 TABLE uniqueid%2 ("
                    "url " + textColumnType() + ","
                    "uniqueid " + textColumnType(8) + " UNIQUE,"
                    "dir " + textColumnType() + ");" )
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
        query( "CREATE INDEX related_artists_artist ON related_artists( artist );" );

        createIndices();
    }
}

void
CollectionDB::createIndices()
{
    query( "CREATE INDEX url_tag ON tags( url );" );
    query( "CREATE INDEX album_tag ON tags( album );" );
    query( "CREATE INDEX artist_tag ON tags( artist );" );
    query( "CREATE INDEX genre_tag ON tags( genre );" );
    query( "CREATE INDEX year_tag ON tags( year );" );
    query( "CREATE INDEX sampler_tag ON tags( sampler );" );

    query( "CREATE INDEX images_album ON images( album );" );
    query( "CREATE INDEX images_artist ON images( artist );" );

    query( "CREATE INDEX embed_url ON embed( url );" );
    query( "CREATE INDEX embed_hash ON embed( hash );" );

    query( "CREATE INDEX directories_dir ON directories( dir );" );
    query( "CREATE INDEX uniqueid_uniqueid ON uniqueid( uniqueid );");
    query( "CREATE INDEX uniqueid_url ON uniqueid( url );");
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
    query( QString( "DROP TABLE embed%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE directories%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE uniqueid%1;" ).arg( temporary ? "_temp" : "" ) );
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
    query( QString( "%1 embed%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 directories%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 uniqueid%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    if ( !temporary )
    {
        query( QString( "%1 related_artists;" ).arg( clearCommand ) );
    }
}


void
CollectionDB::copyTempTables( )
{
    DEBUG_BLOCK

    insert( "INSERT INTO tags SELECT * FROM tags_temp;", NULL );
    insert( "INSERT INTO album SELECT * FROM album_temp;", NULL );
    insert( "INSERT INTO artist SELECT * FROM artist_temp;", NULL );
    insert( "INSERT INTO genre SELECT * FROM genre_temp;", NULL );
    insert( "INSERT INTO year SELECT * FROM year_temp;", NULL );
    insert( "INSERT INTO images SELECT * FROM images_temp;", NULL );
    insert( "INSERT INTO embed SELECT * FROM embed_temp;", NULL );
    insert( "INSERT INTO directories SELECT * FROM directories_temp;", NULL );
    insert( "INSERT INTO uniqueid SELECT * FROM uniqueid_temp;", NULL );
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
            "url " + textColumnType() + ", "
            "lyrics " + longTextColumnType() + ");" ) );

    // create labels table
    query( QString( "CREATE TABLE label ("
        "url " + textColumnType() + ","
        "label " + textColumnType() + ");" ) );

    query( QString( "CREATE TABLE playlists ("
            "playlist " + textColumnType() + ", "
            "url " + textColumnType() + ", "
            "tracknum INTEGER );" ) );

    query( "CREATE INDEX url_label ON label( url );" );
    query( "CREATE INDEX label_label ON label( label );" );
    query( "CREATE INDEX playlist_playlists ON playlists( playlist );" );
    query( "CREATE INDEX url_playlists ON playlists( url );" );
}

void
CollectionDB::createPodcastTables()
{
    QString podcastAutoIncrement = "";
    QString podcastFolderAutoInc = "";
    if ( getDbConnectionType() == DbConnection::postgresql )
    {
        query( QString( "CREATE SEQUENCE podcastepisode_seq;" ) );

        query( QString( "CREATE SEQUENCE podcastfolder_seq;" ) );

        podcastAutoIncrement = QString("DEFAULT nextval('podcastepisode_seq')");
        podcastFolderAutoInc = QString("DEFAULT nextval('podcastfolder_seq')");
    }
    else if ( getDbConnectionType() == DbConnection::mysql )
    {
        podcastAutoIncrement = "AUTO_INCREMENT";
        podcastFolderAutoInc = "AUTO_INCREMENT";
    }

    // create podcast channels table
    query( QString( "CREATE TABLE podcastchannels ("
                    "url " + textColumnType() + " UNIQUE,"
                    "title " + textColumnType() + ","
                    "weblink " + textColumnType() + ","
                    "image " + textColumnType() + ","
                    "comment " + longTextColumnType() + ","
                    "copyright "  + textColumnType() + ","
                    "parent INTEGER,"
                    "directory "  + textColumnType() + ","
                    "autoscan BOOL, fetchtype INTEGER, "
                    "autotransfer BOOL, haspurge BOOL, purgecount INTEGER );" ) );

    // create podcast episodes table
    query( QString( "CREATE TABLE podcastepisodes ("
                    "id INTEGER PRIMARY KEY %1, "
                    "url " + textColumnType() + " UNIQUE,"
                    "localurl " + textColumnType() + ","
                    "parent " + textColumnType() + ","
                    "guid " + textColumnType() + ","
                    "title " + textColumnType() + ","
                    "subtitle " + textColumnType() + ","
                    "composer " + textColumnType() + ","
                    "comment " + longTextColumnType() + ","
                    "filetype "  + textColumnType() + ","
                    "createdate "  + textColumnType() + ","
                    "length INTEGER,"
                    "size INTEGER,"
                    "isNew BOOL );" )
                    .arg( podcastAutoIncrement ) );

    // create podcast folders table
    query( QString( "CREATE TABLE podcastfolders ("
                    "id INTEGER PRIMARY KEY %1, "
                    "name " + textColumnType() + ","
                    "parent INTEGER, isOpen BOOL );" )
                    .arg( podcastFolderAutoInc ) );

    query( "CREATE INDEX url_podchannel ON podcastchannels( url );" );
    query( "CREATE INDEX url_podepisode ON podcastepisodes( url );" );
    query( "CREATE INDEX localurl_podepisode ON podcastepisodes( localurl );" );
    query( "CREATE INDEX url_podfolder ON podcastfolders( id );" );
}


void
CollectionDB::dropPersistentTables()
{
    query( "DROP TABLE amazon;" );
    query( "DROP TABLE lyrics;" );
    query( "DROP TABLE label;" );
    query( "DROP TABLE playlists;" );
}


void
CollectionDB::dropPodcastTables()
{
    query( "DROP TABLE podcastchannels;" );
    query( "DROP TABLE podcastepisodes;" );
    query( "DROP TABLE podcastfolders;" );
}


uint
CollectionDB::artistID( QString value, bool autocreate, const bool temporary, bool exact /* = true */ )
{
    // lookup cache
    if ( m_validArtistCache && m_cacheArtist[(int)temporary] == value )
        return m_cacheArtistID[(int)temporary];

    uint id;
    if ( exact )
        id = IDfromExactValue( "artist", value, autocreate, temporary ).toUInt();
    else
        id = IDFromValue( "artist", value, autocreate, temporary );

    // cache values
    m_cacheArtist[(int)temporary] = value;
    m_cacheArtistID[(int)temporary] = id;
    m_validArtistCache = 1;

    return id;
}


QString
CollectionDB::artistValue( uint id )
{
    // lookup cache
    if ( m_cacheArtistID[0] == id )
        return m_cacheArtist[0];

    QString value = valueFromID( "artist", id );

    // cache values
    m_cacheArtist[0] = value;
    m_cacheArtistID[0] = id;

    return value;
}


uint
CollectionDB::albumID( QString value, bool autocreate, const bool temporary, bool exact /* = true */ )
{
    // lookup cache
    if ( m_validAlbumCache && m_cacheAlbum[(int)temporary] == value )
        return m_cacheAlbumID[(int)temporary];

    uint id;
    if ( exact )
        id = IDfromExactValue( "album", value, autocreate, temporary ).toUInt();
    else
        id = IDFromValue( "album", value, autocreate, temporary );

    // cache values
    m_cacheAlbum[(int)temporary] = value;
    m_cacheAlbumID[(int)temporary] = id;
    m_validAlbumCache = 1;

    return id;
}

QString
CollectionDB::albumValue( uint id )
{
    // lookup cache
    if ( m_cacheAlbumID[0] == id )
        return m_cacheAlbum[0];

    QString value = valueFromID( "album", id );

    // cache values
    m_cacheAlbum[0] = value;
    m_cacheAlbumID[0] = id;

    return value;
}

uint
CollectionDB::genreID( QString value, bool autocreate, const bool temporary, bool exact /* = true */ )
{
    return exact ?
        IDfromExactValue( "genre", value, autocreate, temporary ).toUInt() :
        IDFromValue( "genre", value, autocreate, temporary );
}

QString
CollectionDB::genreValue( uint id )
{
    return valueFromID( "genre", id );
}


uint
CollectionDB::yearID( QString value, bool autocreate, const bool temporary, bool exact /* = true */ )
{
    return exact ?
        IDfromExactValue( "year", value, autocreate, temporary ).toUInt() :
        IDFromValue( "year", value, autocreate, temporary );
}


QString
CollectionDB::yearValue( uint id )
{
    return valueFromID( "year", id );
}


uint
CollectionDB::IDFromValue( QString name, QString value, bool autocreate, const bool temporary )
{
    if ( temporary )
        name.append( "_temp" );
//    what the hell is the reason for this?
//    else
//        conn = NULL;

    QStringList values =
        query( QString(
            "SELECT id, name FROM %1 WHERE name %2;" )
            .arg( name )
            .arg( CollectionDB::likeCondition( value ) ) );

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
    Q_INT64 matchType;
    if ( isValue )
        matchType = QueryBuilder::valName;
    else
        matchType = QueryBuilder::valID;
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addMatch( QueryBuilder::tabAlbum, matchType, album_id );
    qb.addMatch( QueryBuilder::tabArtist, matchType, artist_id );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    QStringList ret = qb.run();

    uint returnValues = qb.countReturnValues();
    if ( returnValues > 1 )
    {
        QStringList ret2;
        for ( QStringList::size_type i = 0; i < ret.size(); i += returnValues )
            ret2 << ret[ i ];
        return ret2;
    }
    else
        return ret;
}

QStringList
CollectionDB::albumDiscTracks( const QString &artist_id, const QString &album_id, const QString &discNumber)
{
    if (getDbConnectionType() == DbConnection::postgresql)
        return query( QString( "SELECT tags.url, tags.track AS __discard FROM tags, year WHERE tags.album = %1 AND "
                            "tags.artist = %2 AND year.id = tags.year AND tags.discnumber = %3 ORDER BY tags.track;" )
                  .arg( album_id )
                  .arg( artist_id )
                  .arg( discNumber ) );
    else
        return query( QString( "SELECT tags.url FROM tags, year WHERE tags.album = %1 AND "
                            "tags.artist = %2 AND year.id = tags.year AND tags.discnumber = %3 ORDER BY tags.track;" )
                  .arg( album_id )
                  .arg( artist_id )
                  .arg( discNumber ) );

}

QStringList
CollectionDB::artistTracks( const QString &artist_id )
{
    return query( QString( "SELECT tags.url FROM tags, album "
                "WHERE tags.artist = '%1' AND album.id = tags.album "
                "ORDER BY album.name, tags.discnumber, tags.track;" )
            .arg( artist_id ) );
}


void
CollectionDB::addImageToAlbum( const QString& image, QValueList< QPair<QString, QString> > info, const bool temporary )
{
    for ( QValueList< QPair<QString, QString> >::ConstIterator it = info.begin(); it != info.end(); ++it )
    {
        if ( (*it).first.isEmpty() || (*it).second.isEmpty() )
            continue;

//         debug() << "Added image for album: " << (*it).first << " - " << (*it).second << ": " << image << endl;
        insert( QString( "INSERT INTO images%1 ( path, artist, album ) VALUES ( '%2', '%3', '%4' );" )
              .arg( temporary ? "_temp" : "",
                 escapeString( image ),
                 escapeString( (*it).first ),
                 escapeString( (*it).second ) ), NULL );
    }
}

void
CollectionDB::addEmbeddedImage( const QString& path, const QString& hash, const QString& description )
{
//     debug() << "Added embedded image hash " << hash << " for file " << path << endl;
    insert( QString( "INSERT INTO embed_temp ( url, hash, description ) VALUES ( '%1', '%2', '%3' );" )
     .arg( escapeString( path ),
        escapeString( hash ),
        escapeString( description ) ), NULL );
}

void
CollectionDB::removeOrphanedEmbeddedImages()
{
    // do it the hard way, since a delete subquery wont work on MySQL
    QStringList orphaned = query( "SELECT embed.url FROM embed LEFT JOIN tags ON embed.url = tags.url WHERE tags.url IS NULL;" );
    foreachType( QStringList, orphaned ) {
        query( QString( "DELETE FROM embed WHERE embed.url = '%1';" )
                .arg( escapeString( *it ) ) );
    }
}

QPixmap
CollectionDB::createDragPixmapFromSQL( const QString &sql, QString textOverRide )
{
    // it is too slow to check if the url is actually in the colleciton.
    QStringList values = instance()->query( sql );
    KURL::List list;
    foreach( values )
    {
        KURL u = KURL::fromPathOrURL( *it );
        if( u.isValid() )
            list += u;
    }
    return createDragPixmap( list, textOverRide );
}

QPixmap
CollectionDB::createDragPixmap( const KURL::List &urls, QString textOverRide )
{
    // settings
    const int maxCovers = 4; // maximum number of cover images to show
    const int coverSpacing = 20; // spacing between stacked covers
    const int fontSpacing = 5; // spacing between covers and info text
    const int coverW = AmarokConfig::coverPreviewSize() > 100 ? 100 : AmarokConfig::coverPreviewSize();
    const int coverH = coverW;
    const int margin = 2; //px margin

    int covers      = 0;
    int songs       = 0;
    int pixmapW     = 0;
    int pixmapH     = 0;
    int remoteUrls  = 0;
    int playlists   = 0;

    QMap<QString, int> albumMap;
    QPixmap coverPm[maxCovers];

    QString song, album;


    // iterate urls, get covers and count artist/albums
    bool correctAlbumCount = true;
    KURL::List::ConstIterator it = urls.begin();
    for ( ; it != urls.end(); ++it )
    {
        if( PlaylistFile::isPlaylistFile( *it )
            || (*it).protocol() == "playlist" || (*it).protocol() == "smartplaylist"
            || (*it).protocol() == "dynamic" )
        {
            playlists++;
        }
        else if( (*it).isLocalFile() )
        {
            songs++;

            if( covers >= maxCovers )
            {
                correctAlbumCount = false;
                continue;
            }

            MetaBundle mb( *it );

            song = mb.title();
            album = mb.album();

            if( !albumMap.contains( mb.artist() + mb.album() ) )
            {
                albumMap[ mb.artist() + mb.album() ] = 1;
                QString coverName = CollectionDB::instance()->albumImage( mb.artist(), mb.album(), false, coverW );

                if ( !coverName.endsWith( "@nocover.png" ) )
                    coverPm[covers++].load( coverName );
            }
        }
        else
        {
            MetaBundle mb( *it );
            if( !albumMap.contains( mb.artist() + mb.album() ) )
            {
                albumMap[ mb.artist() + mb.album() ] = 1;
                QString coverName = CollectionDB::instance()->podcastImage( mb, false, coverW );

                if ( !coverName.endsWith( "@nocover.png" ) )
                    coverPm[covers++].load( coverName );
            }
            remoteUrls++;
        }
    }

    // make better text...
    int albums = albumMap.count();
    QString text;

    if( !textOverRide.isEmpty() )
    {
        text = textOverRide;
    }
    else if( ( songs && remoteUrls ) ||
             ( songs && playlists  ) ||
             ( playlists && remoteUrls ) )
    {
        text = i18n( "One item", "%n items", songs + remoteUrls + playlists );
    }
    else if( songs > 0 )
    {
        if( correctAlbumCount ) {
            text = i18n( "X songs from X albums", "%2 from %1" );
            text = text.arg( albums == 1 && !album.isEmpty() ? album : i18n( "one album", "%n albums",albums ) );
        }
        else
            text = "%1";
        text = text.arg( songs == 1 && !song.isEmpty() ? song : i18n( "One song", "%n songs", songs ) );
    }
    else if( playlists > 0 )
        text = i18n( "One playlist", "%n playlists", playlists );
    else if ( remoteUrls > 0 )
        text = i18n( "One remote file", "%n remote files", remoteUrls );
    else
        text = i18n( "Unknown item" );

    QFont font;
    QFontMetrics fm( font );
    int fontH = fm.height() + margin;
    int minWidth = fm.width( text ) + margin*2; //margin either side

    if ( covers > 0 )
    {
        // insert "..." cover as first image if appropiate
        if ( covers < albums )
        {
            if ( covers < maxCovers ) covers++;
            for ( int i = maxCovers-1; i > 0; i-- )
                coverPm[i] = coverPm[i-1];

            QImage im( locate( "data","amarok/images/more_albums.png" ) );
            coverPm[0].convertFromImage( im.smoothScale( coverW, coverH, QImage::ScaleMin ) );
        }

        pixmapH = coverPm[0].height();
        pixmapW = coverPm[0].width();

        // caluclate pixmap height
        int dW, dH;
        for ( int i = 1; i < covers; i++ )
        {
            dW = coverPm[i].width() - coverPm[i-1].width() + coverSpacing;
            dH = coverPm[i].height() - coverPm[i-1].height() + coverSpacing;
            if ( dW > 0 ) pixmapW += dW;
            if ( dH > 0 ) pixmapH += dH;
        }
        pixmapH += fontSpacing + fontH;

        if ( pixmapW < minWidth )
            pixmapW = minWidth;
    }
    else
    {
        pixmapW = minWidth;
        pixmapH = fontH;
    }

    QPixmap pmdrag( pixmapW, pixmapH );
    QPixmap pmtext( pixmapW, fontH );

    QPalette palette = QToolTip::palette();

    QPainter p;
    p.begin( &pmtext );
    p.fillRect( 0, 0, pixmapW, fontH, QBrush( Qt::black ) ); // border
    p.fillRect( 1, 1, pixmapW-margin, fontH-margin, palette.brush( QPalette::Normal, QColorGroup::Background ) );
    p.setBrush( palette.color( QPalette::Normal, QColorGroup::Text ) );
    p.setFont( font );
    p.drawText( margin, fm.ascent() + 1, text );
    p.end();

    QBitmap pmtextMask(pixmapW, fontH);
    pmtextMask.fill( Qt::color1 );

    // when we have found no covers, just display the text message
    if( !covers )
    {
        pmtext.setMask(pmtextMask);
        return pmtext;
    }

    // compose image
    p.begin( &pmdrag );
    p.setBackgroundMode( Qt::TransparentMode );
    for ( int i = 0; i < covers; i++ )
        bitBlt( &pmdrag, i * coverSpacing, i * coverSpacing, &coverPm[i], 0, Qt::CopyROP );

    bitBlt( &pmdrag, 0, pixmapH - fontH, &pmtext, 0, Qt::CopyROP );
    p.end();

    QBitmap pmdragMask( pmdrag.size(), true );
    for ( int i = 0; i < covers; i++ )
    {
        QBitmap coverMask( coverPm[i].width(), coverPm[i].height() );
        coverMask.fill( Qt::color1 );
        bitBlt( &pmdragMask, i * coverSpacing, i * coverSpacing, &coverMask, 0, Qt::CopyROP );
    }
    bitBlt( &pmdragMask, 0, pixmapH - fontH, &pmtextMask, 0, Qt::CopyROP );
    pmdrag.setMask( pmdragMask );

    return pmdrag;
}

QImage
CollectionDB::fetchImage( const KURL& url, QString &/*tmpFile*/ )
{
    if ( url.protocol() != "file" )
    {
        QString tmpFile;
        KIO::NetAccess::download( url, tmpFile, 0 ); //TODO set 0 to the window, though it probably doesn't really matter
        return QImage( tmpFile );
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
CollectionDB::podcastImage( const MetaBundle &bundle, const bool withShadow, uint width )
{
    PodcastEpisodeBundle peb;
    PodcastChannelBundle pcb;

    KURL url = bundle.url().url();

    if( getPodcastEpisodeBundle( url, &peb ) )
    {
        url = peb.parent().url();
    }

    if( getPodcastChannelBundle( url, &pcb ) )
    {
        if( pcb.imageURL().isValid() )
            return podcastImage( pcb.imageURL().url(), withShadow, width );
    }

    return notAvailCover( withShadow, width );
}


QString
CollectionDB::podcastImage( const QString &remoteURL, const bool withShadow, uint width )
{
    // we aren't going to need a 1x1 size image. this is just a quick hack to be able to show full size images.
    // width of 0 == full size
    if( width == 1 )
        width = AmarokConfig::coverPreviewSize();

    QString s = findAmazonImage( "Podcast", remoteURL, width );

    if( s.isEmpty() )
    {
        s = notAvailCover( withShadow, width );

        const KURL url = KURL::fromPathOrURL( remoteURL );
        if( url.isValid() ) //KIO crashes with invalid URLs
        {
            KIO::Job *job = KIO::storedGet( url, false, false );
            m_podcastImageJobs[job] = remoteURL;
            connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( podcastImageResult( KIO::Job* ) ) );
        }
    }

    if ( withShadow )
        s = makeShadowedImage( s );

    return s;
}

void
CollectionDB::podcastImageResult( KIO::Job *gjob )
{
    QString url = m_podcastImageJobs[gjob];
    m_podcastImageJobs.remove( gjob );

    KIO::StoredTransferJob *job = dynamic_cast<KIO::StoredTransferJob *>( gjob );
    if( !job )
    {
        debug() << "connected to wrong job type" << endl;
        return;
    }

    if( job->error() )
    {
        debug() << "job finished with error" << endl;
        return;
    }

    if( job->isErrorPage() )
    {
        debug() << "error page" << endl;
        return;
    }

    QImage image( job->data() );
    if( !image.isNull() )
    {
       if( url.isEmpty() )
          url = job->url().url();

        QCString key = md5sum( "Podcast", url );
        if( image.save( largeCoverDir().filePath( key ), "PNG") )
           emit imageFetched( url );
    }
}


QString
CollectionDB::albumImage( const QString &artist, const QString &album, bool withShadow, uint width, bool* embedded )
{
    QString s;
    // we aren't going to need a 1x1 size image. this is just a quick hack to be able to show full size images.
    // width of 0 == full size
    if( width == 1 )
        width = AmarokConfig::coverPreviewSize();
    if( embedded )
        *embedded = false;

    s = findAmazonImage( artist, album, width );

    if( s.isEmpty() )
        s = findAmazonImage( "", album, width );

    if( s.isEmpty() )
        s = findDirectoryImage( artist, album, width );

    if( s.isEmpty() )
    {
        s = findEmbeddedImage( artist, album, width );
        if( embedded && !s.isEmpty() )
            *embedded = true;
    }

    if( s.isEmpty() )
        s = notAvailCover( withShadow, width );

    if ( withShadow )
        s = makeShadowedImage( s );

    return s;
}


QString
CollectionDB::albumImage( const uint artist_id, const uint album_id, bool withShadow, uint width, bool* embedded )
{
    return albumImage( artistValue( artist_id ), albumValue( album_id ), withShadow, width, embedded );
}


QString
CollectionDB::albumImage( MetaBundle trackInformation, bool withShadow, uint width, bool* embedded )
{
    QString s;
    if( width == 1 )
        width = AmarokConfig::coverPreviewSize();

    QString album = trackInformation.album();
    QString artist = trackInformation.artist();

    // this art is per track, so should check for it first
    s = findMetaBundleImage( trackInformation, width );
    if( embedded )
        *embedded = !s.isEmpty();

    if( s.isEmpty() )
        s = findAmazonImage( artist, album, width );

    if( s.isEmpty() )
        s = findAmazonImage( "", album, width );

    if( s.isEmpty() )
        s = findDirectoryImage( artist, album, width );

    if( s.isEmpty() )
        s = notAvailCover( withShadow, width );

    if ( withShadow )
        s = makeShadowedImage( s );

    return s;
}


QString
CollectionDB::makeShadowedImage( const QString& albumImage, bool cache )
{
    const QImage original( albumImage );

    if( original.hasAlphaBuffer() )
        return albumImage;

    const QFileInfo fileInfo( albumImage );
    const uint shadowSize = static_cast<uint>( original.width() / 100.0 * 6.0 );
    const QString cacheFile = fileInfo.fileName() + "@shadow";
    QImage shadow;

    if ( !cache && cacheCoverDir().exists( cacheFile ) )
        return cacheCoverDir().filePath( cacheFile );

    const QString folder = amaroK::saveLocation( "covershadow-cache/" );
    const QString file = QString( "shadow_albumcover%1x%2.png" ).arg( original.width() + shadowSize ).arg( original.height() + shadowSize  );
    if ( QFile::exists( folder + file ) )
        shadow.load( folder + file );
    else {
        shadow.load( locate( "data", "amarok/images/shadow_albumcover.png" ) );
        shadow = shadow.smoothScale( original.width() + shadowSize, original.height() + shadowSize );
        shadow.save( folder + file, "PNG" );
    }

    QImage target( shadow );
    bitBlt( &target, 0, 0, &original );

    if ( cache ) {
        target.save( cacheCoverDir().filePath( cacheFile ), "PNG" );
        return cacheCoverDir().filePath( cacheFile );
    }

    target.save( albumImage, "PNG" );
    return albumImage;
}


// Amazon Image
QString
CollectionDB::findAmazonImage( const QString &artist, const QString &album, uint width )
{
    QCString widthKey = makeWidthKey( width );

    if ( artist.isEmpty() && album.isEmpty() )
        return QString::null;

    QCString key = md5sum( artist, album );

    // check cache for existing cover
    if ( cacheCoverDir().exists( widthKey + key ) )
        return cacheCoverDir().filePath( widthKey + key );

    // we need to create a scaled version of this cover
    QDir imageDir = largeCoverDir();
    if ( imageDir.exists( key ) )
    {
        if ( width > 1 )
        {
            QImage img( imageDir.filePath( key ) );
            img.smoothScale( width, width, QImage::ScaleMin ).save( cacheCoverDir().filePath( widthKey + key ), "PNG" );

            return cacheCoverDir().filePath( widthKey + key );
        }
        else
            return imageDir.filePath( key );
    }

    return QString::null;
}


QString
CollectionDB::findDirectoryImage( const QString& artist, const QString& album, uint width )
{
    if ( width == 1 )
        width = AmarokConfig::coverPreviewSize();
    QCString widthKey = makeWidthKey( width );

    if ( album.isEmpty() )
        return QString::null;

    QStringList values;
    if ( artist == i18n( "Various Artists" ) )
    {
         values = query( QString(
            "SELECT path FROM images, artist, tags "
            "WHERE images.artist = artist.name "
            "AND artist.id = tags.artist "
            "AND tags.sampler = %1 "
            "AND images.album %2 " )
                 .arg( boolT() )
                 .arg( CollectionDB::likeCondition( album ) ) );
    }
    else
    {
        values = query( QString(
            "SELECT path FROM images WHERE artist %1 AND album %2 ORDER BY path;" )
            .arg( CollectionDB::likeCondition( artist ),
                  CollectionDB::likeCondition( album ) ) );
    }

    if ( !values.isEmpty() )
    {
        QString image( values.first() );
        uint matches = 0;
        uint maxmatches = 0;
        QRegExp iTunesArt( "^AlbumArt_.*Large" );
        for ( uint i = 0; i < values.count(); i++ )
        {
            matches = values[i].contains( "front", false ) + values[i].contains( "cover", false ) + values[i].contains( "folder", false ) + values[i].contains( iTunesArt );
            if ( matches > maxmatches )
            {
                image = values[i];
                maxmatches = matches;
            }
        }

        QCString key = md5sum( artist, album, image );

        if ( width > 1 )
        {
            QImage img = QImage( image );
            QString path = cacheCoverDir().filePath( widthKey + key );
            img.smoothScale( width, width, QImage::ScaleMin ).save( path, "PNG" );

            return path;
        }
        else //large image
            return image;
    }
    return QString::null;
}


QString
CollectionDB::findEmbeddedImage( const QString& artist, const QString& album, uint width )
{
    // In the case of multiple embedded images, we arbitrarily choose one from the newest file
    // could potentially select multiple images within a file based on description, although a
    // lot of tagging software doesn't fill in that field, so we just get whatever the DB
    // happens to return for us
    QStringList values;
    if ( artist == i18n("Various Artists") ) {
        // VAs need special handling to not match on artist name but instead check for sampler flag
        values = query( QString(
                "SELECT embed.hash, embed.url FROM tags, embed, album "
                "WHERE tags.url = embed.url "
                "AND tags.album = album.id "
                "AND album.name = '%1' "
                "AND tags.sampler = %2 "
                "ORDER BY modifydate DESC LIMIT 1;" )
                .arg( escapeString( album ) )
                .arg( boolT() ) );
    } else {
        values = query( QString(
                "SELECT embed.hash, embed.url FROM tags, embed, artist, album "
                "WHERE tags.url = embed.url "
                "AND tags.artist = artist.id "
                "AND tags.album = album.id "
                "AND artist.name = '%1' "
                "AND album.name = '%2' "
                "ORDER BY modifydate DESC LIMIT 1;" )
                .arg( escapeString( artist ) )
                .arg( escapeString( album ) ) );
    }

    if ( values.count() == 2 ) {
        QCString hash = values.first().utf8();
        QString result = loadHashFile( hash, width );
        if ( result.isEmpty() ) {
            // need to get original from file first
            MetaBundle mb(  KURL::fromPathOrURL( values.last() ) );
            if ( extractEmbeddedImage( mb, hash ) ) {
                // try again, as should be possible now
                result = loadHashFile( hash, width );
            }
        }
        return result;
    }
    return QString::null;
}


QString
CollectionDB::findMetaBundleImage( MetaBundle trackInformation, uint width )
{
    QStringList values =
            query( QString(
            "SELECT embed.hash FROM tags LEFT JOIN embed ON tags.url = embed.url WHERE tags.url = '%1' ORDER BY hash DESC LIMIT 1;" )
            .arg( escapeString( trackInformation.url().path() ) ) );

    if ( values.empty() || !values.first().isEmpty() ) {
        QCString hash;
        QString result;
        if( !values.empty() ) { // file in collection, so we know the hash
            hash = values.first().utf8();
            result = loadHashFile( hash, width );
        }
        if ( result.isEmpty() ) {
            // need to get original from file first
            if ( extractEmbeddedImage( trackInformation, hash ) ) {
                // try again, as should be possible now
                result = loadHashFile( hash, width );
            }
        }
        return result;
    }
    return QString::null;
}


QCString
CollectionDB::makeWidthKey( uint width )
{
    return QString::number( width ).local8Bit() + "@";
}


bool
CollectionDB::removeAlbumImage( const QString &artist, const QString &album )
{
    QCString widthKey = "*@";
    QCString key = md5sum( artist, album );
    query( "DELETE FROM amazon WHERE filename='" + key + "'" );

    // remove scaled versions of images (and add the asterisk for the shadow-caches)
    QStringList scaledList = cacheCoverDir().entryList( widthKey + key + "*" );
    if ( scaledList.count() > 0 )
        for ( uint i = 0; i < scaledList.count(); i++ )
            QFile::remove( cacheCoverDir().filePath( scaledList[ i ] ) );

    // remove large, original image
    if ( largeCoverDir().exists( key ) && QFile::remove( largeCoverDir().filePath( key ) ) )
    {
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
CollectionDB::notAvailCover( const bool withShadow, int width )
{
    if ( width <= 1 )
        width = AmarokConfig::coverPreviewSize();
    QString widthKey = QString::number( width ) + "@";
    QString s;

    if( cacheCoverDir().exists( widthKey + "nocover.png" ) )
        s = cacheCoverDir().filePath( widthKey + "nocover.png" );
    else
    {
        m_noCover.smoothScale( width, width, QImage::ScaleMin ).save( cacheCoverDir().filePath( widthKey + "nocover.png" ), "PNG" );
        s = cacheCoverDir().filePath( widthKey + "nocover.png" );
    }

    if ( withShadow )
        s = makeShadowedImage( s );

    return s;
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
CollectionDB::composerList( bool withUnknowns, bool withCompilations )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComposer );

    if ( !withUnknowns )
        qb.excludeMatch( QueryBuilder::tabSong, i18n( "Unknown" ) );
    if ( !withCompilations )
        qb.setOptions( QueryBuilder::optNoCompilations );

    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valComposer );
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
    qb.addFilter( QueryBuilder::tabSong, "" );

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
                      "AND lower(artist.name) = lower('" + escapeString( artist ) + "') " +
                      ( withUnknown ? QString::null : "AND album.name <> '' " ) +
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) +
                      " ORDER BY lower( album.name );" );
    }
    else
    {
        return query( "SELECT DISTINCT album.name FROM tags, album, artist WHERE "
                      "tags.album = album.id AND tags.artist = artist.id "
                      "AND lower(artist.name) = lower('" + escapeString( artist ) + "') " +
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
CollectionDB::addPodcastChannel( const PodcastChannelBundle &pcb, const bool &replace )
{
    QString command;
    if( replace ) {
        command = "REPLACE INTO podcastchannels "
                  "( url, title, weblink, image, comment, copyright, parent, directory"
                  ", autoscan, fetchtype, autotransfer, haspurge, purgecount ) "
                  "VALUES (";
    } else {
        command = "INSERT INTO podcastchannels "
                  "( url, title, weblink, image, comment, copyright, parent, directory"
                  ", autoscan, fetchtype, autotransfer, haspurge, purgecount ) "
                  "VALUES (";
    }

    QString title       = pcb.title();
    KURL    link        = pcb.link();
    KURL    image       = pcb.imageURL();
    QString description = pcb.description();
    QString copyright   = pcb.copyright();

    if( title.isEmpty() )
        title = pcb.url().prettyURL();

    command += "'" + escapeString( pcb.url().url() )  + "',";
    command += ( title.isEmpty() ?       "NULL" : "'" + escapeString( title ) + "'" ) + ",";
    command += ( link.isEmpty() ?        "NULL" : "'" + escapeString( link.url() ) + "'" ) + ",";
    command += ( image.isEmpty() ?       "NULL" : "'" + escapeString( image.url() ) + "'" ) + ",";
    command += ( description.isEmpty() ? "NULL" : "'" + escapeString( description ) + "'" ) + ",";
    command += ( copyright.isEmpty() ?   "NULL" : "'" + escapeString( copyright ) + "'" ) + ",";
    command += QString::number( pcb.parentId() ) + ",'";
    command += escapeString( pcb.saveLocation().url() ) + "',";
    command += pcb.autoscan() ? boolT() + "," : boolF() + ",";
    command += QString::number( pcb.fetchType() ) + ",";
    command += pcb.autotransfer() ? boolT() + "," : boolF() + ",";
    command += pcb.hasPurge() ? boolT() + "," : boolF() + ",";
    command += QString::number( pcb.purgeCount() ) + ");";

    //FIXME: currently there's no way to check if an INSERT query failed or not - always return true atm.
    // Now it might be possible as insert returns the rowid.
    insert( command, NULL );
    return true;
}

int
CollectionDB::addPodcastEpisode( const PodcastEpisodeBundle &episode, const int idToUpdate )
{
    QString command;

    if( idToUpdate ) {
        command = "REPLACE INTO podcastepisodes "
                  "( id, url, localurl, parent, title, subtitle, composer, comment, filetype, createdate, guid, length, size, isNew ) "
                  "VALUES (";
    } else {
        command = "INSERT INTO podcastepisodes "
                  "( url, localurl, parent, title, subtitle, composer, comment, filetype, createdate, guid, length, size, isNew ) "
                  "VALUES (";
    }

    QString localurl    = episode.localUrl().url();
    QString title       = episode.title();
    QString subtitle    = episode.subtitle();
    QString author      = episode.author();
    QString description = episode.description();
    QString type        = episode.type();
    QString date        = episode.date();
    QString guid        = episode.guid();
    int     duration    = episode.duration();
    uint    size        = episode.size();

    if( title.isEmpty() )
        title = episode.url().prettyURL();

    if( idToUpdate )
        command += QString::number( idToUpdate ) + ",";

    command += "'" + escapeString( episode.url().url() )   + "',";
    command += ( localurl.isEmpty()       ? "NULL" : "'" + escapeString( localurl )       + "'" ) + ",";
    command += "'" + escapeString( episode.parent().url()) + "',";
    command += ( title.isEmpty()       ? "NULL" : "'" + escapeString( title )       + "'" ) + ",";
    command += ( subtitle.isEmpty()    ? "NULL" : "'" + escapeString( subtitle )    + "'" ) + ",";
    command += ( author.isEmpty()      ? "NULL" : "'" + escapeString( author )      + "'" ) + ",";
    command += ( description.isEmpty() ? "NULL" : "'" + escapeString( description ) + "'" ) + ",";
    command += ( type.isEmpty()        ? "NULL" : "'" + escapeString( type )        + "'" ) + ",";
    command += ( date.isEmpty()        ? "NULL" : "'" + escapeString( date )        + "'" ) + ",";
    command += ( guid.isEmpty()        ? "NULL" : "'" + escapeString( guid )        + "'" ) + ",";
    command += QString::number( duration ) + ",";
    command += QString::number( size ) + ",";
    command += episode.isNew() ? boolT() + " );" : boolF() + " );";

    insert( command, NULL );

    if( idToUpdate ) return idToUpdate;
    //This is a bit of a hack. We have just inserted an item, so it is going to be the one with the
    //highest id.  Change this if threaded insertions are used in the future.
    QStringList values = query( QString("SELECT id FROM podcastepisodes WHERE url='%1' ORDER BY id DESC;")
                                        .arg( escapeString( episode.url().url() ) ) );
    if( values.isEmpty() ) return -1;

    return values[0].toInt();
}

QValueList<PodcastChannelBundle>
CollectionDB::getPodcastChannels()
{
    QString command = "SELECT url, title, weblink, image, comment, copyright, parent, directory "
        ", autoscan, fetchtype, autotransfer, haspurge, purgecount FROM podcastchannels;";

    QStringList values = query( command );
    QValueList<PodcastChannelBundle> bundles;

    foreach( values )
    {
        PodcastChannelBundle pcb;
        pcb.setURL         ( KURL::fromPathOrURL(*it) );
        pcb.setTitle       ( *++it );
        pcb.setLink        ( KURL::fromPathOrURL(*++it) );
        pcb.setImageURL    ( KURL::fromPathOrURL(*++it) );
        pcb.setDescription ( *++it );
        pcb.setCopyright   ( *++it );
        pcb.setParentId    ( (*++it).toInt() );
        pcb.setSaveLocation( KURL::fromPathOrURL(*++it) );
        pcb.setAutoScan    ( *++it == boolT() ? true : false );
        pcb.setFetchType   ( (*++it).toInt() );
        pcb.setAutoTransfer( *++it == boolT() ? true : false  );
        pcb.setPurge       ( *++it == boolT() ? true : false  );
        pcb.setPurgeCount  ( (*++it).toInt() );

        bundles.append( pcb );
    }

    return bundles;
}

QValueList<PodcastEpisodeBundle>
CollectionDB::getPodcastEpisodes( const KURL &parent, bool onlyNew, int limit )
{
    QString command = QString( "SELECT id, url, localurl, parent, guid, title, subtitle, composer, comment, filetype, createdate, length, size, isNew FROM podcastepisodes WHERE ( parent='%1'" ).arg( parent.url() );
    if( onlyNew )
        command += QString( " AND isNew='%1'" ).arg( boolT() );
    command += " ) ORDER BY id";
    if( limit != -1 )
        command += QString( " DESC LIMIT %1 OFFSET 0" ).arg( limit );
    command += ";";

    QStringList values = query( command );
    QValueList<PodcastEpisodeBundle> bundles;

    foreach( values )
    {
        PodcastEpisodeBundle peb;
        peb.setDBId        ( (*it).toInt() );
        peb.setURL         ( KURL::fromPathOrURL(*++it) );
        if( *++it != "NULL" )
            peb.setLocalURL    ( KURL::fromPathOrURL(*it) );
        peb.setParent      ( KURL::fromPathOrURL(*++it) );
        peb.setGuid        ( *++it );
        peb.setTitle       ( *++it );
        if( *++it != NULL )
            peb.setSubtitle( *it );
        peb.setAuthor      ( *++it );
        peb.setDescription ( *++it );
        peb.setType        ( *++it );
        peb.setDate        ( *++it );
        peb.setDuration    ( (*++it).toInt() );
        if( *++it == NULL )
            peb.setSize        ( 0 );
        else
            peb.setSize        ( (*it).toInt() );
        peb.setNew         ( (*++it) == boolT() ? true : false  );

        bundles.append( peb );
    }

    return bundles;
}

PodcastEpisodeBundle
CollectionDB::getPodcastEpisodeById( int id )
{
    QString command = QString( "SELECT url, localurl, parent, guid, title, subtitle, composer, comment, filetype, createdate, length, size, isNew FROM podcastepisodes WHERE id=%1;").arg( id );

    QStringList values = query( command );
    PodcastEpisodeBundle peb;
    foreach( values )
    {
        peb.setDBId        ( id );
        peb.setURL         ( KURL::fromPathOrURL(*it) );
        if( *++it != "NULL" )
            peb.setLocalURL( KURL::fromPathOrURL(*it) );
        peb.setParent      ( KURL::fromPathOrURL(*++it) );
        peb.setGuid        ( *++it );
        peb.setTitle       ( *++it );
        peb.setSubtitle    ( *++it );
        peb.setAuthor      ( *++it );
        peb.setDescription ( *++it );
        peb.setType        ( *++it );
        peb.setDate        ( *++it );
        peb.setDuration    ( (*++it).toInt() );
        if( *++it == NULL )
            peb.setSize    ( 0 );
        else
            peb.setSize    ( (*it).toInt() );
        peb.setNew         ( (*++it) == boolT() ? true : false  );
    }

    return peb;
}

bool
CollectionDB::getPodcastEpisodeBundle( const KURL &url, PodcastEpisodeBundle *peb )
{
    int id = 0;
    if( url.isLocalFile() )
    {
        QStringList values =
            query( QString( "SELECT id FROM podcastepisodes WHERE localurl = '%1';" )
                    .arg( escapeString( url.url() ) ) );
        if( !values.isEmpty() )
            id = values[0].toInt();
    }
    else
    {
        QStringList values =
            query( QString( "SELECT id FROM podcastepisodes WHERE url = '%1';" )
                    .arg( escapeString( url.url() ) ) );
        if( !values.isEmpty() )
            id = values[0].toInt();
    }

    if( id )
    {
        *peb = getPodcastEpisodeById( id );
        return true;
    }

    return false;
}

bool
CollectionDB::getPodcastChannelBundle( const KURL &url, PodcastChannelBundle *pcb )
{
    QStringList values = query( QString(
                "SELECT url, title, weblink, image, comment, copyright, parent, directory "
                ", autoscan, fetchtype, autotransfer, haspurge, purgecount FROM podcastchannels WHERE url = '%1';"
                ).arg( escapeString( url.url() ) ) );

    foreach( values )
    {
        pcb->setURL         ( KURL::fromPathOrURL(*it) );
        pcb->setTitle       ( *++it );
        pcb->setLink        ( KURL::fromPathOrURL(*++it) );
        if( *++it != "NULL" )
            pcb->setImageURL( KURL::fromPathOrURL(*it) );
        pcb->setDescription ( *++it );
        pcb->setCopyright   ( *++it );
        pcb->setParentId    ( (*++it).toInt() );
        pcb->setSaveLocation( KURL::fromPathOrURL(*++it) );
        pcb->setAutoScan    ( *++it == boolT() ? true : false );
        pcb->setFetchType   ( (*++it).toInt() );
        pcb->setAutoTransfer( *++it == boolT() ? true : false  );
        pcb->setPurge       ( *++it == boolT() ? true : false  );
        pcb->setPurgeCount  ( (*++it).toInt() );
    }

    return !values.isEmpty();
}

// return newly created folder id
int
CollectionDB::addPodcastFolder( const QString &name, const int parent_id, const bool isOpen )
{
    QString command = QString( "INSERT INTO podcastfolders ( name, parent, isOpen ) VALUES ('" );
    command += escapeString( name )   + "',";
    command += QString::number( parent_id ) + ",";
    command += isOpen ? boolT() + ");" : boolF() + ");";

    insert( command, NULL );

    command = QString( "SELECT id FROM podcastfolders WHERE name = '%1' AND parent = '%2';" )
                       .arg( name, QString::number(parent_id) );
    QStringList values = query( command );

    return values[0].toInt();
}

void
CollectionDB::updatePodcastChannel( const PodcastChannelBundle &b )
{
    if( getDbConnectionType() == DbConnection::postgresql )
    {
        query( QStringx( "UPDATE podcastchannels SET title='%1', weblink='%2', comment='%3', "
                         "copyright='%4', parent=%5, directory='%6', autoscan=%7, fetchtype=%8, "
			 "autotransfer=%9, haspurge=%10, purgecount=%11 WHERE url='%12';" )
	       .args ( QStringList()
		       << escapeString( b.title() )
		       << escapeString( b.link().url() )
		       << escapeString( b.description() )
		       << escapeString( b.copyright() )
		       << QString::number( b.parentId() )
		       << escapeString( b.saveLocation().url() )
		       << ( b.autoscan() ? boolT() : boolF() )
		       << QString::number( b.fetchType() )
		       << (b.hasPurge() ? boolT() : boolF() )
		       << (b.autotransfer() ? boolT() : boolF() )
		       << QString::number( b.purgeCount() )
		       << escapeString( b.url().url() )
		   )
	    );
    }
    else {
        addPodcastChannel( b, true ); //replace the already existing row
    }
}

void
CollectionDB::updatePodcastEpisode( const int id, const PodcastEpisodeBundle &b )
{
    if( getDbConnectionType() == DbConnection::postgresql )
    {
        query( QStringx( "UPDATE podcastepisodes SET url='%1', localurl='%2', parent='%3', title='%4', subtitle='%5', composer='%6', comment='%7', "
                 "filetype='%8', createdate='%9', guid='%10', length=%11, size=%12, isNew=%13 WHERE id=%14;" )
              .args( QStringList()
                  << escapeString( b.url().url() )
                  << ( b.localUrl().isValid() ? escapeString( b.url().url() ) : "NULL" )
                  << escapeString( b.parent().url() )
                  << escapeString( b.title() )
                  << escapeString( b.subtitle() )
                  << escapeString( b.author() )
                  << escapeString( b.description() )
                  << escapeString( b.type() )
                  << escapeString( b.date() )
                  << escapeString( b.guid() )
                  << QString::number( b.duration() )
                  << escapeString( QString::number( b.size() ) )
                  << ( b.isNew() ? boolT() : boolF() )
                  << QString::number( id )
                  )
             );
    }
    else {
        addPodcastEpisode( b, id );
    }
}

void
CollectionDB::updatePodcastFolder( const int folder_id, const QString &name, const int parent_id, const bool isOpen )
{
    if( getDbConnectionType() == DbConnection::postgresql ) {
        query( QStringx( "UPDATE podcastfolders SET name='%1', parent=%2, isOpen=%3 WHERE id=%4;" )
                .args( QStringList()
                    << escapeString(name)
                    << QString::number(parent_id)
                    << ( isOpen ? boolT() : boolF() )
                    << QString::number(folder_id)
                    )
             );
    }
    else {
        query( QStringx( "REPLACE INTO podcastfolders ( id, name, parent, isOpen ) "
                        "VALUES ( %1, '%2', %3, %4 );" )
                .args( QStringList()
                    << QString::number(folder_id)
                    << escapeString(name)
                    << QString::number(parent_id)
                    << ( isOpen ? boolT() : boolF() )
                    )
             );
    }
}

void
CollectionDB::removePodcastChannel( const KURL &url )
{
    //remove channel
    query( QString( "DELETE FROM podcastchannels WHERE url = '%1';" )
              .arg( escapeString( url.url() ) ) );
    //remove all children
    query( QString( "DELETE FROM podcastepisodes WHERE parent = '%1';" )
              .arg( escapeString( url.url() ) ) );
}


/// Try not to delete by url, since some podcast feeds have all the same url
void
CollectionDB::removePodcastEpisode( const int id )
{
    if( id < 0 ) return;
    query( QString( "DELETE FROM podcastepisodes WHERE id = '%1';" )
              .arg( QString::number(id) ) );
}

void
CollectionDB::removePodcastFolder( const int id )
{
    if( id < 0 ) return;
    query( QString("DELETE FROM podcastfolders WHERE id=%1;")
              .arg( QString::number(id) ) );
}

bool
CollectionDB::addSong( MetaBundle* bundle, const bool incremental )
{
    if ( !QFileInfo( bundle->url().path() ).isReadable() ) return false;

    QString command = "INSERT INTO tags_temp "
                      "( url, dir, createdate, modifydate, album, artist, genre, year, title, "
                      "composer, comment, track, discnumber, sampler, length, bitrate, "
                      "samplerate, filesize, filetype ) "
                      "VALUES ('";

    QString artist = bundle->artist();
    QString title = bundle->title();
    if ( title.isEmpty() )
    {
        title = bundle->url().fileName();
        if ( bundle->url().fileName().find( '-' ) > 0 )
        {
            if ( artist.isEmpty() )
            {
                artist = bundle->url().fileName().section( '-', 0, 0 ).stripWhiteSpace();
                bundle->setArtist( artist );
            }
            title = bundle->url().fileName().section( '-', 1 ).stripWhiteSpace();
            title = title.left( title.findRev( '.' ) ).stripWhiteSpace();
            if ( title.isEmpty() ) title = bundle->url().fileName();
        }
        bundle->setTitle( title );
    }

    command += escapeString( bundle->url().path() ) + "','";
    command += escapeString( bundle->url().directory() ) + "',";
    command += QString::number( QFileInfo( bundle->url().path() ).created().toTime_t() ) + ",";
    command += QString::number( QFileInfo( bundle->url().path() ).lastModified().toTime_t() ) + ",";

    command += escapeString( QString::number( albumID( bundle->album(),   true, !incremental, true ) ) ) + ",";
    command += escapeString( QString::number( artistID( bundle->artist(), true, !incremental, true ) ) ) + ",";
    command += escapeString( QString::number( genreID( bundle->genre(),   true, !incremental, true ) ) ) + ",'";
    command += escapeString( QString::number( yearID( QString::number( bundle->year() ), true, !incremental, true ) ) ) + "','";

    command += escapeString( bundle->title() ) + "',";
    command += ( bundle->composer().isEmpty() ? "NULL" : "'"+escapeString( bundle->composer() ) + "'" ) + ",'";
    command += escapeString( bundle->comment() ) + "', ";
    command += escapeString( QString::number( bundle->track() ) ) + " , ";
    command += escapeString( QString::number( bundle->discNumber() ) ) + " , ";
    switch( bundle->compilation() ) {
        case MetaBundle::CompilationNo:
            command += boolF();
            break;

        case MetaBundle::CompilationYes:
            command += boolT();
            break;

        case MetaBundle::CompilationUnknown:
        default:
            command += "NULL";
    }
    command += ",";

    // NOTE any of these may be -1 or -2, this is what we want
    //      see MetaBundle::Undetermined
    command += QString::number( bundle->length() ) + ",";
    command += QString::number( bundle->bitrate() ) + ",";
    command += QString::number( bundle->sampleRate() ) + ",";
    command += QString::number( bundle->filesize() ) + ",";
    command += QString::number( bundle->fileType() ) + ")";

    //FIXME: currently there's no way to check if an INSERT query failed or not - always return true atm.
    // Now it might be possible as insert returns the rowid.
    insert( command, NULL );

    doATFStuff( bundle, true );

    return true;
}

void
CollectionDB::doATFStuff( MetaBundle* bundle, const bool tempTables )
{
    //DEBUG_BLOCK
    //debug() << "m_atfEnabled = " << (m_atfEnabled ? "true" : "false") << endl;
    if( !m_atfEnabled || bundle->uniqueId().isEmpty() ) return;
    //ATF Stuff
    QString currid = escapeString( bundle->uniqueId() );
    QString currurl = escapeString( bundle->url().path() );
    QString currdir = escapeString( bundle->url().directory() );

    //debug() << "Checking currid = " << currid << ", currurl = " << currurl << endl;

    if( currid.isEmpty() || currurl.isEmpty() )
        return;

    QStringList urls = query( QString(
            "SELECT url, uniqueid "
            "FROM uniqueid%1 "
            "WHERE url = '%2';" )
                .arg( tempTables ? "_temp" : "" )
                .arg( currurl ) );
    QStringList uniqueids = query( QString(
            "SELECT url, uniqueid "
            "FROM uniqueid%1 "
            "WHERE uniqueid = '%2';" )
                .arg( tempTables ? "_temp" : "" )
                .arg( currid ) );

    QStringList nonTempIDs = query( QString(
            "SELECT url, uniqueid "
            "FROM uniqueid "
            "WHERE uniqueid = '%1';" )
                .arg( currid ) );

    QStringList nonTempURLs = query( QString(
            "SELECT url, uniqueid "
            "FROM uniqueid "
            "WHERE url = '%1';" )
                .arg( currurl ) );

    bool tempTablesAndInPermanent = false;
    bool permanentFullMatch = false;

    //if we're not using temp tables here, i.e. tempTables is false,
    //then the results from both sets of queries above should be equal,
    //so behavior should be the same
    if( tempTables && ( nonTempURLs.count() > 0 || nonTempIDs.count() > 0 ) )
            tempTablesAndInPermanent = true;
    if( tempTablesAndInPermanent && nonTempURLs.count() > 0 && nonTempIDs.count() > 0 )
            permanentFullMatch = true;

    //first case: not in permanent table or temporary table
    if( !tempTablesAndInPermanent && urls.empty() && uniqueids.empty() )
    {
        QString insertline = QString( "INSERT INTO uniqueid%1 (url, uniqueid, dir) VALUES ('%2', '%3', '%4')" )
                .arg( ( tempTables ? "_temp" : "" ),
                    currurl,
                    currid,
                    currdir );
        //debug() << "running command: " << insertline << endl;
        insert( insertline, NULL );
        return;
    }

    //next case: not in permanent table, but a match on one or the other in the temporary table
    //OR, we are using permanent tables (and not considering temp ones)
    if( !tempTablesAndInPermanent )
    {
        if( urls.empty() ) //uniqueid already found in temporary table but not url; check the old URL
        {
            //debug() << "At doATFStuff, stat-ing file " << uniqueids[0] << endl;
            //stat the original URL
            bool statSuccessful = QFile::exists( uniqueids[0] );
            if( statSuccessful ) //if true, new one is a copy
            {
                //debug() << "stat was successful, new file is a copy" << endl;
                bundle->newUniqueId();
                doATFStuff( bundle, true ); //yes, it's recursive, but what's wrong with that? :-)
            }
            else  //it's a move, not a copy, or a copy and then both files were moved...can't detect that
            {
                //debug() << "stat was NOT successful, updating tables with: " << endl;
                //debug() << QString( "UPDATE uniqueid%1 SET url='%2', dir='%3' WHERE uniqueid='%4';" ).arg( ( tempTables ? "_temp" : "" ), currurl, currdir, currid ) << endl;
                query( QString( "UPDATE uniqueid%1 SET url='%2', dir='%3' WHERE uniqueid='%4';" )
                      .arg( ( tempTables ? "_temp" : "" ),
                         currurl,
                         currdir,
                         currid ) );
                emit fileMoved( uniqueids[0], bundle->url().path(), currid );
            }
        }
        //okay then, url already found in temporary table but different uniqueid
        //a file exists in the same place as before, but new uniqueid...assume
        //that this is desired user behavior
        //NOTE: this should never happen during an incremental scan with temporary tables...!
        else //uniqueids.empty()
        {
            //debug() << "file exists in same place as before, new uniqueid" << endl;
            query( QString( "UPDATE uniqueid%1 SET uniqueid='%2' WHERE url='%3';" )
                    .arg( tempTables ? "_temp" : "" )
                    .arg( currid )
                    .arg( currurl ) );
            emit uniqueIdChanged( bundle->url().path(), urls[1], currid );
        }
        return;
    }
    //okay...being here means, we are using temporary tables, AND it exists in the permanent table
    else
    {
        //first case...full match exists in permanent table, should then be no match in temp table
        //(since code below deleted from permanent table after changes)
        //in this case, just insert into temp table
        if( permanentFullMatch )
        {
            QString insertline = QString( "INSERT INTO uniqueid_temp (url, uniqueid, dir) VALUES ('%1', '%2', '%3')" )
                .arg( currurl,
                    currid,
                    currdir );
            //debug() << "running command: " << insertline << endl;
            insert( insertline, NULL );
            return;
        }

        //second case...full match exists in permanent table, but path is different
        if( nonTempURLs.empty() )
        {
            //debug() << "At doATFStuff part 2, stat-ing file " << nonTempIDs[0] << endl;
            //stat the original URL
            bool statSuccessful = QFile::exists( nonTempIDs[0] );
            if( statSuccessful ) //if true, new one is a copy
            {
                //debug() << "stat part 2 was successful, new file is a copy" << endl;
                bundle->newUniqueId();
                doATFStuff( bundle, true ); //yes, it's recursive, but what's wrong with that? :-)
            }
            else  //it's a move, not a copy, or a copy and then both files were moved...can't detect that
            {
                //debug() << "stat part 2 was NOT successful, updating tables with: " << endl;
                query( QString( "INSERT INTO uniqueid_temp (url, uniqueid, dir) VALUES ('%1', '%2', '%3')" )
                .arg( currurl,
                    currid,
                    currdir ) );
                query( QString( "DELETE FROM uniqueid WHERE uniqueid='%1';" )
                      .arg( currid ) );
                emit fileMoved( nonTempIDs[0], bundle->url().path(), currid );
            }
        }
        else
        {
            //debug() << "file exists in same place as before, part 2, new uniqueid" << endl;
            query( QString( "INSERT INTO uniqueid_temp (url, uniqueid, dir) VALUES ('%1', '%2', '%3')" )
                .arg( currurl,
                    currid,
                    currdir ) );
                query( QString( "DELETE FROM uniqueid WHERE url='%1';" )
                      .arg( currurl ) );
            emit uniqueIdChanged( bundle->url().path(), nonTempURLs[1], currid );
        }
        return;
    }
}

void
CollectionDB::newUniqueIdForFile( const QString &path )
{
    DEBUG_BLOCK
    KURL url = KURL::fromPathOrURL( path );

    if( !QFile::exists( path ) )
    {
        debug() << "QFile::exists returned false for " << path << endl;
        return;
    }

    // Clean it.
    url.cleanPath();

    MetaBundle bundle( url );
    bundle.newUniqueId();
    doATFStuff( &bundle, false );
}

QString
CollectionDB::urlFromUniqueId( const QString &id )
{
    if( !m_atfEnabled )
        return QString::null;

    QStringList urls = query( QString(
            "SELECT url, uniqueid "
            "FROM uniqueid "
            "WHERE uniqueid = '%1';" )
                .arg( id ) );

    if( urls.empty() )
        return QString::null;

    return urls[0];
}

QString
CollectionDB::getURL( const MetaBundle &bundle )
{
    uint artID = artistID( bundle.artist(), false );
    if( !artID )
        return QString::null;

    uint albID = albumID( bundle.album(), false );
    if( !albID )
        return QString::null;

    QString q = QString( "SELECT tags.url "
            "FROM tags "
            "WHERE tags.album = '%1' AND tags.artist = '%2' AND tags.track = '%3' AND tags.title = '%4';" )
        .arg( albID )
        .arg( artID )
        .arg( bundle.track() )
        .arg( escapeString( bundle.title() ) );

    QStringList urls = query( q );

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
    while( values.count() < 15 )
        values += "IF YOU CAN SEE THIS THERE IS A BUG!";

    QStringList::ConstIterator it = values.begin();

    bundle.setAlbum     ( *it ); ++it;
    bundle.setArtist    ( *it ); ++it;
    bundle.setGenre     ( *it ); ++it;
    bundle.setTitle     ( *it ); ++it;
    bundle.setYear      ( (*it).toInt() ); ++it;
    bundle.setComment   ( *it ); ++it;
    bundle.setDiscNumber( (*it).toInt() ); ++it;
    bundle.setComposer  ( *it ); ++it;
    bundle.setTrack     ( (*it).toInt() ); ++it;
    bundle.setBitrate   ( (*it).toInt() ); ++it;
    bundle.setLength    ( (*it).toInt() ); ++it;
    bundle.setSampleRate( (*it).toInt() ); ++it;
    bundle.setFilesize  ( (*it).toInt() ); ++it;
    bundle.setFileType  ( (*it).toInt() ); ++it;

    bool ok;
    int val = (*it).toInt( &ok );
    bundle.setCompilation( ok ? val : MetaBundle::CompilationUnknown );

    if( AmarokConfig::advancedTagFeatures() )
        bundle.setUniqueId();
}

bool
CollectionDB::bundleForUrl( MetaBundle* bundle )
{
    QStringList values = query( QString(
            "SELECT album.name, artist.name, genre.name, tags.title, "
            "year.name, tags.comment, tags.discnumber, tags.composer, "
            "tags.track, tags.bitrate, tags.length, tags.samplerate, "
            "tags.filesize, tags.filetype, tags.sampler "
            "FROM tags, album, artist, genre, year "
            "WHERE album.id = tags.album AND artist.id = tags.artist AND "
            "genre.id = tags.genre AND year.id = tags.year AND tags.url = '%1';" )
                .arg( escapeString( bundle->url().path() ) ) );

    if ( !values.empty() )
        fillInBundle( values, *bundle );
    else
    {
        int id = 0;
        values = query( QString(
                    "SELECT id FROM podcastepisodes WHERE localurl = '%1';" )
                .arg( escapeString( bundle->url().url() ) ) );
        if( !values.isEmpty() )
        {
            if( bundle->url().protocol() == "file" && QFile::exists( bundle->url().path() ) )
            {
                MetaBundle mb( bundle->url(), true /* avoid infinite recursion */ );
                *bundle = mb;
            }
            id = values[0].toInt();
        }
        else
        {
            values = query( QString(
                        "SELECT id FROM podcastepisodes WHERE url = '%1';" )
                    .arg( escapeString( bundle->url().url() ) ) );
            if( !values.isEmpty() )
                id = values[0].toInt();
        }

        if( id )
        {
            bundle->setPodcastBundle( getPodcastEpisodeById( id ) );
            bundle->setTitle( bundle->podcastBundle()->title() );
            if( bundle->artist().isEmpty() )
                bundle->setArtist( bundle->podcastBundle()->author() );
            PodcastChannelBundle pcb;
            if( getPodcastChannelBundle( bundle->podcastBundle()->parent(), &pcb ) )
            {
                bundle->setAlbum( pcb.title() );
            }
            bundle->setGenre( QString( "Podcast" ) );
        }
    }

    return !values.isEmpty();
}


QValueList<MetaBundle>
CollectionDB::bundlesByUrls( const KURL::List& urls )
{
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
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComposer );
            qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBitrate );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valSamplerate );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFilesize );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFileType );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valIsCompilation );

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
                b.setComposer  (  *++it );
                b.setYear      ( (*++it).toInt() );
                b.setComment   (  *++it );
                b.setTrack     ( (*++it).toInt() );
                b.setBitrate   ( (*++it).toInt() );
                b.setDiscNumber( (*++it).toInt() );
                b.setLength    ( (*++it).toInt() );
                b.setSampleRate( (*++it).toInt() );
                b.setFilesize  ( (*++it).toInt() );
                b.setFileType  ( (*++it).toInt() );
                b.setPath      (  *++it );
                bool ok;
                int val = (*++it).toInt( &ok );
                b.setCompilation( ok ? val : MetaBundle::CompilationUnknown );

                if( m_atfEnabled )
                    b.setUniqueId();

                b.checkExists();

                buns50.append( b );
            }

            // we get no guarantee about the order that the database
            // will return our values, and sqlite indeed doesn't return
            // them in the desired order :( (MySQL does though)
            foreach( paths )
            {
                for( BundleList::Iterator jt = buns50.begin(), end = buns50.end(); jt != end; ++jt )
                {
                    if ( ( *jt ).url().path() == ( *it ) )
                    {
                        bundles += *jt;
                        buns50.remove( jt );
                        goto success;
                    }
                }

                // if we get here, we didn't find an entry
                {
                    KURL url = KURL::fromPathOrURL( *it );

                    const MetaBundle *mb = MediaBrowser::instance()->getBundle( url );
                    if ( mb )
                    {
                        debug() << "Bundle recovered from media browser for: " << *it << endl;
                        b = MetaBundle( *mb );
                    }
                    else
                    {
                        debug() << "No bundle recovered for: " << *it << endl;
                        if( url.isLocalFile() )
                        {
                            b = MetaBundle( url );
                        }
                        else
                        {
                            b = MetaBundle();
                            b.setUrl( url );
                            // FIXME: more context for i18n after string freeze
                            b.setTitle( QString( "%1 %2 %3%4" )
                                  .arg( url.filename(),
                                     i18n( "from" ),
                                     url.hasHost() ? url.host() : QString(),
                                     url.directory( false ) ) );
                        }

                        // try to see if the engine has some info about the
                        // item (the intended behaviour should be that if the
                        // item is an AudioCD track, the engine can return
                        // CDDB data for it)
                        Engine::SimpleMetaBundle smb;
                        if ( EngineController::engine()->metaDataForUrl( b.url(), smb ) )
                        {
                            b.setTitle( smb.title );
                            b.setArtist( smb.artist );
                            b.setAlbum( smb.album );
                            b.setComment( smb.comment );
                            b.setGenre( smb.genre );
                            b.setBitrate( smb.bitrate.toInt() );
                            b.setSampleRate( smb.samplerate.toInt() );
                            b.setLength( smb.length.toInt() );
                            b.setYear( smb.year.toInt() );
                            b.setTrack( smb.tracknr.toInt() );
                        }

                        // check if it's a podcast
                        PodcastEpisodeBundle peb;
                        if( getPodcastEpisodeBundle( url, &peb ) )
                        {
                            b.setPodcastBundle( peb );
                            b.setTitle( peb.title() );
                            if( b.artist().isEmpty() )
                                b.setArtist( peb.author() );
                            PodcastChannelBundle pcb;
                            if( getPodcastChannelBundle( peb.parent(), &pcb ) )
                            {
                                b.setAlbum( pcb.title() );
                            }
                            b.setGenre( QString ( "Podcast" ) );
                        }
                    }
                }
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


void
CollectionDB::addSongPercentage( const QString &url, int percentage,
        const QString &reason, const QDateTime *playtime )
{
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, percentage, rating FROM statistics "
            "WHERE url = '%1';" )
            .arg( escapeString( url ) ) );

    uint atime = playtime ? playtime->toTime_t() : QDateTime::currentDateTime().toTime_t();

    // check boundaries
    if ( percentage > 100 ) percentage = 100;
    if ( percentage < 1 )   percentage = 1;

    if ( !values.isEmpty() )
    {
        // increment playcounter and update accesstime
        if (getDbConnectionType() == DbConnection::postgresql) {
            query( QString( "UPDATE statistics SET playcounter=%1, accessdate=%2 WHERE url='%3';" )
                            .arg( values[0] + " + 1" )
                            .arg( atime )
                            .arg( escapeString( url ) ) );
        }
        else
        {
            query( QString( "REPLACE INTO statistics ( url, createdate, accessdate, percentage, playcounter, rating ) "
                            "VALUES ( '%6', %2, %3, %4, %5, %1 );" )
                            .arg( values[3] )
                            .arg( values[1] )
                            .arg( atime )
                            .arg( values[2] )
                            .arg( values[0] + " + 1" )
                            .arg( escapeString( url ) ) );
        }
    }
    else
    {
        insert( QString( "INSERT INTO statistics ( url, createdate, accessdate, percentage, playcounter, rating ) "
                        "VALUES ( '%3', %1, %2, 0, 1, 0 );" )
                        .arg( atime )
                        .arg( atime )
                        .arg( escapeString( url ) ), NULL );
    }

    double prevscore = 50;
    int playcount = 0;
    if( !values.isEmpty() )
    {
        playcount = values[ 0 ].toInt();
        // This stops setting the Rating (which creates a row) from affecting the
        // prevscore of an unplayed track. See bug 127475
        if ( playcount )
            prevscore = values[ 2 ].toDouble();
    }
    const QStringList v = query( QString( "SELECT length FROM tags WHERE url = '%1';" ).arg( escapeString( url ) ) );
    const int length = v.isEmpty() ? 0 : v.first().toInt();

    ScriptManager::instance()->requestNewScore( url, prevscore, playcount, length, percentage, reason );
}


int
CollectionDB::getSongPercentage( const QString &url )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, url );

    QStringList values = qb.run();

    if( !values.isEmpty() )
        return values.first().toInt();

    return 0;
}

int
CollectionDB::getSongRating( const QString &url )
{
    QStringList values = query( QString( "SELECT rating FROM statistics WHERE url = '%1';" )
                                         .arg( escapeString( url ) ) );

    if( values.count() )
        return kClamp( values.first().toInt(), 0, 10 );

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
CollectionDB::setSongRating( const QString &url, int rating, bool toggleHalf )
{
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, accessdate, percentage, rating FROM statistics WHERE url = '%1';" )
            .arg( escapeString( url ) ));

    bool ok = true;
    int prev = values[4].toInt( &ok );
    if( ok && toggleHalf && prev == rating )
    {
        if( rating == 1 )
            rating = 0;
        else if( rating % 2 ) //.5
            rating++;
        else
            rating--;
    }

    // check boundaries
    if ( rating > 10 ) rating = 10;
    if ( rating < 0 || rating == 1 ) rating = 0; //ratings are 1-5

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
        .arg( escapeString( newURL ),
              escapeString( oldURL ) ) );

    query( QString( "UPDATE statistics SET url = '%1' WHERE url = '%2';" )
        .arg( escapeString( newURL ),
              escapeString( oldURL ) ) );

    query( QString( "UPDATE lyrics SET url = '%1' WHERE url = '%2';" )
        .arg( escapeString( newURL ),
              escapeString( oldURL ) ) );
    //  Clean up.
    query( QString( "DELETE FROM tags WHERE url = '%1';" )
        .arg( escapeString( oldURL ) ) );

    query( QString( "DELETE FROM statistics WHERE url = '%1';" )
        .arg( escapeString( oldURL ) ) );

    query( QString( "DELETE FROM lyrics WHERE url = '%1';" )
        .arg( escapeString( oldURL ) ) );

    query( QString( "UPDATE uniqueid SET url = '%1' WHERE url = '%2';" )
        .arg( escapeString( newURL ),
              escapeString( oldURL ) ) );

    query( QString( "UPDATE playlists SET url = '%1' WHERE url = '%2';" )
        .arg( escapeString( newURL ),
              escapeString( oldURL ) ) );
}

void
CollectionDB::fileOperationResult( KIO::Job *job ) // slot
{
    if(job->error())
    {
        m_fileOperationFailed = true;
        debug() << "file operation failed: " << job->errorText() << endl;
    }
    else
    {
        m_fileOperationFailed = false;
    }

    m_waitForFileOperation = false;
}


bool
CollectionDB::organizeFile( const KURL &src, const OrganizeCollectionDialog &dialog, bool copy )
{
   bool overwrite = dialog.overwriteCheck->isChecked();
   bool localFile = src.isLocalFile();
   KURL tmpSrc = src;
   if( !localFile )
   {
      QString tmp;
      int count = 0;
      do
      {
         QString extension = src.url().section( ".", -1 );
         tmp = QString( dialog.folderCombo->currentText() + "/amarok-tmp-%1." + extension ).arg( count );
         count++;
      } while( QFile::exists( tmp ) );
      tmpSrc = KURL::fromPathOrURL( tmp );

      KIO::Job *job = 0;
      if( copy )
      {
         job = KIO::file_copy( src, tmpSrc, -1, false, false, false );
      }
      else
      {
         job = KIO::file_move( src, tmpSrc, -1, false, false, false );
      }
      connect( job, SIGNAL(result( KIO::Job * )), SLOT(fileOperationResult( KIO::Job * )) );
      m_waitForFileOperation = true;
      while( m_waitForFileOperation )
      {
         usleep( 10000 );
         kapp->processEvents( 100 );
      }

      if( m_fileOperationFailed )
      {
         debug() << "failed to transfer " << src.url() << " to " << tmpSrc << endl;
         return false;
      }
   }

   //Building destination here.
   MetaBundle mb( tmpSrc );
   QString dest = dialog.buildDestination( dialog.buildFormatString(), mb );

   debug() << "Destination: " << dest << endl;

   if( tmpSrc.path() != dest ) //supress error warning that file couldn't be moved
   {
      if( !CollectionDB::instance()->moveFile( tmpSrc.url(), dest, overwrite, copy && localFile ) )
      {
         if( !localFile )
            QFile::remove( tmpSrc.path() );
         return false;
      }
   }

   //Use cover image for folder icon
   if( dialog.coverCheck->isChecked() && !mb.artist().isEmpty() && !mb.album().isEmpty() )
   {
      KURL dstURL = KURL::fromPathOrURL( dest );
      dstURL.cleanPath();

      QString path  = dstURL.directory();
      QString cover = CollectionDB::instance()->albumImage( mb.artist(), mb.album(), false, 1 );

      if( !QFile::exists(path + "/.directory") && !cover.endsWith( "nocover.png" ) )
      {
         //QPixmap thumb;        //Not amazon nice.
         //if ( thumb.load( cover ) ){
         //thumb.save(path + "/.front.png", "PNG", -1 ); //hide files

         KSimpleConfig config(path + "/.directory");
         config.setGroup("Desktop Entry");

         if( !config.hasKey("Icon") )
         {
            //config.writeEntry("Icon", QString("%1/.front.png").arg( path ));
            config.writeEntry( "Icon", cover );
            config.sync();
            debug() << "Using this cover as icon for: " << path << endl;
            debug() << cover << endl;
         }
         //}         //Not amazon nice.
      }
   }

   if( localFile && QDir().rmdir( src.directory() ) )
   {
      debug() << "removed: " << src.directory() << endl;
   }

   return true;
}

bool
CollectionDB::moveFile( const QString &src, const QString &dest, bool overwrite, bool copy )
{
    DEBUG_BLOCK
    if(src == dest){
        debug() << "Source and destination URLs are the same, aborting."  << endl;
        return false;
    }

    // Escape URL.
    KURL srcURL = KURL::fromPathOrURL( src );
    KURL dstURL = KURL::fromPathOrURL( dest );

    // Clean it.
    srcURL.cleanPath();
    dstURL.cleanPath();

    QString uid = getUniqueId( src );

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

    m_waitForFileOperation = false;

    KIO::Job *job = 0;
    if( copy )
    {
        job = KIO::file_copy( srcURL, dstURL, -1, overwrite, false, false );
    }
    else
    {
        job = KIO::file_move( srcURL, dstURL, -1, overwrite, false, false );
    }
    connect( job, SIGNAL(result( KIO::Job * )), SLOT(fileOperationResult( KIO::Job * )) );
    m_waitForFileOperation = true;
    while( m_waitForFileOperation )
    {
        usleep( 10000 );
        kapp->processEvents( 100 );
    }

    if( !m_fileOperationFailed )
    {
        emit fileMoved( src, dest );

        if( copy )
        {
            MetaBundle bundle( dstURL );
            if( bundle.isValidMedia() )
            {
                bundle.newUniqueId();
                addSong( &bundle, true );
                return true;
            }
        }
        else
        {
            if( isFileInCollection( srcURL.path() ) )
            {
                migrateFile( srcURL.path(), dstURL.path() );
                if( m_atfEnabled )
                    emit fileMoved( src, dest, uid );
                else
                    emit fileMoved( src, dest);
                return true;
            }
            else
            {
                MetaBundle bundle( dstURL );
                if( bundle.isValidMedia() )
                {
                    bundle.newUniqueId();
                    addSong( &bundle, true );
                    return true;
                }
            }
        }
    }

    return false;
}


void
CollectionDB::updateDirStats( QString path, const long datetime, const bool temporary )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    if ( getDbConnectionType() == DbConnection::postgresql )
    {
        // REPLACE INTO is not valid SQL for postgres, so we need to check whether we
        // should UPDATE() or INSERT()
        QStringList values = query( QString("SELECT * FROM directories%1 WHERE dir='%2';")
            .arg( temporary ? "_temp" : "")
            .arg( escapeString( path ) ) );

        if ( values.count() > 0 )
        {
            query( QString( "UPDATE directories%1 SET changedate=%2 WHERE dir='%3';")
            .arg( temporary ? "_temp" : "" )
            .arg( datetime )
            .arg( escapeString( path ) ) );
        }
        else
        {
            query( QString( "INSERT INTO directories%1 (dir,changedate) VALUES ('%3','%2');")
            .arg( temporary ? "_temp" : "")
            .arg( datetime )
            .arg( escapeString( path ) )
            );
        }
    }
    else
    {
        query( QString( "REPLACE INTO directories%1 ( dir, changedate ) VALUES ( '%3', %2 );" )
                  .arg( temporary ? "_temp" : "" )
                  .arg( datetime )
                  .arg( escapeString( path ) )
                  );
    }

    INotify::instance()->watchDir( path );
}


void
CollectionDB::removeSongsInDir( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    query( QString( "DELETE FROM tags WHERE dir = '%1';" )
              .arg( escapeString( path ) ) );

    query( QString( "DELETE FROM uniqueid WHERE dir = '%1';" )
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
        if( AmarokConfig::advancedTagFeatures() )
            query( QString( "DELETE FROM uniqueid WHERE url = '%1';" )
                .arg( escapeString( (*it).path() ) ) );
    }
}


QStringList
CollectionDB::similarArtists( const QString &artist, uint count )
{
    QStringList values;

    values = query( QString( "SELECT suggestion FROM related_artists WHERE artist = '%1' LIMIT %2 OFFSET 0;" )
                                 .arg( escapeString( artist ), QString::number( count ) ) );

    if ( values.isEmpty() )
        Scrobbler::instance()->similarArtists( artist );

    return values;
}


void
CollectionDB::sanitizeCompilations()
{
    query( QString( "UPDATE tags_temp SET sampler = %1 WHERE sampler IS NULL;").arg( boolF() ) );
}

void
CollectionDB::checkCompilations( const QString &path, const bool temporary )
{
    QStringList albums;
    QStringList artists;
    QStringList dirs;

    albums = query( QString( "SELECT DISTINCT album.name FROM tags_temp, album%1 AS album WHERE tags_temp.dir = '%2' AND album.id = tags_temp.album AND tags_temp.sampler IS NULL;" )
              .arg( temporary ? "_temp" : "" )
              .arg( escapeString( path ) ));

    for ( uint i = 0; i < albums.count(); i++ )
    {
        if ( albums[ i ].isEmpty() ) continue;

        const uint album_id = albumID( albums[ i ], false, temporary, true );
        artists = query( QString( "SELECT DISTINCT artist.name FROM tags_temp, artist%1 AS artist WHERE tags_temp.album = '%2' AND tags_temp.artist = artist.id;" )
                            .arg( temporary ? "_temp" : "" )
                            .arg( album_id ) );
        dirs    = query( QString( "SELECT DISTINCT dir FROM tags_temp WHERE album = '%1';" )
                            .arg( album_id ) );

        if ( artists.count() > dirs.count() )
        {
            debug() << "Detected compilation: " << albums[ i ] << " - " << artists.count() << ":" << dirs.count() << endl;
        }
        query( QString( "UPDATE tags_temp SET sampler = %1 WHERE album = '%2' AND sampler IS NULL;" )
                         .arg(artists.count() > dirs.count() ? boolT() : boolF()).arg( album_id ) );
    }
}


QStringList
CollectionDB::setCompilation( const QString &album, const bool enabled, const bool updateView )
{
    uint id = albumID( album, false, false, true );
    QStringList retval;

    if ( id )
    {
        QString idString = QString::number( id );
        query( QString( "UPDATE tags SET sampler = %1 WHERE tags.album = %2;" )
                .arg( enabled ? boolT() : boolF() )
                .arg( idString ) );
        retval = query( QString( "SELECT url FROM tags WHERE tags.album = %1;" ).arg( idString ) );
    }

    // Update the Collection-Browser view,
    // using QTimer to make sure we don't manipulate the GUI from a thread
    if ( updateView )
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );

    // return a list so tags can be updated
    return retval;
}


void
CollectionDB::removeDirFromCollection( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    query( QString( "DELETE FROM directories WHERE dir = '%1';" )
                    .arg( escapeString( path ) ) );
}


QString
CollectionDB::IDfromExactValue( QString table, QString value, bool autocreate, bool temporary /* = false */ )
{
    if ( temporary )
        table.append( "_temp" );

    QString querystr( QString( "SELECT id FROM %1 WHERE name " ).arg( table ) );
    querystr += exactCondition( value ) + ';';
    QStringList result = query( querystr );
    if ( result.isEmpty() )
    {
        if ( autocreate )
            return QString::number( insert( QString( "INSERT INTO %1 ( name ) VALUES ( '%2' );" )
                 .arg( table, escapeString( value ) ),
                 table ) );
        else
            return 0;
    }
    else
    {
        if ( result.count() > 1 )
            debug() << "More than one entry in the " << table << " database for '" << value << '\'' << endl;
        return result.first();
    }
}

void CollectionDB::deleteRedundantName( const QString &table, QString ID )
{
    QString querystr( QString( "SELECT %1 FROM tags WHERE tags.%1 = %2 LIMIT 1;" ).arg( table, ID ) );
    QStringList result = query( querystr );
    if ( result.isEmpty() )
        query( QString( "DELETE FROM %1 WHERE id = %2;" ).arg( table,ID ) );
}


void
CollectionDB::updateTags( const QString &url, const MetaBundle &bundle, const bool updateView )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComposer );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFilesize );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFileType );
    // [10] is above. [11] is below.
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valID );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valID );
    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valID );

    qb.addURLFilters ( QStringList( url ) );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    QStringList values = qb.run();

    if ( values.isEmpty() )
        return;

    if ( values.count() > 15 )
    {
        error() << "Query returned more than 1 song. Aborting updating metadata" << endl;
        return;
    }

    bool art=false, alb=false, gen=false, year=false;

    QString command = "UPDATE tags SET ";
    if ( values[ 0 ] != bundle.title() )
        command += "title = '" + escapeString( bundle.title() ) + "', ";
    if ( values[ 1 ] != bundle.artist() )
    {
        art = true;
        command += "artist = " + IDfromExactValue( "artist", bundle.artist() ) + ", ";
    }
    if ( values[ 2 ] != bundle.album() )
    {
        alb = true;
        command += "album = "  + IDfromExactValue( "album", bundle.album() ) + ", ";
    }
    if ( values[ 3 ] != bundle.genre() )
    {
        gen = true;
        command += "genre = "  + IDfromExactValue( "genre", bundle.genre() ) + ", ";
    }
    if ( values[ 4 ] != QString::number( bundle.year() ) )
    {
        year = false;
        command += "year = "   + IDfromExactValue( "year", QString::number( bundle.year() ) ) + ", ";
    }
    if ( values[ 5 ] != QString::number( bundle.track() ) )
        command += "track = " + QString::number( bundle.track() ) + ", ";
    if ( values[ 6 ] != bundle.comment() )
        command += "comment = '" + escapeString( bundle.comment() ) + "', ";
    if ( values[ 7 ] != bundle.composer() )
        command += "composer = '" + escapeString( bundle.composer() ) + "', ";
    if ( values[ 8 ] != QString::number( bundle.discNumber() ) )
        command += "discnumber = '" + QString::number( bundle.discNumber() ) + "', ";
    if ( values[ 9 ] != QString::number( bundle.filesize() ) )
        command += "filesize = '" + QString::number( bundle.filesize() ) + "', ";
    if ( values[ 10 ] != QString::number( bundle.fileType() ) )
        command += "filetype = '" + QString::number( bundle.fileType() ) + "', ";

    if ( "UPDATE tags SET " == command )
    {
        debug() << "No tags selected to be changed" << endl;
    }
    else
    {
        //We have to remove the trailing comma from command
        query( command.left( command.length() - 2 ) + " WHERE url = '" + escapeString( url ) + "';" );
    }

    //Check to see if we use the entry anymore. If not, delete it
    if ( art )
        deleteRedundantName( "artist", values[ 11 ] );
    if ( alb )
        deleteRedundantName( "album", values[ 12 ] );
    if ( gen )
        deleteRedundantName( "genre", values[ 13 ] );
    if ( year )
        deleteRedundantName( "year", values[ 14 ] );

    if ( EngineController::instance()->bundle().url() == bundle.url() )
    {
        debug() << "Current song edited, updating widgets: " << bundle.title() << endl;
        EngineController::instance()->currentTrackMetaDataChanged( bundle );
    }

    // Update the Collection-Browser view,
    // using QTimer to make sure we don't manipulate the GUI from a thread
    if ( updateView )
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( databaseChanged() ) );

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
    doATFStuff( &bundle, false );
}

QString
CollectionDB::getUniqueId( const QString &url )
{
    QStringList values = query( QString( "SELECT url FROM uniqueid WHERE url = '%1';" ).arg( escapeString( url ) ));
    return values[0];
}

void
CollectionDB::setLyrics( const QString &url, const QString &lyrics )
{
    QStringList values = query(QString("SELECT lyrics FROM lyrics WHERE url = '%1';").arg( escapeString( url ) ) );
    if(values.count() > 0)
    {
        if ( !lyrics.isEmpty() )
            query( QString( "UPDATE lyrics SET lyrics = '%1' WHERE url = '%2';" ).arg( escapeString( lyrics), escapeString( url ) ));
        else
            query( QString( "DELETE FROM lyrics WHERE url = '%1';" ).arg( escapeString( url ) ));
    }
    else
    {
        insert( QString( "INSERT INTO lyrics (url, lyrics) values ( '%1', '%2' );" ).arg( escapeString( url ), escapeString( lyrics ) ),
         NULL);
    }
}


QString
CollectionDB::getLyrics( const QString &url )
{
    QStringList values = query( QString( "SELECT lyrics FROM lyrics WHERE url = '%1';" ).arg( escapeString( url ) ));
    return values[0];
}

void CollectionDB::removeInvalidAmazonInfo( const QString& md5sum )
{
    query( QString( "DELETE FROM amazon WHERE filename='%1'" ).arg( md5sum ) );
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
        if ( AmarokConfig::postgresqlHost() != config->host() )
        {
            recreateConnections = true;
        }
        else if ( AmarokConfig::postgresqlPort() != config->port() )
        {
            recreateConnections = true;
        }
        else if ( AmarokConfig::postgresqlDbName() != config->database() )
        {
            recreateConnections = true;
        }
        else if ( AmarokConfig::postgresqlUser() != config->username() )
        {
            recreateConnections = true;
        }
        else if ( AmarokConfig::postgresqlPassword() != config->password() )
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
        PlaylistBrowser::instance()->loadPodcastsFromDatabase();

        emit databaseEngineChanged();
    }
}

DbConnection * CollectionDB::getMyConnection()
{
    //after some thought, to be thread-safe, must lock at the beginning of this function,
    //not only if a new connection is made
    connectionMutex->lock();

    DbConnection *dbConn;
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
        dbConn = new MySqlConnection( static_cast<MySqlConfig*>( m_dbConfig ) );
    }
    else
#endif
#ifdef USE_POSTGRESQL
    if ( m_dbConnType == DbConnection::postgresql )
    {
        dbConn = new PostgresqlConnection( static_cast<PostgresqlConfig*>( m_dbConfig ) );
    }
    else
#endif
    {
        dbConn = new SqliteConnection( static_cast<SqliteConfig*>( m_dbConfig ) );
    }

    threadConnections->insert(currThread, dbConn);

    connectionMutex->unlock();
    return dbConn;
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


void CollectionDB::engineTrackEnded( int finalPosition, int trackLength, const QString &reason )
{
    //This is where percentages are calculated
    //TODO statistics are not calculated when currentTrack doesn't exist

    // Don't update statistics if song has been played for less than 15 seconds
    // if ( finalPosition < 15000 ) return;

    const KURL url = EngineController::instance()->bundle().url();
    debug() << "track ended: " << url.url() << endl;
    PodcastEpisodeBundle peb;
    if( getPodcastEpisodeBundle( url.url(), &peb ) )
    {
        PlaylistBrowser::instance()->findPodcastEpisode( peb.url(), peb.parent() )->setListened();

        if( !url.isLocalFile() )
            return;
    }

    if ( url.path().isEmpty() || !m_autoScoring ) return;

    // sanity check
    if ( finalPosition > trackLength || finalPosition <= 0 )
        finalPosition = trackLength;

    int pct = (int) ( ( (double) finalPosition / (double) trackLength ) * 100 );

    // increase song counter & calculate new statistics
    addSongPercentage( url.path(), pct, reason );
}


void
CollectionDB::timerEvent( QTimerEvent* )
{
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
    if ( AmarokConfig::monitorChanges() )
        scanModifiedDirs();
}


void
CollectionDB::startScan()  //SLOT
{
    QStringList folders = AmarokConfig::collectionFolders();

    if ( folders.isEmpty() )
    {
        dropTables( false );
        createTables( false );
    }
    else if( PlaylistBrowser::instance() )
    {
        emit scanStarted();
        ThreadWeaver::instance()->queueJob( new ScanController( this, false, folders ) );
    }
}


void
CollectionDB::stopScan() //SLOT
{
    ThreadWeaver::instance()->abortAllJobsNamed( "CollectionScanner" );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::dirDirty( const QString& path )
{
    debug() << k_funcinfo << "Dirty: " << path << endl;

    ThreadWeaver::instance()->queueJob( new ScanController( this, false, path ) );
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
                    .arg( escapedArtist,
                          CollectionDB::instance()->escapeString( *it ) ), NULL);

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

void
CollectionDB::atfMigrateStatisticsUrl( const QString& oldUrl, const QString& newUrl, const QString& /*uniqueid*/ )
{
    if( !m_atfEnabled )
        return;

    query( QString( "DELETE FROM statistics WHERE url = '%1';" )
                            .arg( escapeString( newUrl ) ) );
    query( QString( "UPDATE statistics SET url = '%1' WHERE url = '%2';" )
                            .arg( escapeString( newUrl ) )
                            .arg( escapeString( oldUrl ) ) );
}

void
CollectionDB::atfMigrateStatisticsUniqueId( const QString& /*url*/, const QString& oldid, const QString& newid )
{
    //don't need to check for ATF on, signal never emitted otherwise
    query( QString( "DELETE FROM statistics WHERE uniqueid = '%1';" )
                            .arg( escapeString( newid ) ) );
    query( QString( "UPDATE statistics SET uniqueid = '%1' WHERE uniqueid = '%2';" )
                            .arg( escapeString( newid ) )
                            .arg( escapeString( oldid ) ) );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::initialize()
{
    DEBUG_BLOCK

    /// Create DBConfig instance:

#ifdef USE_MYSQL
    if ( m_dbConnType == DbConnection::mysql )
    {
        QString appVersion = amaroK::config( "General Options" )->readEntry( "Version" );
        QString passwd = AmarokConfig::mySqlPassword2(); // stored as string type

        if( passwd.isEmpty() )
        {
            if( appVersion.startsWith( "1.3" ) )
            {
                /// This is because of the encrypted -> plaintext conversion
                passwd = AmarokConfig::mySqlPassword(); // stored as password type
                AmarokConfig::setMySqlPassword2( passwd );
            }
            else if( appVersion.startsWith( "1.4" ) )
            {
                passwd = amaroK::config( "MySql" )->readEntry( "MySqlPassword" ); //read the field as plaintext
                AmarokConfig::setMySqlPassword2( passwd ); // store it in plaintext field
            }
        }

        m_dbConfig = new MySqlConfig(
                    AmarokConfig::mySqlHost(),
                    AmarokConfig::mySqlPort(),
                    AmarokConfig::mySqlDbName(),
                    AmarokConfig::mySqlUser(),
                    passwd );
    }
    else
#endif
#ifdef USE_POSTGRESQL
    if ( m_dbConnType == DbConnection::postgresql )
    {
        QString appVersion = amaroK::config( "General Options" )->readEntry( "Version" );
        QString passwd = AmarokConfig::postgresqlPassword2();

        if( passwd.isEmpty() )
        {
            if( appVersion.startsWith( "1.3" ) )
            {
                /// This is because of the encrypted -> plaintext conversion
                passwd = AmarokConfig::postgresqlPassword(); // stored as password type
                AmarokConfig::setPostgresqlPassword2( passwd );
            }
            else if( appVersion.startsWith( "1.4" ) &&
                   ( appVersion.contains( "beta", false ) ||
                     appVersion.contains( "svn",  false ) ) )
            {
                passwd = amaroK::config( "Postgresql" )->readEntry( "PostgresqlPassword" );
                AmarokConfig::setPostgresqlPassword2( passwd );
            }
        }

        m_dbConfig = new PostgresqlConfig(
                    AmarokConfig::postgresqlHost(),
                    AmarokConfig::postgresqlPort(),
                    AmarokConfig::postgresqlDbName(),
                    AmarokConfig::postgresqlUser(),
                    passwd );
    }
    else
#endif
    {
        m_dbConfig = new SqliteConfig( "collection.db" );
    }

    DbConnection *dbConn = getMyConnection();

    KConfig* config = amaroK::config( "Collection Browser" );
    if(!dbConn->isConnected())
        amaroK::MessageQueue::instance()->addMessage(dbConn->lastError());
    if ( !dbConn->isInitialized() || !isValid() )
    {
        warning() << "Your database is either corrupt or empty. Dropping all and recreating..." << endl;
        dropTables( false );
        dropPersistentTables();
        dropPodcastTables();
        dropStatsTable();

        createTables(false);
        createPersistentTables();
        createPodcastTables();
        createStatsTable();
    }
    else
    {
        if ( adminValue( "Database Stats Version" ).toInt() != DATABASE_STATS_VERSION
          || config->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION )
        {
            debug() << "Different database stats version detected! Stats table will be updated or rebuilt." << endl;

            #if 0 // causes mysterious crashes
            if( getType() == DbConnection::sqlite && QFile::exists( amaroK::saveLocation()+"collection.db" ) )
            {
                debug() << "Creating a backup of the database in "
                        << amaroK::saveLocation()+"collection-backup.db" << "." << endl;

                bool copied = KIO::NetAccess::file_copy( amaroK::saveLocation()+"collection.db",
                                                         amaroK::saveLocation()+"collection-backup.db",
                                                         -1 /*perms*/, true /*overwrite*/, false /*resume*/ );

                if( !copied )
                {
                    debug() << "Backup failed! Perhaps the volume is not writable." << endl;
                    debug() << "Error was: " << KIO::NetAccess::lastErrorString() << endl;
                }
            }
            #endif

            int prev = adminValue( "Database Stats Version" ).toInt();

            /* If config returns 3 or lower, it came from an Amarok version that was not aware of
               admin table, so we can't trust this table at all */
            if( !prev || ( config->readNumEntry( "Database Stats Version", 0 )
                      && config->readNumEntry( "Database Stats Version", 0 ) <= 3  ) )
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

            if( prev == 5)
            {
                //Code to add unqiueid column and maybe "deleted" column will go here,
                //pending talks with Seb...
                //don't bump actual version in collectiondb.h before this is written!
            }
            if( prev == 4 )
            {
                debug() << "Updating stats-database!" << endl;
                query( "UPDATE statistics SET rating = rating * 2;" );
            }
            else if( prev == 3 ) //every version from 1.2 forward had a stats version of 3
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
            debug() << "Detected old schema for tables with important data. Amarok will convert the tables, ignore any \"table already exists\" errors." << endl;
            createPersistentTables();
            /* Copy lyrics */
            debug() << "Trying to get lyrics from old db schema." << endl;
            QStringList Lyrics = query( "SELECT url, lyrics FROM tags where tags.lyrics IS NOT NULL;" );
            for (uint i=0; i<Lyrics.count(); i+=2  )
                setLyrics( Lyrics[i], Lyrics[i+1]  );
            debug() << "Building podcast tables" << endl;
            createPodcastTables();
        }
        else if ( PersistentVersion == "1" || PersistentVersion == "2" ) {
            createPersistentTables(); /* From 1 to 2 nothing changed. There was just a bug on the code, and
                                         on some cases the table wouldn't be created.
                                         From 2 to 3, lyrics were made plain text, instead of HTML */
            debug() << "Converting Lyrics to Plain Text." << endl;
            QStringList Lyrics = query( "SELECT url, lyrics FROM lyrics;" );
            for (uint i=0; i<Lyrics.count(); i+=2  )
                setLyrics( Lyrics[i], Lyrics[i+1]  );
            debug() << "Building podcast tables" << endl;
            createPodcastTables();
        }
        else if ( PersistentVersion.toInt() < 4 )
        {
            debug() << "Building podcast tables" << endl;
            createPodcastTables();
        }
        else if ( PersistentVersion.toInt() < 5 )
        {
            debug() << "Updating podcast tables" << endl;
            query( "ALTER TABLE podcastchannels ADD image " + textColumnType() + ";" );
            query( "ALTER TABLE podcastepisodes ADD localurl " + textColumnType() + ";" );
            query( "ALTER TABLE podcastepisodes ADD subtitle " + textColumnType() + ";" );
            query( "ALTER TABLE podcastepisodes ADD size INTEGER;" );
            query( "ALTER TABLE podcastepisodes DROP comment;" );
            query( "ALTER TABLE podcastepisodes ADD comment " + longTextColumnType() + ";" );
            query( "CREATE INDEX localurl_podepisode ON podcastepisodes( localurl );" );
        }
        else if ( PersistentVersion.toInt() < 6 )
        {
            debug() << "Updating podcast tables" << endl;
            query( "ALTER TABLE podcastchannels ADD image " + textColumnType() + ";" );
            query( "ALTER TABLE podcastepisodes ADD subtitle " + textColumnType() + ";" );
            query( "ALTER TABLE podcastepisodes ADD size INTEGER;" );
            query( "ALTER TABLE podcastepisodes DROP comment;" );
            query( "ALTER TABLE podcastepisodes ADD comment " + longTextColumnType() + ";" );
        }
        else if ( PersistentVersion.toInt() < 11 )
        {
            debug() << "This is used to handle problems from uniqueid changeover and should not do anything" << endl;
        }
        else if ( PersistentVersion.toInt() < 12 )
        {
            debug() << "Adding playlists table..." << endl;
            createPersistentTables();
        }
        else if ( PersistentVersion.toInt() == 12 )
        {
            //UP-TO-DATE!  Keep that number in sync to make things easier.
        }
        else {
            if ( adminValue( "Database Persistent Tables Version" ).toInt() != DATABASE_PERSISTENT_TABLES_VERSION ) {
                error() << "There is a bug in Amarok: instead of destroying your valuable database tables, I'm quitting" << endl;
                exit( 1 );

                debug() << "Rebuilding persistent tables database!" << endl;
                dropPersistentTables();
                createPersistentTables();
            }
        }

        QString PodcastVersion = adminValue( "Database Podcast Tables Version" );
        if ( PodcastVersion.isEmpty() || PodcastVersion.toInt() < 2 )
        {
            debug() << "Podcast tables created and up to date" << endl;
        }
        else
        {
            debug() << "Rebuilding podcast tables database!" << endl;
            dropPodcastTables();
            createPodcastTables();
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
    //do we need or want to delete the actual connection objects as well as clearing them from the QMap?
    //or does QMap's clear function delete them?
    //this situation is not at all likely to happen often, so leaving them might be okay to prevent a
    //thread from having its connection torn out from under it...not likely, but possible
    //and leaving it should not end up eating much memory at all

    connectionMutex->lock();

    threadConnections->clear();
    delete m_dbConfig;

    connectionMutex->unlock();
}


void
CollectionDB::scanModifiedDirs()
{
    if ( !m_scanInProgress )
    {
        //we check if a job is pending because we don't want to abort incremental collection readings
        if ( !ThreadWeaver::instance()->isJobPending( "CollectionScanner" ) && PlaylistBrowser::instance() )
        {
            m_scanInProgress = true;
            m_rescanRequired = false;
            emit scanStarted();

            ThreadWeaver::instance()->onlyOneJob( new ScanController( this, true ) );
        }
    }
    else
        m_rescanRequired = true;
}


void
CollectionDB::customEvent( QCustomEvent *e )
{
    if ( e->type() == (int)ScanController::JobFinishedEvent )
    {
        ScanController* s = static_cast<ScanController*>( e );
        m_scanInProgress = false;

        if ( s->isIncremental() )
        {
            debug() << "JobFinishedEvent from Incremental ScanController received.\n";
            emit scanDone( s->hasChanged() );

            // check if something changed while we were scanning. in this case we should
            // rescan again, now.
            if ( m_rescanRequired )
                QTimer::singleShot( 0, CollectionDB::instance(), SLOT( scanMonitor() ) );
        }
        else
        {
            debug() << "JobFinishedEvent from ScanController received.\n";
            emit scanDone( s->wasSuccessful() );
        }
    }
}


QString
CollectionDB::loadHashFile( const QCString& hash, uint width )
{
    //debug() << "loadHashFile: " << hash << " - " << width << endl;

    QString full = tagCoverDir().filePath( hash );

    if ( width == 0 ) {
        if ( QFileInfo( full ).isReadable() ) {
            //debug() << "loadHashFile: fullsize: " << full << endl;
            return full;
        }
    } else {
        if ( width == 1 ) width = AmarokConfig::coverPreviewSize();
        QCString widthKey = makeWidthKey( width );

        QString path = cacheCoverDir().filePath( widthKey + hash );
        if ( QFileInfo( path ).isReadable() ) {
            //debug() << "loadHashFile: scaled: " << path << endl;
            return path;
        } else if ( QFileInfo( full ).isReadable() ) {
            //debug() << "loadHashFile: scaling: " << full << endl;
            QImage image( full );
            if ( image.smoothScale( width, width, QImage::ScaleMin ).save( path, "PNG" ) ) {
                //debug() << "loadHashFile: scaled: " << path << endl;
                return path;
            }
        }
    }
    return QString::null;
}


bool
CollectionDB::extractEmbeddedImage( MetaBundle &trackInformation, QCString& hash )
{
    //debug() << "extractEmbeddedImage: " << hash << " - " << trackInformation.url().path() << endl;

    MetaBundle::EmbeddedImageList images;
    trackInformation.embeddedImages( images );
    foreachType ( MetaBundle::EmbeddedImageList, images ) {
        if ( hash.isEmpty() || (*it).hash() == hash ) {
            if ( (*it).save( tagCoverDir() ) ) {
                //debug() << "extractEmbeddedImage: saved to " << tagCoverDir().path() << endl;
                hash = (*it).hash();
                return true;
            }
        }
    }
    return false;
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

DbConnection::DbConnection()
    : m_initialized( false )
{}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS SqliteConnection
//////////////////////////////////////////////////////////////////////////////////////////

SqliteConnection::SqliteConnection( const SqliteConfig* /*config*/ )
    : DbConnection()
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
    int busyCnt = 0;

    //compile SQL program to virtual machine, reattempting if busy
    do {
        if ( busyCnt )
        {
            ::usleep( 100000 );      // Sleep 100 msec
            debug() << "sqlite3_prepare: BUSY counter: " << busyCnt << endl;
        }
        error = sqlite3_prepare( m_db, statement.utf8(), -1, &stmt, &tail );
    }
    while ( SQLITE_BUSY==error && busyCnt++ < 120 );

    if ( error != SQLITE_OK )
    {
        if ( SQLITE_BUSY==error )
            Debug::error() << "Gave up waiting for lock to clear" << endl;
        Debug::error() << k_funcinfo << " sqlite3_compile error:" << endl;
        Debug::error() << sqlite3_errmsg( m_db ) << endl;
        Debug::error() << "on query: " << statement << endl;
        values = QStringList();
    }
    else
    {
        busyCnt = 0;
        int number = sqlite3_column_count( stmt );
        //execute virtual machine by iterating over rows
        while ( true )
        {
            error = sqlite3_step( stmt );

            if ( error == SQLITE_BUSY )
            {
                if ( busyCnt++ > 120 ) {
                    Debug::error() << "Busy-counter has reached maximum. Aborting this sql statement!\n";
                    break;
                }
                ::usleep( 100000 ); // Sleep 100 msec
                debug() << "sqlite3_step: BUSY counter: " << busyCnt << endl;
                continue;
            }
            if ( error == SQLITE_MISUSE )
                debug() << "sqlite3_step: MISUSE" << endl;
            if ( error == SQLITE_DONE || error == SQLITE_ERROR )
                break;

            //iterate over columns
            for ( int i = 0; i < number; i++ )
            {
                values << QString::fromUtf8( reinterpret_cast<const char*>( sqlite3_column_text( stmt, i ) ) );
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
    int busyCnt = 0;

    //compile SQL program to virtual machine, reattempting if busy
    do {
        if ( busyCnt )
        {
            ::usleep( 100000 );      // Sleep 100 msec
            debug() << "sqlite3_prepare: BUSY counter: " << busyCnt << endl;
        }
        error = sqlite3_prepare( m_db, statement.utf8(), -1, &stmt, &tail );
    }
    while ( SQLITE_BUSY==error && busyCnt++ < 120 );

    if ( error != SQLITE_OK )
    {
        if ( SQLITE_BUSY==error )
            Debug::error() << "Gave up waiting for lock to clear" << endl;
        Debug::error() << k_funcinfo << " sqlite3_compile error:" << endl;
        Debug::error() << sqlite3_errmsg( m_db ) << endl;
        Debug::error() << "on insert: " << statement << endl;
    }
    else
    {
        busyCnt = 0;
        //execute virtual machine by iterating over rows
        while ( true )
        {
            error = sqlite3_step( stmt );

            if ( error == SQLITE_BUSY )
            {
                if ( busyCnt++ > 120 ) {
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
MySqlConnection::MySqlConnection( const MySqlConfig* config )
    : DbConnection()
    , m_connected( false )
{
    DEBUG_BLOCK

    debug() << k_funcinfo << endl;
    m_db = mysql_init(NULL);
    if (m_db)
    {
//         if ( config->username().isEmpty() )
//             pApp->slotConfigAmarok("MySql");

        if ( mysql_real_connect( m_db, config->host().latin1(),
                                       config->username().latin1(),
                                       config->password().latin1(),
                                       config->database().latin1(),
                                       config->port(),
                                       NULL, CLIENT_COMPRESS ) )
        {
            m_initialized = true;

#if MYSQL_VERSION_ID >= 40113
            // now set the right charset for the connection
            QStringList my_qslist = query( "SHOW VARIABLES LIKE 'character_set_database'" );
            if( !my_qslist.isEmpty() && !mysql_set_character_set( m_db, const_cast<char *>( my_qslist[1].latin1() ) ) )
                //charset was updated
                debug() << "Connection Charset is now: " << my_qslist[1].latin1() << endl;
            else
                error() << "Failed to set database charset\n";
#endif

            m_db->reconnect = 1; //setting reconnect flag for newer mysqld
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
            + i18n("<p>You can configure MySQL in the Collection section under Settings->Configure Amarok</p>");
}
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS PostgresqlConnection
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_POSTGRESQL
PostgresqlConnection::PostgresqlConnection( const PostgresqlConfig* config )
      : DbConnection()
      , m_connected( false )
{
  QString conninfo;
    debug() << k_funcinfo << endl;

//     if ( config->username().isEmpty() )
//         pApp->slotConfigAmarok("Postgresql");

    conninfo = "host='" + config->host() +
      "' port=" + QString::number( config->port() ) +
      " dbname='" + config->database() +
      "' user='" + config->username() +
      "' password='" + config->password() + "'";

    m_db = PQconnectdb( conninfo.utf8() );

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
            + i18n("<p>You can configure Postgresql in the Collection section under Settings->Configure Amarok</p>");
}
#endif



//////////////////////////////////////////////////////////////////////////////////////////
// CLASS SqliteConfig
//////////////////////////////////////////////////////////////////////////////////////////

SqliteConfig::SqliteConfig( const QString& dbfile )
    : m_dbfile( dbfile )
{}


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
{}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS PostgresqlConfig
//////////////////////////////////////////////////////////////////////////////////////////

PostgresqlConfig::PostgresqlConfig(
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
        if (tables == tabAlbum || tables==tabArtist || tables==tabGenre || tables == tabYear || tables == tabStats || tables == tabPodcastEpisodes || tables == tabPodcastFolders || tables == tabPodcastChannels)
            m_tables = tableName(tables);
        else
            tables |= tabSong;
    }


    if ( tables & tabSong )
    {
        if ( tables & tabAlbum )
            m_tables += " LEFT JOIN " + tableName( tabAlbum) + " ON album.id=tags.album";
        if ( tables & tabArtist )
            m_tables += " LEFT JOIN " + tableName( tabArtist) + " ON artist.id=tags.artist";
        if ( tables & tabGenre )
            m_tables += " LEFT JOIN " + tableName( tabGenre) + " ON genre.id=tags.genre";
        if ( tables & tabYear )
            m_tables += " LEFT JOIN " + tableName( tabYear) + " ON year.id=tags.year";
        if ( tables & tabStats )
            m_tables += " LEFT JOIN " + tableName( tabStats) + " ON statistics.url=tags.url";
        if ( tables & tabLyrics )
            m_tables += " LEFT JOIN " + tableName( tabLyrics) + " ON lyrics.url=tags.url";

    }
}


void
QueryBuilder::addReturnValue( int table, Q_INT64 value, bool caseSensitive /* = false */ )
{
    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ",";

    if ( table & tabStats && value & valScore )
    {
        if ( CollectionDB::instance()->getType() == DbConnection::sqlite )
            m_values += "CAST(";
        m_values += "round(";
    }

    if ( value == valDummy )
        m_values += "''";
    else
    {
        if ( caseSensitive && CollectionDB::instance()->getType() == DbConnection::mysql )
            m_values += " BINARY ";
        m_values += tableName( table ) + ".";
        m_values += valueName( value );
    }

    if ( table & tabStats && value & valScore )
    {
        m_values += " + 0.4 )";
        if ( CollectionDB::instance()->getType() == DbConnection::sqlite )
            m_values += " AS INTEGER)";
    }

    m_linkTables |= table;
    m_returnValues++;
}

void
QueryBuilder::addReturnFunctionValue( int function, int table, Q_INT64 value)
{
    // translate NULL and 0 values into the default value for percentage/rating
    // First translate 0 to NULL via NULLIF, then NULL to default via COALESCE
    bool defaults = function == funcAvg && ( value & valPercentage || value & valRating );

    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ",";
    m_values += functionName( function ) + "(";
    if ( defaults )
        m_values += "COALESCE(NULLIF(";
    m_values += tableName( table ) + ".";
    m_values += valueName( value );
    if ( defaults )
    {
        m_values += ", 0), ";
        if ( value & valPercentage )
            m_values += "50";
        else
            m_values += "6";
        m_values += ")";
    }
    m_values += ") AS ";
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
    ParsedExpression parsed = ExpressionParser::parse( query );

    for( uint i = 0, n = parsed.count(); i < n; ++i ) //check each part for matchiness
    {
        beginOR();
        for( uint ii = 0, nn = parsed[i].count(); ii < nn; ++ii )
        {
            const expression_element &e = parsed[i][ii];
            QString s = e.text;
            int mode;
            switch( e.match )
            {
                case expression_element::More:     mode = modeGreater; break;
                case expression_element::Less:     mode = modeLess;    break;
                case expression_element::Contains:
                default:                           mode = modeNormal;  break;
            }
            bool exact = false; // enable for numeric values

            int table = -1, value = -1;
            if( e.field == "artist" )
                table = tabArtist;
            else if( e.field == "album" )
                table = tabAlbum;
            else if( e.field == "title" )
                table = tabSong;
            else if( e.field == "genre" )
                table = tabGenre;
            else if( e.field == "year" )
            {
                table = tabYear;
                value = valName;
                exact = true;
            }
            else if( e.field == "score" )
            {
                table = tabStats;
                value = valScore;
                exact = true;
            }
            else if( e.field == "rating" )
            {
                table = tabStats;
                value = valRating;
                exact = true;
                s = QString::number( int( s.toFloat() * 2 ) );
            }
            else if( e.field == "directory" )
            {
                table = tabSong;
                value = valDirectory;
            }
            else if( e.field == "length" )
            {
                table = tabSong;
                value = valLength;
                exact = true;
            }
            else if( e.field == "playcount" )
            {
                table = tabStats;
                value = valPlayCounter;
                exact = true;
            }
            else if( e.field == "samplerate" )
            {
                table = tabSong;
                value = valSamplerate;
                exact = true;
            }
            else if( e.field == "track" )
            {
                table = tabSong;
                value = valTrack;
                exact = true;
            }
            else if( e.field == "disc" || e.field == "discnumber" )
            {
                table = tabSong;
                value = valDiscNumber;
                exact = true;
            }
            else if( e.field == "size" || e.field == "filesize" )
            {
                table = tabSong;
                value = valFilesize;
                exact = true;
                if( s.lower().endsWith( "m" ) )
                    s = QString::number( s.left( s.length()-1 ).toLong() * 1024 * 1024 );
                else if( s.lower().endsWith( "k" ) )
                    s = QString::number( s.left( s.length()-1 ).toLong() * 1024 );
            }
            else if( e.field == "filename" || e.field == "url" )
            {
                table = tabSong;
                value = valURL;
            }
            else if( e.field == "filetype" )
            {
                table = tabSong;
                value = valURL;
                mode = modeEndMatch;
                s.prepend( '.' );
            }
            else if( e.field == "bitrate" )
            {
                table = tabSong;
                value = valBitrate;
                exact = true;
            }
            else if( e.field == "comment" )
            {
                table = tabSong;
                value = valComment;
            }
            else if( e.field == "composer" )
            {
                table = tabSong;
                value = valComposer;
            }
            else if( e.field == "lyrics" )
            {
                table = tabLyrics;
                value = valLyrics;
            }

            if( e.negate )
            {
                if( value >= 0 )
                    excludeFilter( table, value, s, mode, exact );
                else
                    excludeFilter( table >= 0 ? table : defaultTables, s );
            }
            else
            {
                if( value >= 0 )
                    addFilter( table, value, s, mode, exact );
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

        if ( tables & tabAlbum )
            m_where += "OR album.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabArtist )
            m_where += "OR artist.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabGenre )
            m_where += "OR genre.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabYear )
            m_where += "OR year.name " + CollectionDB::likeCondition( filter, false, false );
        if ( tables & tabSong )
            m_where += "OR tags.title " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabComposer )
            m_where += "OR tags.composer " + CollectionDB::likeCondition( filter, true, true );

        if ( i18n( "Unknown" ).contains( filter, false ) )
        {
            if ( tables & tabAlbum )
                m_where += "OR album.name = '' ";
            if ( tables & tabArtist )
                m_where += "OR artist.name = '' ";
            if ( tables & tabGenre )
                m_where += "OR genre.name = '' ";
            if ( tables & tabYear )
                m_where += "OR year.name = '' ";
            if ( tables & tabSong )
                m_where += "OR tags.title = '' ";
            if ( tables & tabComposer )
                m_where += "OR tags.composer = '' OR tags.composer IS NULL ";
        }
        if ( ( tables & tabArtist ) && i18n( "Various Artists" ).contains( filter, false ) )
            m_where += QString( "OR tags.sampler = %1 " ).arg( CollectionDB::instance()->boolT() );
        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::addFilter( int tables, Q_INT64 value, const QString& filter, int mode, bool exact )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";

        QString m, s;
        if (mode == modeLess || mode == modeGreater)
            s = ( mode == modeLess ? "< '" : "> '" ) + CollectionDB::instance()->escapeString( filter ) + "' ";
        else
        {
            if (exact)
                s = " = " + CollectionDB::instance()->escapeString( filter );
            else
                s = CollectionDB::likeCondition( filter, true, mode != modeEndMatch );
        }

        m_where += QString( "OR %1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;

        if ( !exact && ( (value & valName) || (value & valComposer) ) && mode == modeNormal && i18n( "Unknown").contains( filter, false ) )
            m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );
        if ( !exact && ( value & valComposer ) && mode == modeNormal && i18n( "Unknown").contains( filter, false ) )
            m_where += QString( "OR %1.%2 IS NULL " ).arg( tableName( tables ) ).arg( valueName( value ) );

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

            if ( tables & tabAlbum )
                m_where += "OR album.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabArtist )
                m_where += "OR artist.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabGenre )
                m_where += "OR genre.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabYear )
                m_where += "OR year.name " + CollectionDB::likeCondition( filter[i], false, false );
            if ( tables & tabSong )
                m_where += "OR tags.title " + CollectionDB::likeCondition( filter[i], true, true );

            if ( i18n( "Unknown" ).contains( filter[i], false ) )
            {
                if ( tables & tabAlbum )
                    m_where += "OR album.name = '' ";
                if ( tables & tabArtist )
                    m_where += "OR artist.name = '' ";
                if ( tables & tabGenre )
                    m_where += "OR genre.name = '' ";
                if ( tables & tabYear )
                    m_where += "OR year.name = '' ";
                if ( tables & tabSong )
                    m_where += "OR tags.title = '' ";
            }
            if ( i18n( "Various Artists" ).contains( filter[ i ], false ) && ( tables & tabArtist ) )
                m_where += "OR tags.sampler = " + CollectionDB::instance()->boolT() + ' ';
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


        if ( tables & tabAlbum )
            m_where += "AND album.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabArtist )
            m_where += "AND artist.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabGenre )
            m_where += "AND genre.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabYear )
            m_where += "AND year.name NOT " + CollectionDB::likeCondition( filter, false, false );
        if ( tables & tabSong )
            m_where += "AND tags.title NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabComposer )
            m_where += "AND tags.composer NOT " + CollectionDB::likeCondition( filter, true, true );

        if ( i18n( "Unknown" ).contains( filter, false ) )
        {
            if ( tables & tabAlbum )
                m_where += "AND album.name <> '' ";
            if ( tables & tabArtist )
                m_where += "AND artist.name <> '' ";
            if ( tables & tabGenre )
                m_where += "AND genre.name <> '' ";
            if ( tables & tabYear )
                m_where += "AND year.name <> '' ";
            if ( tables & tabSong )
                m_where += "AND tags.title <> '' ";
            if ( tables & tabComposer )
                m_where += "AND tags.composer <> '' AND tags.composer IS NOT NULL ";
        }

       if ( i18n( "Various Artists" ).contains( filter, false ) && (  tables & tabArtist ) )
            m_where += "AND tags.sampler = " + CollectionDB::instance()->boolF() + ' ';


        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::excludeFilter( int tables, Q_INT64 value, const QString& filter, int mode, bool exact )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + " ";

        QString m, s;
        if (mode == modeLess || mode == modeGreater)
            s = ( mode == modeLess ? ">= '" : "<= '" ) + CollectionDB::instance()->escapeString( filter ) + "' ";
        else
        {
            if (exact)
                s = " <> " + CollectionDB::instance()->escapeString( filter );
            else
                s = "NOT " + CollectionDB::instance()->likeCondition( filter, false, false && mode != modeEndMatch ) + " ";
        }

        m_where += QString( "AND %1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;

        if ( !exact && ( (value & valName) || (value & valComposer) ) && mode == modeNormal && i18n( "Unknown").contains( filter, false ) )
            m_where += QString( "AND %1.%2 <> '' " ).arg( tableName( tables ) ).arg( valueName( value ) );
        if ( !exact && ( value & valComposer ) && mode == modeNormal && i18n( "Unknown").contains( filter, false ) )
            m_where += QString( "AND %1.%2 IS NOT NULL " ).arg( tableName( tables ) ).arg( valueName( value ) );

        m_where += " ) ";
    }

    m_linkTables |= tables;
}

void
QueryBuilder::addMatch( int tables, const QString& match, bool interpretUnknown /* = true */, bool caseSensitive /* = false */ )
{
    QString matchCondition = caseSensitive ? CollectionDB::exactCondition( match ) : CollectionDB::likeCondition( match );

    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";
    if ( tables & tabAlbum )
        m_where += "OR album.name " + matchCondition;
    if ( tables & tabArtist )
        m_where += "OR artist.name " + matchCondition;
    if ( tables & tabGenre )
        m_where += "OR genre.name " + matchCondition;
    if ( tables & tabYear )
        m_where += "OR year.name " + matchCondition;
    if ( tables & tabSong )
        m_where += "OR tags.title " + matchCondition;

    if ( interpretUnknown && match == i18n( "Unknown" ) )
    {
        if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
        if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
        if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
        if ( tables & tabYear ) m_where += "OR year.name = '' ";
    }
    m_where += " ) ";

    m_linkTables |= tables;
}


void
QueryBuilder::addMatch( int tables, Q_INT64 value, const QString& match )
{
    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";
    m_where += QString( "OR %1.%2 %3" ).arg( tableName( tables ) ).arg( valueName( value ) ).arg( CollectionDB::likeCondition( match ) );

    if ( ( value & valName ) && match == i18n( "Unknown" ) )
        m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

    m_where += " ) ";

    m_linkTables |= tables;
}


void
QueryBuilder::addMatches( int tables, const QStringList& match, bool interpretUnknown /* = true */, bool caseSensitive /* = false */ )
{
    QStringList matchConditions;
    for ( uint i = 0; i < match.count(); i++ )
        matchConditions << ( caseSensitive ? CollectionDB::exactCondition( match[i] ) : CollectionDB::likeCondition( match[i] ) );

    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + " ";

    for ( uint i = 0; i < match.count(); i++ )
    {
        if ( tables & tabAlbum )
            m_where += "OR album.name " + matchConditions[ i ];
        if ( tables & tabArtist )
            m_where += "OR artist.name " + matchConditions[ i ];
        if ( tables & tabGenre )
            m_where += "OR genre.name " + matchConditions[ i ];
        if ( tables & tabYear )
            m_where += "OR year.name " + matchConditions[ i ];
        if ( tables & tabSong )
            m_where += "OR tags.title " + matchConditions[ i ];
        if ( tables & tabStats )
            m_where += "OR statistics.url " + matchConditions[ i ];


        if ( interpretUnknown && match[i] == i18n( "Unknown" ) )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
            if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
            if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
            if ( tables & tabYear ) m_where += "OR year.name = '' ";
        }
    }

    m_where += " ) ";
    m_linkTables |= tables;
}

void
QueryBuilder::excludeMatch( int tables, const QString& match )
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

    m_linkTables |= tables;
}


void
QueryBuilder::exclusiveFilter( int tableMatching, int tableNotMatching, Q_INT64 value )
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

    if (CollectionDB::instance()->getType() == DbConnection::postgresql && options & optRemoveDuplicates && options & optRandomize)
    {
            m_values = "DISTINCT " + CollectionDB::instance()->randomFunc() + " AS __random "+ m_values;
            if ( !m_sort.isEmpty() )
                m_sort += ",";
            m_sort += CollectionDB::instance()->randomFunc() + " ";
    }
    else
    {
            if ( options & optRemoveDuplicates )
                m_values = "DISTINCT " + m_values;
            if ( options & optRandomize )
            {
                if ( !m_sort.isEmpty() ) m_sort += ",";
                m_sort += CollectionDB::instance()->randomFunc() + " ";
            }
    }
}


void
QueryBuilder::sortBy( int table, Q_INT64 value, bool descending )
{
    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valRating || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate ||
         value & valPercentage || value & valFilesize || value & valDiscNumber ||
         table & tabYear )
        b = false;

	// only coalesce for certain columns
	bool c = false;
    if ( value & valScore || value & valRating || value & valPlayCounter || value & valPercentage || value & valAccessDate || value & valCreateDate )
		c = true;

    if ( !m_sort.isEmpty() ) m_sort += ",";
    if ( b ) m_sort += "LOWER( ";
    if ( c ) m_sort += "COALESCE( ";

    m_sort += tableName( table ) + ".";
    m_sort += valueName( value );

    if ( c ) m_sort += ", 0 )";

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
QueryBuilder::sortByFunction( int function, int table, Q_INT64 value, bool descending )
{
    // This function should be used with the equivalent addReturnFunctionValue (with the same function on same values)
    // since it uses the "func(table.value) AS functablevalue" definition.

    // this column is already coalesced, but need to reconstruct for postgres
    bool defaults = function == funcAvg && ( value & valPercentage || value & valRating );

    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valRating || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate ||
         value & valPercentage || value & valFilesize || value & valDiscNumber ||
         table & tabYear )
        b = false;

    // only coalesce for certain columns
    bool c = false;
    if ( !defaults && ( value & valScore || value & valRating || value & valPlayCounter || value & valPercentage|| value & valAccessDate || value & valCreateDate ) )
        c = true;

    if ( !m_sort.isEmpty() ) m_sort += ",";
    //m_sort += functionName( function ) + "(";
    if ( b ) m_sort += "LOWER( ";
    if ( c && CollectionDB::instance()->getType() != DbConnection::mysql) m_sort += "COALESCE( ";

    QString columnName;

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        columnName = functionName( function ) + "(";
        if ( defaults )
            columnName += "COALESCE(NULLIF(";
        columnName += tableName( table )+"."+valueName( value );
        if ( defaults )
        {
            columnName += ", 0), ";
            if ( value & valPercentage )
                columnName += "50";
            else
                columnName += "6";
            columnName += ")";
        }
        columnName += ")";
    }
    else
        columnName = functionName( function )+tableName( table )+valueName( value );

    m_sort += columnName;

    if ( c && CollectionDB::instance()->getType() != DbConnection::mysql) m_sort += ", 0 )";

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
QueryBuilder::groupBy( int table, Q_INT64 value )
{
    if ( !m_group.isEmpty() ) m_group += ",";
    m_group += tableName( table ) + ".";
    m_group += valueName( value );

    m_linkTables |= table;
}

void
QueryBuilder::having( int table, Q_INT64 value, int function, int mode, const QString& match )
{
    if( !m_having.isEmpty() ) m_having += " AND ";

    QString fn = functionName( function );
    fn.isEmpty() ?
        m_having += tableName( table ) + "." + valueName( value ) :
        m_having += functionName( function )+"("+tableName( table )+"."+valueName( value )+")";

    switch( mode )
    {
        case modeNormal:
            m_having += "=" + match;
            break;

        case modeLess:
            m_having += "<" + match;
            break;

        case modeGreater:
            m_having += ">" + match;

        default:
            break;
    }
}

void
QueryBuilder::setLimit( int startPos, int length )
{
    m_limit = QString( " LIMIT %2 OFFSET %1 " ).arg( startPos ).arg( length );
}


/* NOTE: It's important to keep these two functions and the const in sync! */
const int
QueryBuilder::dragFieldCount = 16;

QString
QueryBuilder::dragSQLFields()
{
    return "album.name, artist.name, genre.name, tags.title, year.name, "
           "tags.comment, tags.track, tags.bitrate, tags.discnumber, "
           "tags.length, tags.samplerate, tags.filesize, tags.url, "
           "tags.sampler, tags.filetype, tags.composer";
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
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valSamplerate );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFilesize );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valIsCompilation );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFileType );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComposer );
}


void
QueryBuilder::buildQuery()
{
    if ( m_query.isEmpty() )
    {
        linkTables( m_linkTables );

        m_query = "SELECT " + m_values + " FROM " + m_tables + " " + m_join + " WHERE " + CollectionDB::instance()->boolT() + " " + m_where;
        // GROUP BY must be before ORDER BY for sqlite
        // HAVING must be between GROUP BY and ORDER BY
        if ( !m_group.isEmpty() )  m_query += " GROUP BY " + m_group;
        if ( !m_having.isEmpty() ) m_query += " HAVING " + m_having;
        if ( !m_sort.isEmpty() )   m_query += " ORDER BY " + m_sort;
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
    m_having = "";

    m_linkTables = 0;
    m_returnValues = 0;
}


QString
QueryBuilder::tableName( int table )
{
    QString tables;

    if ( CollectionDB::instance()->getType() != DbConnection::postgresql )
    {
        if ( table & tabSong )   tables += ",tags";
    }
    if ( table & tabArtist ) tables += ",artist";
    if ( table & tabAlbum )  tables += ",album";
    if ( table & tabGenre )  tables += ",genre";
    if ( table & tabYear )   tables += ",year";
    if ( table & tabStats )  tables += ",statistics";
    if ( table & tabLyrics )  tables += ",lyrics";
    if ( table & tabPodcastChannels ) tables += ",podcastchannels";
    if ( table & tabPodcastEpisodes ) tables += ",podcastepisodes";
    if ( table & tabPodcastFolders ) tables += ",podcasttables";
    if ( CollectionDB::instance()->getType() == DbConnection::postgresql )
    {
        if ( table & tabSong )   tables += ",tags";
    }

    // when there are multiple tables involved, we always need table tags for linking them
    return tables.mid( 1 );
}


QString
QueryBuilder::valueName( Q_INT64 value )
{
    QString values;

    if ( value & valID )          values += "id";
    if ( value & valName )        values += "name";
    if ( value & valURL )         values += "url";
    if ( value & valDirectory )   values += "dir";
    if ( value & valTitle )       values += "title";
    if ( value & valTrack )       values += "track";
    if ( value & valComposer )    values += "composer";
    if ( value & valDiscNumber )  values += "discnumber";
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
    if ( value & valFilesize )    values += "filesize";
    if ( value & valFileType )    values += "filetype";
    if ( value & valIsCompilation)values += "sampler";
    if ( value & valCopyright )   values += "copyright";
    if ( value & valParent )      values += "parent";
    if ( value & valWeblink )     values += "weblink";
    if ( value & valAutoscan )    values += "autoscan";
    if ( value & valFetchtype )   values += "fetchtype";
    if ( value & valAutotransfer )values += "autotransfer";
    if ( value & valPurge )       values += "haspurge";
    if ( value & valPurgeCount )  values += "purgeCount";
    if ( value & valIsNew )       values += "isNew";

    return values;
}

QString
QueryBuilder::functionName( int function )
{
    QString functions = QString::null;

    if ( function & funcCount )     functions += "Count";
    if ( function & funcMax )       functions += "Max";
    if ( function & funcMin )       functions += "Min";
    if ( function & funcAvg )       functions += "Avg";
    if ( function & funcSum )       functions += "Sum";

    return functions;
}


#include "collectiondb.moc"
