// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// (c) 2005 Ian Monroe <ian@monroe.nu>
// (c) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>
// (c) 2005 Isaiah Damron <xepo@trifault.net>
// (c) 2005-2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// (c) 2006 Jonas Hurrelmann <j@outpo.st>
// (c) 2006 Shane King <kde@dontletsstart.com>
// (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>
// (c) 2006 Stanislav Nikolov <valsinats@gmail.com>
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
#include "mountpointmanager.h"    //buildQuery()
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
#include "threadmanager.h"

#include <qbuffer.h>
#include <qcheckbox.h>
#include <qdeepcopy.h>
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
#include <kdialogbase.h>          //checkDatabase()
#include <kglobal.h>
#include <kinputdialog.h>         //setupCoverFetcher()
#include <klineedit.h>            //setupCoverFetcher()
#include <klocale.h>
#include <kmdcodec.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kio/job.h>
#include <kio/netaccess.h>

#include <cmath>                 //DbConnection::sqlite_power()
#include <ctime>                 //query()
#include <cstdlib>               //exit()
#include <unistd.h>              //usleep()

#include <taglib/audioproperties.h>

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

using Amarok::QStringx;

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
INotify::watchDir( const QString directory )
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

    IdList list = MountPointManager::instance()->getMountedDeviceIds();
    QString deviceIds;
    foreachType( IdList, list )
    {
        if ( !deviceIds.isEmpty() ) deviceIds += ',';
        deviceIds += QString::number(*it);
    }
    const QStringList values = m_parent->query( QString( "SELECT dir, deviceid FROM directories WHERE deviceid IN (%1);" )
                                                    .arg( deviceIds ) );
    foreach( values )
    {
        QString rpath = *it;
        int deviceid = (*(++it)).toInt();
        QString abspath = MountPointManager::instance()->getAbsolutePath( deviceid, rpath );
        watchDir( abspath );
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
QMutex* CollectionDB::itemCoverMapMutex = new QMutex();
//we don't have to worry about this map leaking memory since ThreadManager limits the total
//number of QThreads ever created
QMap<QThread *, DbConnection *> *CollectionDB::threadConnections = new QMap<QThread *, DbConnection *>();
QMap<QListViewItem*, CoverFetcher*> *CollectionDB::itemCoverMap = new QMap<QListViewItem*, CoverFetcher*>();

CollectionDB* CollectionDB::instance()
{
    static CollectionDB db;
    return &db;
}


CollectionDB::CollectionDB()
        : EngineObserver( EngineController::instance() )
        , m_autoScoring( true )
        , m_noCover( locate( "data", "amarok/images/nocover.png" ) )
        , m_shadowImage( locate( "data", "amarok/images/shadow_albumcover.png" ) )
        , m_scanInProgress( false )
        , m_rescanRequired( false )
        , m_aftEnabledPersistentTables()
        , m_moveFileJobCancelled( false )
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

    //perform all necessary operations to allow MountPointManager to access the devices table here
    //there is a recursive dependency between CollectionDB and MountPointManager and this is the workaround
    //checkDatabase has to be able to access MountPointManager

    //<OPEN DATABASE>
    initialize();
    //</OPEN DATABASE>

    // Remove cached "nocover" images, so that a new version actually gets shown
    // The asterisk is for also deleting the shadow-caches.
    const QStringList entryList = cacheCoverDir().entryList( "*nocover.png*", QDir::Files );
    foreach( entryList )
        cacheCoverDir().remove( *it );


    connect( this, SIGNAL(fileMoved(const QString&, const QString&, const QString&)),
             this, SLOT(aftMigratePermanentTablesUrl(const QString&, const QString&, const QString&)) );
    connect( this, SIGNAL(uniqueIdChanged(const QString&, const QString&, const QString&)),
             this, SLOT(aftMigratePermanentTablesUniqueId(const QString&, const QString&, const QString&)) );

    connect( qApp, SIGNAL( aboutToQuit() ), this, SLOT( disableAutoScoring() ) );

    connect( this, SIGNAL( coverRemoved( const QString&, const QString& ) ),
                   SIGNAL( coverChanged( const QString&, const QString& ) ) );
    connect( Scrobbler::instance(), SIGNAL( similarArtistsFetched( const QString&, const QStringList& ) ),
             this,                    SLOT( similarArtistsFetched( const QString&, const QStringList& ) ) );

    // If we're supposed to monitor dirs for changes, make sure we run it once
    // on startup, since inotify can't inform us about old events
//     QTimer::singleShot( 0, this, SLOT( scanMonitor() ) )
    initDirOperations();
    m_aftEnabledPersistentTables << "lyrics" << "statistics" << "tags_labels";
}


CollectionDB::~CollectionDB()
{
    DEBUG_BLOCK

#ifdef HAVE_INOTIFY
    if ( INotify::instance()->fd() >= 0 )
        close( INotify::instance()->fd() );
#endif

    destroy();
}


inline QString
CollectionDB::exactCondition( const QString &right )
{
    if ( DbConnection::mysql == instance()->getDbConnectionType() )
        return QString( "= BINARY '" + instance()->escapeString( right ) + '\'' );
    else
        return QString( "= '" + instance()->escapeString( right ) + '\'' );
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

    ret += '\'';
    if ( anyBegin )
            ret += '%';
    ret += escaped;
    if ( anyEnd )
            ret += '%';
    ret += '\'';

    //Use / as the escape character
    ret += " ESCAPE '/' ";

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::initDirOperations()
{
    //this code was originally part of the ctor. It has to call MountPointManager to
    //generate absolute paths from deviceids and relative paths. MountPointManager's ctor
    //absolutely has to access the database, which resulted in a recursive ctor call. To
    //solve this problem, the directory access code was moved into its own method, which can
    //only be called when the CollectionDB object already exists.

    //FIXME max: make sure we check additional directories if we connect a new device
#ifdef HAVE_INOTIFY
    // Try to initialize inotify, if not available use the old timer approach.
    int inotify_fd = inotify_init();
    if ( inotify_fd < 0 )
#endif
    {
//         debug() << "INotify not available, using QTimer!" << endl;
        startTimer( MONITOR_INTERVAL * 1000 );
    }
#ifdef HAVE_INOTIFY
    else
    {
        debug() << "INotify enabled!" << endl;
        ThreadManager::instance()->onlyOneJob( new INotify( this, inotify_fd ) );
    }
#endif

}


/**
 * Executes a SQL query on the already opened database
 * @param statement SQL program to execute. Only one SQL statement is allowed.
 * @return          The queried data, or QStringList() on error.
 */
QStringList
CollectionDB::query( const QString& statement, bool suppressDebug )
{
    m_mutex.lock();
    clock_t start;
    if ( DEBUG )
    {
        debug() << "Query-start: " << statement << endl;
        start = clock();
    }
    if ( statement.stripWhiteSpace().isEmpty() )
    {
        m_mutex.unlock();
        return QStringList();
    }

    DbConnection *dbConn;
    dbConn = getMyConnection();

    QStringList values = dbConn->query( statement, suppressDebug );
    if ( DEBUG )
    {
        clock_t finish = clock();
        const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
        debug() << "SQL-query (" << duration << "s): " << statement << endl;
    }
    m_mutex.unlock();
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
    m_mutex.lock();
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
    m_mutex.unlock();
    return id;
}

QString
CollectionDB::deviceidSelection( const bool showAll )
{
    if ( !showAll )
    {
        IdList list = MountPointManager::instance()->getMountedDeviceIds();
        QString deviceIds = "";
        foreachType( IdList, list )
        {
            if ( it != list.begin() ) deviceIds += ',';
            deviceIds += QString::number(*it);
        }
        return " AND tags.deviceid IN (" + deviceIds + ')';
    }
    else return "";
}

QStringList
CollectionDB::URLsFromQuery( const QStringList &result ) const
{
    QStringList values;
    foreach( result )
    {
        const int id = (*it).toInt();
        values << MountPointManager::instance()->getAbsolutePath( id, *(++it) );
    }
    return values;
}

KURL::List
CollectionDB::URLsFromSqlDrag( const QStringList &values ) const
{
    KURL::List urls;
    for( QStringList::const_iterator it = values.begin();
            it != values.end();
            it++ )
    {
        const QString &rel = *it;
        it++;
        int id = (*it).toInt();
        urls += KURL::fromPathOrURL( MountPointManager::instance()->getAbsolutePath( id, rel ) );
        for( int i = 0;
                i < QueryBuilder::dragFieldCount-1 && it != values.end();
                i++ )
            it++;
    }

    return urls;
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
    QStringList values5;

    values1 = query( "SELECT COUNT( url ) FROM tags LIMIT 1 OFFSET 0;" );
    values2 = query( "SELECT COUNT( url ) FROM statistics LIMIT 1 OFFSET 0;" );
    values3 = query( "SELECT COUNT( url ) FROM podcastchannels LIMIT 1 OFFSET 0;" );
    values4 = query( "SELECT COUNT( url ) FROM podcastepisodes LIMIT 1 OFFSET 0;" );
    values5 = query( "SELECT COUNT( id ) FROM devices LIMIT 1 OFFSET 0;" );

    //It's valid as long as we've got _some_ tables that have something in.
    return !( values1.isEmpty() && values2.isEmpty() && values3.isEmpty() && values4.isEmpty() && values5.isEmpty() );
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
                    "url " + exactTextColumnType() + ","
                    "dir " + exactTextColumnType() + ","
                    "createdate INTEGER,"
                    "modifydate INTEGER,"
                    "album INTEGER,"
                    "artist INTEGER,"
                    "composer INTEGER,"
                    "genre INTEGER,"
                    "title " + textColumnType() + ","
                    "year INTEGER,"
                    "comment " + longTextColumnType() + ","
                    "track NUMERIC(4),"
                    "discnumber INTEGER,"
                    "bitrate INTEGER,"
                    "length INTEGER,"
                    "samplerate INTEGER,"
                    "filesize INTEGER,"
                    "filetype INTEGER,"
                    "sampler BOOL,"
                    "bpm FLOAT,"
                    "deviceid INTEGER);" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    QString albumAutoIncrement = "";
    QString artistAutoIncrement = "";
    QString composerAutoIncrement = "";
    QString genreAutoIncrement = "";
    QString yearAutoIncrement = "";
    if ( getDbConnectionType() == DbConnection::postgresql )
    {
        if(!temporary)
        {
            query( QString( "CREATE SEQUENCE album_seq;" ) );
            query( QString( "CREATE SEQUENCE artist_seq;" ) );
            query( QString( "CREATE SEQUENCE composer_seq;" ) );
            query( QString( "CREATE SEQUENCE genre_seq;" ) );
            query( QString( "CREATE SEQUENCE year_seq;" ) );
        }

        albumAutoIncrement = QString("DEFAULT nextval('album_seq')");
        artistAutoIncrement = QString("DEFAULT nextval('artist_seq')");
        composerAutoIncrement = QString("DEFAULT nextval('composer_seq')");
        genreAutoIncrement = QString("DEFAULT nextval('genre_seq')");
        yearAutoIncrement = QString("DEFAULT nextval('year_seq')");
    }
    else if ( getDbConnectionType() == DbConnection::mysql )
    {
        albumAutoIncrement = "AUTO_INCREMENT";
        artistAutoIncrement = "AUTO_INCREMENT";
        composerAutoIncrement = "AUTO_INCREMENT";
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

    //create composer table
    query( QString( "CREATE %1 TABLE composer%2 ("
                    "id INTEGER PRIMARY KEY %3,"
                    "name " + textColumnType() + ");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" )
                    .arg( composerAutoIncrement ) );

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
                    "path " + exactTextColumnType() + ","
                    "deviceid INTEGER,"
                    "artist " + textColumnType() + ","
                    "album " + textColumnType() + ");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create embed table
    query( QString( "CREATE %1 TABLE embed%2 ("
                    "url " + exactTextColumnType() + ","
                    "deviceid INTEGER,"
                    "hash " + exactTextColumnType() + ","
                    "description " + textColumnType() + ");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    // create directory statistics table
    query( QString( "CREATE %1 TABLE directories%2 ("
                    "dir " + exactTextColumnType() + ","
                    "deviceid INTEGER,"
                    "changedate INTEGER);" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create uniqueid table
    query( QString( "CREATE %1 TABLE uniqueid%2 ("
                    "url " + exactTextColumnType() + ","
                    "deviceid INTEGER,"
                    "uniqueid " + exactTextColumnType(32) + " UNIQUE,"
                    "dir " + exactTextColumnType() + ");" )
                    .arg( temporary ? "TEMPORARY" : "" )
                    .arg( temporary ? "_temp" : "" ) );

    //create indexes
    query( QString( "CREATE INDEX album_idx%1 ON album%2( name );" )
                    .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "CREATE INDEX artist_idx%1 ON artist%2( name );" )
                    .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "CREATE INDEX composer_idx%1 ON composer%2( name );" )
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

        createIndices();
    }
    else
    {
        query( "CREATE UNIQUE INDEX url_tagtemp ON tags_temp( url, deviceid );" );
        query( "CREATE UNIQUE INDEX embed_urltemp ON embed_temp( url, deviceid );" );
        query( "CREATE UNIQUE INDEX dir_temp_dir ON directories_temp( dir, deviceid );" );
    }
}

void
CollectionDB::createIndices()
{
    //This creates the indices for tables created in createTables. It should not refer to
    //tables which are not created in that function.
    debug() << "Creating indices, ignore errors about already existing indices" << endl;

    query( "CREATE UNIQUE INDEX url_tag ON tags( url, deviceid );" );
    query( "CREATE INDEX album_tag ON tags( album );" );
    query( "CREATE INDEX artist_tag ON tags( artist );" );
    query( "CREATE INDEX composer_tag ON tags( composer );" );
    query( "CREATE INDEX genre_tag ON tags( genre );" );
    query( "CREATE INDEX year_tag ON tags( year );" );
    query( "CREATE INDEX sampler_tag ON tags( sampler );" );

    query( "CREATE INDEX images_album ON images( album );" );
    query( "CREATE INDEX images_artist ON images( artist );" );

    query( "CREATE INDEX images_url ON images( path, deviceid );" );

    query( "CREATE UNIQUE INDEX embed_url ON embed( url, deviceid );" );
    query( "CREATE INDEX embed_hash ON embed( hash );" );

    query( "CREATE UNIQUE INDEX directories_dir ON directories( dir, deviceid );" );
    query( "CREATE INDEX uniqueid_uniqueid ON uniqueid( uniqueid );");
    query( "CREATE INDEX uniqueid_url ON uniqueid( url, deviceid );");

    query( "CREATE INDEX album_idx ON album( name );" );
    query( "CREATE INDEX artist_idx ON artist( name );" );
    query( "CREATE INDEX composer_idx ON composer( name );" );
    query( "CREATE INDEX genre_idx ON genre( name );" );
    query( "CREATE INDEX year_idx ON year( name );" );

    query( "CREATE INDEX tags_artist_index ON tags( artist );" );
    query( "CREATE INDEX tags_album_index ON tags( album );" );
    query( "CREATE INDEX tags_deviceid_index ON tags( deviceid ); ");
    query( "CREATE INDEX tags_url_index ON tags( url ); ");

    query( "CREATE INDEX embed_deviceid_index ON embed( deviceid ); ");
    query( "CREATE INDEX embed_url_index ON embed( url ); "); 

    query( "CREATE INDEX related_artists_artist ON related_artists( artist );" );

    debug() << "Finished creating indices, stop ignoring errors" << endl;
}

void
CollectionDB::createPermanentIndices()
{
    //this method creates all indices which are not referred to in createTables
    //this method is called on each startup of amarok
    //until we figure out a way to handle this better it produces SQL errors if the indices
    //already exist, but these can be ignored
    debug() << "Creating permanent indices, ignore errors about already existing indices" << endl;

    query( "CREATE UNIQUE INDEX lyrics_url ON lyrics( url, deviceid );" );
    query( "CREATE INDEX lyrics_uniqueid ON lyrics( uniqueid );" );
    query( "CREATE INDEX playlist_playlists ON playlists( playlist );" );
    query( "CREATE INDEX url_playlists ON playlists( url );" );
    query( "CREATE UNIQUE INDEX labels_name ON labels( name, type );" );
    query( "CREATE INDEX tags_labels_uniqueid ON tags_labels( uniqueid );" ); //m:n relationship, DO NOT MAKE UNIQUE!
    query( "CREATE INDEX tags_labels_url ON tags_labels( url, deviceid );" ); //m:n relationship, DO NOT MAKE UNIQUE!
    query( "CREATE INDEX tags_labels_labelid ON tags_labels( labelid );" );   //m:n relationship, DO NOT MAKE UNIQUE!

    query( "CREATE UNIQUE INDEX url_stats ON statistics( deviceid, url );" );
    query( "CREATE INDEX percentage_stats ON statistics( percentage );" );
    query( "CREATE INDEX rating_stats ON statistics( rating );" );
    query( "CREATE INDEX playcounter_stats ON statistics( playcounter );" );
    query( "CREATE INDEX uniqueid_stats ON statistics( uniqueid );" );

    query( "CREATE INDEX url_podchannel ON podcastchannels( url );" );
    query( "CREATE INDEX url_podepisode ON podcastepisodes( url );" );
    query( "CREATE INDEX localurl_podepisode ON podcastepisodes( localurl );" );
    query( "CREATE INDEX url_podfolder ON podcastfolders( id );" );

    debug() << "Finished creating permanent indices, stop ignoring errors" << endl;
}


void
CollectionDB::dropTables( const bool temporary )
{
    query( QString( "DROP TABLE tags%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE album%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE artist%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE composer%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE genre%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE year%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE images%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE embed%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE directories%1;" ).arg( temporary ? "_temp" : "" ) );
    query( QString( "DROP TABLE uniqueid%1;" ).arg( temporary ? "_temp" : "" ) );
    if ( !temporary )
    {
        query( QString( "DROP TABLE related_artists;" ) );
        debug() << "Dropping media table" << endl;
    }

    if ( getDbConnectionType() == DbConnection::postgresql )
    {
        if (temporary == false) {
            query( QString( "DROP SEQUENCE album_seq;" ) );
            query( QString( "DROP SEQUENCE artist_seq;" ) );
            query( QString( "DROP SEQUENCE composer_seq;" ) );
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
    query( QString( "%1 composer%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 genre%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 year%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 images%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 embed%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 directories%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    query( QString( "%1 uniqueid%2;" ).arg( clearCommand ).arg( temporary ? "_temp" : "" ) );
    if ( !temporary )
    {
        query( QString( "%1 related_artists;" ).arg( clearCommand ) );
        //debug() << "Clearing media table" << endl;
        //query( QString( "%1 media;" ).arg( clearCommand ) );
    }
}


void
CollectionDB::copyTempTables( )
{
    DEBUG_BLOCK

    insert( "INSERT INTO tags SELECT * FROM tags_temp;", NULL );

    //mysql 5 supports subqueries with IN, mysql 4 doesn't. this way will work for all SQL servers
    QStringList albumIdList = query( "SELECT album.id FROM album;" );
    //in an empty database, albumIdList is empty. This would result in a SQL query like NOT IN ( ) without
    //the -1 below which is invalid SQL. The auto generated values start at 1 so this is fine
    QString albumIds = "-1";
    foreach( albumIdList )
    {
        albumIds += ',';
        albumIds += *it;
    }
    insert( QString ( "INSERT INTO album SELECT * FROM album_temp WHERE album_temp.id NOT IN ( %1 );" ).arg( albumIds ), NULL );

    QStringList artistIdList = query( "SELECT artist.id FROM artist;" );
    QString artistIds = "-1";
    foreach( artistIdList )
    {
        artistIds += ',';
        artistIds += *it;
    }
    insert( QString ( "INSERT INTO artist SELECT * FROM artist_temp WHERE artist_temp.id NOT IN ( %1 );" ).arg( artistIds ), NULL );

    QStringList composerIdList = query( "SELECT composer.id FROM composer;" );
    QString composerIds = "-1";
    foreach( composerIdList )
    {
        composerIds += ',';
        composerIds += *it;
    }
    insert( QString ( "INSERT INTO composer SELECT * FROM composer_temp WHERE composer_temp.id NOT IN ( %1 );" ).arg( composerIds ), NULL );

    QStringList genreIdList = query( "SELECT genre.id FROM genre;" );
    QString genreIds = "-1";
    foreach( genreIdList )
    {
        genreIds += ',';
        genreIds += *it;
    }
    insert( QString ( "INSERT INTO genre SELECT * FROM genre_temp WHERE genre_temp.id NOT IN ( %1 );" ).arg( genreIds ), NULL );

    QStringList yearIdList = query( "SELECT year.id FROM year;" );
    QString yearIds = "-1";
    foreach( yearIdList )
    {
        yearIds += ',';
        yearIds += *it;
    }
    insert( QString ( "INSERT INTO year SELECT * FROM year_temp WHERE year_temp.id NOT IN ( %1 );" ).arg( yearIds ), NULL );

    insert( "INSERT INTO images SELECT * FROM images_temp;", NULL );
    insert( "INSERT INTO embed SELECT * FROM embed_temp;", NULL );
    insert( "INSERT INTO directories SELECT * FROM directories_temp;", NULL );
    insert( "INSERT INTO uniqueid SELECT * FROM uniqueid_temp;", NULL );
}

void
CollectionDB::prepareTempTables()
{
    DEBUG_BLOCK
    insert( "INSERT INTO album_temp SELECT * from album;", 0 );
    insert( "INSERT INTO artist_temp SELECT * from artist;", 0 );
    insert( "INSERT INTO composer_temp SELECT * from composer;", 0 );
    insert( "INSERT INTO genre_temp SELECT * from genre;", 0 );
    insert( "INSERT INTO year_temp SELECT * from year;", 0 );
}

void
CollectionDB::createDevicesTable()
{
    debug() << "Creating DEVICES table" << endl;
    QString deviceAutoIncrement = "";
    if ( getDbConnectionType() == DbConnection::postgresql )
    {
        query( QString( "CREATE SEQUENCE devices_seq;" ) );
        deviceAutoIncrement = QString("DEFAULT nextval('devices_seq')");
    }
    else if ( getDbConnectionType() == DbConnection::mysql )
    {
        deviceAutoIncrement = "AUTO_INCREMENT";
    }
    query( QString( "CREATE TABLE devices ("
                    "id INTEGER PRIMARY KEY %1,"
                    "type " + textColumnType() + ","
                    "label " + textColumnType() + ","
                    "lastmountpoint " + textColumnType() + ","
                    "uuid " + textColumnType() + ","
                    "servername " + textColumnType() + ","
                    "sharename " + textColumnType() + ");" )
                 .arg( deviceAutoIncrement ) );
    query( "CREATE INDEX devices_type ON devices( type );" );
    query( "CREATE INDEX devices_uuid ON devices( uuid );" );
    query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );
}

void
CollectionDB::createStatsTable()
{
    // create music statistics database
    query( QString( "CREATE TABLE statistics ("
                    "url " + exactTextColumnType() + ","
                    "deviceid INTEGER,"
                    "createdate INTEGER,"
                    "accessdate INTEGER,"
                    "percentage FLOAT,"
                    "rating INTEGER DEFAULT 0,"
                    "playcounter INTEGER,"
                    "uniqueid " + exactTextColumnType(32) + " UNIQUE,"
                    "deleted BOOL DEFAULT " + boolF() + ","
                    "PRIMARY KEY(url, deviceid) );" ) );

}

//Old version, used in upgrade code. This should never be changed.
void
CollectionDB::createStatsTableV8()
{
    // create music statistics database - old form, for upgrade code.
    query( QString( "CREATE TABLE statistics ("
                    "url " + textColumnType() + " UNIQUE,"
                    "createdate INTEGER,"
                    "accessdate INTEGER,"
                    "percentage FLOAT,"
                    "rating INTEGER DEFAULT 0,"
                    "playcounter INTEGER,"
                    "uniqueid " + textColumnType(8) + " UNIQUE,"
                    "deleted BOOL DEFAULT " + boolF() + ");" ) );

    query( "CREATE INDEX url_stats ON statistics( url );" );
    query( "CREATE INDEX percentage_stats ON statistics( percentage );" );
    query( "CREATE INDEX rating_stats ON statistics( rating );" );
    query( "CREATE INDEX playcounter_stats ON statistics( playcounter );" );
    query( "CREATE INDEX uniqueid_stats ON statistics( uniqueid );" );
}

//Old version, used in upgrade code
void
CollectionDB::createStatsTableV10( bool temp )
{
    // create music statistics database
    query( QString( "CREATE %1 TABLE statistics%2 ("
                    "url " + exactTextColumnType() + ","
                    "deviceid INTEGER,"
                    "createdate INTEGER,"
                    "accessdate INTEGER,"
                    "percentage FLOAT,"
                    "rating INTEGER DEFAULT 0,"
                    "playcounter INTEGER,"
                    "uniqueid " + exactTextColumnType(32) + " UNIQUE,"
                    "deleted BOOL DEFAULT " + boolF() + ","
                    "PRIMARY KEY(url, deviceid) );"
                    ).arg( temp ? "TEMPORARY" : "" )
                     .arg( temp ? "_fix_ten" : "" ) );

    if ( !temp )
    {
        query( "CREATE UNIQUE INDEX url_stats ON statistics( deviceid, url );" );
        query( "CREATE INDEX percentage_stats ON statistics( percentage );" );
        query( "CREATE INDEX rating_stats ON statistics( rating );" );
        query( "CREATE INDEX playcounter_stats ON statistics( playcounter );" );
        query( "CREATE INDEX uniqueid_stats ON statistics( uniqueid );" );
    }
}


void
CollectionDB::dropStatsTable()
{
    query( "DROP TABLE statistics;" );
}

void
CollectionDB::dropStatsTableV1()
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
            "filename " + exactTextColumnType(33) + ", "
            "refetchdate INTEGER );" );

    // create lyrics table
    query( QString( "CREATE TABLE lyrics ("
            "url " + exactTextColumnType() + ", "
            "deviceid INTEGER,"
            "lyrics " + longTextColumnType() + ", "
            "uniqueid " + exactTextColumnType(32) + ");" ) );

    query( QString( "CREATE TABLE playlists ("
            "playlist " + textColumnType() + ", "
            "url " + exactTextColumnType() + ", "
            "tracknum INTEGER );" ) );

    QString labelsAutoIncrement = "";
    if ( getDbConnectionType() == DbConnection::postgresql )
    {
        query( QString( "CREATE SEQUENCE labels_seq;" ) );

        labelsAutoIncrement = QString("DEFAULT nextval('labels_seq')");
    }
    else if ( getDbConnectionType() == DbConnection::mysql )
    {
        labelsAutoIncrement = "AUTO_INCREMENT";
    }

    //create labels tables
    query( QString( "CREATE TABLE labels ("
                    "id INTEGER PRIMARY KEY " + labelsAutoIncrement + ", "
                    "name " + textColumnType() + ", "
                    "type INTEGER);" ) );

    query( QString( "CREATE TABLE tags_labels ("
                    "deviceid INTEGER,"
                    "url " + exactTextColumnType() + ", "
                    "uniqueid " + exactTextColumnType(32) + ", "      //m:n relationship, DO NOT MAKE UNIQUE!
                    "labelid INTEGER REFERENCES labels( id ) ON DELETE CASCADE );" ) );
}

void
CollectionDB::createPersistentTablesV12()
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
CollectionDB::createPersistentTablesV14( bool temp )
{
    const QString a( temp ? "TEMPORARY" : "" );
    const QString b( temp ? "_fix" : "" );

    // create amazon table
    query(  QString( "CREATE %1 TABLE amazon%2 ( "
            "asin " + textColumnType(20) + ", "
            "locale " + textColumnType(2) + ", "
            "filename " + exactTextColumnType(33) + ", "
            "refetchdate INTEGER );" ).arg( a,b ) );

    // create lyrics table
    query( QString( "CREATE %1 TABLE lyrics%2 ("
            "url " + exactTextColumnType() + ", "
            "deviceid INTEGER,"
            "lyrics " + longTextColumnType() + ");" ).arg( a,b ) );

    query( QString( "CREATE %1 TABLE playlists%2 ("
            "playlist " + textColumnType() + ", "
            "url " + exactTextColumnType() + ", "
            "tracknum INTEGER );" ).arg( a,b ) );

    if ( !temp )
    {
        query(  "CREATE UNIQUE INDEX lyrics_url ON lyrics( url, deviceid );" );
        query( "CREATE INDEX playlist_playlists ON playlists( playlist );" );
        query( "CREATE INDEX url_playlists ON playlists( url );" );
    }
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
                    "url " + exactTextColumnType() + " UNIQUE,"
                    "title " + textColumnType() + ","
                    "weblink " + exactTextColumnType() + ","
                    "image " + exactTextColumnType() + ","
                    "comment " + longTextColumnType() + ","
                    "copyright "  + textColumnType() + ","
                    "parent INTEGER,"
                    "directory "  + textColumnType() + ","
                    "autoscan BOOL, fetchtype INTEGER, "
                    "autotransfer BOOL, haspurge BOOL, purgecount INTEGER );" ) );

    // create podcast episodes table
    query( QString( "CREATE TABLE podcastepisodes ("
                    "id INTEGER PRIMARY KEY %1, "
                    "url " + exactTextColumnType() + " UNIQUE,"
                    "localurl " + exactTextColumnType() + ","
                    "parent " + exactTextColumnType() + ","
                    "guid " + exactTextColumnType() + ","
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
CollectionDB::createPodcastTablesV2( bool temp )
{
    const QString a( temp ? "TEMPORARY" : "" );
    const QString b( temp ? "_fix" : "" );

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
    query( QString( "CREATE %1 TABLE podcastchannels%2 ("
                    "url " + exactTextColumnType() + " UNIQUE,"
                    "title " + textColumnType() + ","
                    "weblink " + exactTextColumnType() + ","
                    "image " + exactTextColumnType() + ","
                    "comment " + longTextColumnType() + ","
                    "copyright "  + textColumnType() + ","
                    "parent INTEGER,"
                    "directory "  + textColumnType() + ","
                    "autoscan BOOL, fetchtype INTEGER, "
                    "autotransfer BOOL, haspurge BOOL, purgecount INTEGER );" ).arg( a,b ) );

    // create podcast episodes table
    query( QString( "CREATE %2 TABLE podcastepisodes%3 ("
                    "id INTEGER PRIMARY KEY %1, "
                    "url " + exactTextColumnType() + " UNIQUE,"
                    "localurl " + exactTextColumnType() + ","
                    "parent " + exactTextColumnType() + ","
                    "guid " + exactTextColumnType() + ","
                    "title " + textColumnType() + ","
                    "subtitle " + textColumnType() + ","
                    "composer " + textColumnType() + ","
                    "comment " + longTextColumnType() + ","
                    "filetype "  + textColumnType() + ","
                    "createdate "  + textColumnType() + ","
                    "length INTEGER,"
                    "size INTEGER,"
                    "isNew BOOL );" )
                    .arg( podcastAutoIncrement, a, b ) );

    // create podcast folders table
    query( QString( "CREATE %2 TABLE podcastfolders%3 ("
                    "id INTEGER PRIMARY KEY %1, "
                    "name " + textColumnType() + ","
                    "parent INTEGER, isOpen BOOL );" )
                    .arg( podcastFolderAutoInc, a, b ) );

    if ( !temp )
    {
        query( "CREATE INDEX url_podchannel ON podcastchannels( url );" );
        query( "CREATE INDEX url_podepisode ON podcastepisodes( url );" );
        query( "CREATE INDEX localurl_podepisode ON podcastepisodes( localurl );" );
        query( "CREATE INDEX url_podfolder ON podcastfolders( id );" );
    }
}


void
CollectionDB::dropPersistentTables()
{
    query( "DROP TABLE amazon;" );
    query( "DROP TABLE lyrics;" );
    query( "DROP TABLE playlists;" );
    query( "DROP TABLE tags_labels;" );
    query( "DROP TABLE labels;" );
}

void
CollectionDB::dropPersistentTablesV14()
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

void
CollectionDB::dropPodcastTablesV2()
{
    query( "DROP TABLE podcastchannels;" );
    query( "DROP TABLE podcastepisodes;" );
    query( "DROP TABLE podcastfolders;" );
}

void
CollectionDB::dropDevicesTable()
{
    query( "DROP TABLE devices;" );
}

uint
CollectionDB::artistID( QString value, bool autocreate, const bool temporary, bool exact /* = true */ )
{
    // lookup cache
    if ( m_validArtistCache && m_cacheArtist[(int)temporary] == value )
        return m_cacheArtistID[(int)temporary];

    uint id;
    if ( exact )
        id = IDFromExactValue( "artist", value, autocreate, temporary ).toUInt();
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
CollectionDB::composerID( QString value, bool autocreate, const bool temporary, bool exact /* = true */ )
{
    // lookup cache
    if ( m_validComposerCache && m_cacheComposer[(int)temporary] == value )
        return m_cacheComposerID[(int)temporary];

    uint id;
    if ( exact )
        id = IDFromExactValue( "composer", value, autocreate, temporary ).toUInt();
    else
        id = IDFromValue( "composer", value, autocreate, temporary );

    // cache values
    m_cacheComposer[(int)temporary] = value;
    m_cacheComposerID[(int)temporary] = id;
    m_validComposerCache = 1;

    return id;
}


QString
CollectionDB::composerValue( uint id )
{
    // lookup cache
    if ( m_cacheComposerID[0] == id )
        return m_cacheComposer[0];

    QString value = valueFromID( "composer", id );

    // cache values
    m_cacheComposer[0] = value;
    m_cacheComposerID[0] = id;

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
        id = IDFromExactValue( "album", value, autocreate, temporary ).toUInt();
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
        IDFromExactValue( "genre", value, autocreate, temporary ).toUInt() :
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
        IDFromExactValue( "year", value, autocreate, temporary ).toUInt() :
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
CollectionDB::albumTracks( const QString &artist_id, const QString &album_id )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addMatch( QueryBuilder::tabAlbum, QueryBuilder::valID, album_id );
    const bool isCompilation = albumIsCompilation( album_id );
    if( !isCompilation )
        qb.addMatch( QueryBuilder::tabArtist, QueryBuilder::valID, artist_id );
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
    QStringList rs;
    rs = query( QString( "SELECT tags.deviceid, tags.url FROM tags, year WHERE tags.album = %1 AND "
                         "tags.artist = %2 AND year.id = tags.year AND tags.discnumber = %3 "
                         + deviceidSelection() + " ORDER BY tags.track;" )
                .arg( album_id )
                .arg( artist_id )
                .arg( discNumber ) );
    QStringList result;
    foreach( rs )
    {
        const int id = (*it).toInt();
        result << MountPointManager::instance()->getAbsolutePath( id, *(++it) );
    }
    return result;
}

QStringList
CollectionDB::artistTracks( const QString &artist_id )
{
    QStringList rs = query( QString( "SELECT tags.deviceid, tags.url FROM tags, album "
                "WHERE tags.artist = '%1' AND album.id = tags.album " + deviceidSelection() +
                "ORDER BY album.name, tags.discnumber, tags.track;" )
            .arg( artist_id ) );
    QStringList result = QStringList();
    foreach( rs )
    {
        const int id = (*it).toInt();
        result << MountPointManager::instance()->getAbsolutePath( id, *(++it) );
    }
    return result;
}


void
CollectionDB::addImageToAlbum( const QString& image, QValueList< QPair<QString, QString> > info, const bool temporary )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( image );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, image );
    for ( QValueList< QPair<QString, QString> >::ConstIterator it = info.begin(); it != info.end(); ++it )
    {
        if ( (*it).first.isEmpty() || (*it).second.isEmpty() )
            continue;

        QString sql = QString( "INSERT INTO images%1 ( path, deviceid, artist, album ) VALUES ( '%3', %2" )
                            .arg( temporary ? "_temp" : "" )
                            .arg( deviceid )
                            .arg( escapeString( rpath ) );
        sql += QString( ", '%1'" ).arg( escapeString( (*it).first ) );
        sql += QString( ", '%1' );" ).arg( escapeString( (*it).second ) );

//         debug() << "Added image for album: " << (*it).first << " - " << (*it).second << ": " << image << endl;
        insert( sql, NULL );
    }
}

void
CollectionDB::addEmbeddedImage( const QString& path, const QString& hash, const QString& description )
{
//     debug() << "Added embedded image hash " << hash << " for file " << path << endl;
    //TODO: figure out what this embedded table does and then add the necessary code
    //what are embedded images anyway?
    int deviceid = MountPointManager::instance()->getIdForUrl( path );
    QString rpath = MountPointManager::instance()->getRelativePath(deviceid, path );
    insert( QString( "INSERT INTO embed_temp ( url, deviceid, hash, description ) VALUES ( '%2', %1, '%3', '%4' );" )
                .arg( deviceid )
                .arg( escapeString( rpath ), escapeString( hash ), escapeString( description ) ), NULL );
}

void
CollectionDB::removeOrphanedEmbeddedImages()
{
    //TODO refactor
    // do it the hard way, since a delete subquery wont work on MySQL
    QStringList orphaned = query( "SELECT embed.deviceid, embed.url FROM embed LEFT JOIN tags ON embed.url = tags.url AND embed.deviceid = tags.deviceid WHERE tags.url IS NULL;" );
    foreach( orphaned ) {
        QString deviceid = *it;
        QString rpath = *(++it);
        query( QString( "DELETE FROM embed WHERE embed.deviceid = %1 AND embed.url = '%2';" )
                .arg( deviceid, escapeString( rpath ) ) );
    }
}

QPixmap
CollectionDB::createDragPixmapFromSQL( const QString &sql, QString textOverRide )
{
    // it is too slow to check if the url is actually in the colleciton.
    //TODO mountpointmanager: figure out what has to be done here
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

            QString artist = mb.artist();
            if( mb.compilation() == MetaBundle::CompilationYes )
                artist = QString( "Various_AMAROK_Artists" ); // magic key for the albumMap!

            if( !albumMap.contains( artist + album ) )
            {
                albumMap[ artist + album ] = 1;
                QString coverName = CollectionDB::instance()->albumImage( mb.artist(), album, false, coverW );

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

                if ( covers < maxCovers && !coverName.endsWith( "@nocover.png" ) )
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
        // insert "..." cover as first image if appropriate
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
    Amarok::OverrideCursor keep;

    const bool isCompilation = albumIsCompilation( QString::number( albumID( album, false, false, true ) ) );
    const QString artist_ = isCompilation ? "" : artist;

    // remove existing album covers
    removeAlbumImage( artist_, album );

    QCString key = md5sum( artist_, album );
    newAmazonReloadDate(asin, AmarokConfig::amazonLocale(), key);
    // Save Amazon product page URL as embedded string, for later retreival
    if ( !amazonUrl.isEmpty() )
        img.setText( "amazon-url", 0, amazonUrl );

    const bool b = img.save( largeCoverDir().filePath( key ), "PNG");
    emit coverChanged( artist_, album );
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
        s = findAmazonImage( "", album, width ); // handle compilations

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
CollectionDB::albumImage( const MetaBundle &trackInformation, bool withShadow, uint width, bool* embedded )
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
        s = findAmazonImage( "", album, width ); // handle compilations
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
    qApp->lock();
    const QImage original( albumImage, "PNG" );
    qApp->unlock();

    if( original.hasAlphaBuffer() )
        return albumImage;

    const QFileInfo fileInfo( albumImage );
    const uint shadowSize = static_cast<uint>( original.width() / 100.0 * 6.0 );
    const QString cacheFile = fileInfo.fileName() + "@shadow";

    if ( !cache && cacheCoverDir().exists( cacheFile ) )
        return cacheCoverDir().filePath( cacheFile );

    QImage shadow;

    const QString folder = Amarok::saveLocation( "covershadow-cache/" );
    const QString file = QString( "shadow_albumcover%1x%2.png" ).arg( original.width() + shadowSize ).arg( original.height() + shadowSize  );
    if ( QFile::exists( folder + file ) ) {
        qApp->lock();
        shadow.load( folder + file, "PNG" );
        qApp->unlock();
    }
    else {
        shadow = QDeepCopy<QImage>(instance()->m_shadowImage);
        shadow = shadow.smoothScale( original.width() + shadowSize, original.height() + shadowSize );
        shadow.save( folder + file, "PNG" );
    }

    QImage target(shadow);
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
        return QString();

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

    return QString();
}


QString
CollectionDB::findDirectoryImage( const QString& artist, const QString& album, uint width )
{
    if ( width == 1 )
        width = AmarokConfig::coverPreviewSize();
    QCString widthKey = makeWidthKey( width );
    if ( album.isEmpty() )
        return QString();

    IdList list = MountPointManager::instance()->getMountedDeviceIds();
    QString deviceIds;
    foreachType( IdList, list )
    {
        if ( !deviceIds.isEmpty() ) deviceIds += ',';
        deviceIds += QString::number(*it);
    }

    QStringList rs;
    if ( artist == i18n( "Various Artists" ) || artist.isEmpty() )
    {
         rs = query( QString(
            "SELECT distinct images.deviceid,images.path FROM images, artist, tags "
            "WHERE images.artist = artist.name "
            "AND artist.id = tags.artist "
            "AND tags.sampler = %1 "
            "AND images.album %2 "
            "AND images.deviceid IN (%3) " )
                 .arg( boolT() )
                 .arg( CollectionDB::likeCondition( album ) )
                 .arg( deviceIds ) );
    }
    else
    {
        rs = query( QString(
            "SELECT distinct images.deviceid,images.path FROM images WHERE artist %1 AND album %2 AND deviceid IN (%3) ORDER BY path;" )
                    .arg( CollectionDB::likeCondition( artist ) )
                    .arg( CollectionDB::likeCondition( album ) )
                    .arg( deviceIds ) );
    }
    QStringList values = URLsFromQuery( rs );
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
            QString path = cacheCoverDir().filePath( widthKey + key );
            if ( !QFile::exists( path ) )
            {
                QImage img( image );
                img.smoothScale( width, width, QImage::ScaleMin ).save( path, "PNG" );
            }
            return path;
        }
        else //large image
            return image;
    }
    return QString();
}


QString
CollectionDB::findEmbeddedImage( const QString& artist, const QString& album, uint width )
{
    // In the case of multiple embedded images, we arbitrarily choose one from the newest file
    // could potentially select multiple images within a file based on description, although a
    // lot of tagging software doesn't fill in that field, so we just get whatever the DB
    // happens to return for us
    QStringList rs;
    if ( artist == i18n("Various Artists") || artist.isEmpty() ) {
        // VAs need special handling to not match on artist name but instead check for sampler flag
        rs = query( QString(
                "SELECT embed.hash, embed.deviceid, embed.url FROM "
                "tags INNER JOIN embed ON tags.url = embed.url "
                     "INNER JOIN album ON tags.album = album.id "
                "WHERE "
                "album.name = '%1' "
                "AND tags.sampler = %2 "
                "ORDER BY modifydate DESC LIMIT 1;" )
                .arg( escapeString( album ) )
                .arg( boolT() ) );
    } else {
        rs = query( QString(
                "SELECT embed.hash, embed.deviceid, embed.url FROM "
                "tags INNER JOIN embed ON tags.url = embed.url "
                     "INNER JOIN artist ON tags.artist = artist.id "
                     "INNER JOIN album ON tags.album = album.id "
                "WHERE  "
                "artist.name = '%1' "
                "AND album.name = '%2' "
                "ORDER BY modifydate DESC LIMIT 1;" )
                .arg( escapeString( artist ) )
                .arg( escapeString( album ) ) );
    }

    QStringList values = QStringList();
    if ( rs.count() == 3 ) {
        values += rs.first();
        values += MountPointManager::instance()->getAbsolutePath( rs[1].toInt(), rs[2] );
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
    return QString();
}


QString
CollectionDB::findMetaBundleImage( const MetaBundle& trackInformation, uint width )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( trackInformation.url() );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, trackInformation.url().path() );
    QStringList values =
            query( QString(
            "SELECT embed.hash FROM tags LEFT JOIN embed ON tags.url = embed.url "
            " AND tags.deviceid = embed.deviceid WHERE tags.url = '%2' AND tags.deviceid = %1 ORDER BY hash DESC LIMIT 1;" )
            .arg( deviceid ).arg( escapeString( rpath ) ) );

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
    return QString();
}


QCString
CollectionDB::makeWidthKey( uint width )
{
    return QString::number( width ).local8Bit() + '@';
}


bool
CollectionDB::removeAlbumImage( const QString &artist, const QString &album )
{
    DEBUG_BLOCK

    QCString widthKey = "*@";
    QCString key = md5sum( artist, album );
    query( "DELETE FROM amazon WHERE filename='" + key + '\'' );

    // remove scaled versions of images (and add the asterisk for the shadow-caches)
    QStringList scaledList = cacheCoverDir().entryList( widthKey + key + '*' );
    if ( scaledList.count() > 0 )
        for ( uint i = 0; i < scaledList.count(); i++ )
            QFile::remove( cacheCoverDir().filePath( scaledList[ i ] ) );

    bool deleted = false;
    // remove large, original image
    if ( largeCoverDir().exists( key ) && QFile::remove( largeCoverDir().filePath( key ) ) )
        deleted = true;

    QString hardImage = findDirectoryImage( artist, album );
    debug() << "hardImage: " << hardImage << endl;

    if( !hardImage.isEmpty() )
    {
        int id = MountPointManager::instance()->getIdForUrl( hardImage );
        QString rpath = MountPointManager::instance()->getRelativePath( id, hardImage );
        query( "DELETE FROM images WHERE path='" + escapeString( hardImage ) + "' AND deviceid = " + QString::number( id ) + ';' );
        deleted = true;
    }

    if ( deleted )
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
    QString widthKey = QString::number( width ) + '@';
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

    qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optShowAll );
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    return qb.run();
}


QStringList
CollectionDB::composerList( bool withUnknowns, bool withCompilations )
{
    DEBUG_BLOCK
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabComposer, QueryBuilder::valName );

    if ( !withUnknowns )
        qb.excludeMatch( QueryBuilder::tabComposer, i18n( "Unknown" ) );
    if ( !withCompilations )
        qb.setOptions( QueryBuilder::optNoCompilations );

    qb.groupBy( QueryBuilder::tabComposer, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optShowAll );
    qb.sortBy( QueryBuilder::tabComposer, QueryBuilder::valName );
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

    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optShowAll );
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

    qb.groupBy( QueryBuilder::tabGenre, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optShowAll );
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

    qb.groupBy( QueryBuilder::tabYear, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optShowAll );
    qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
    return qb.run();
}

QStringList
CollectionDB::labelList()
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabLabels, QueryBuilder::valName );
    qb.groupBy( QueryBuilder::tabLabels, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optShowAll );
    qb.sortBy( QueryBuilder::tabLabels, QueryBuilder::valName );
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
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) + deviceidSelection() +
                      " ORDER BY lower( album.name );" );
    }
    // mysql is case insensitive and lower() is very slow
    else if (getDbConnectionType() == DbConnection::mysql)
    {
        return query( "SELECT DISTINCT album.name FROM tags, album, artist WHERE "
                      "tags.album = album.id AND tags.artist = artist.id "
                      "AND artist.name = '" + escapeString( artist ) + "' " +
                      ( withUnknown ? QString::null : "AND album.name <> '' " ) +
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) + deviceidSelection() +
                      " ORDER BY album.name;" );
    }
    else // sqlite
    {
        return query( "SELECT DISTINCT album.name FROM tags, album, artist WHERE "
                      "tags.album = album.id AND tags.artist = artist.id "
                      "AND lower(artist.name) = lower('" + escapeString( artist ) + "') " +
                      ( withUnknown ? QString::null : "AND album.name <> '' " ) +
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) + deviceidSelection() +
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
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) + deviceidSelection() +
                      " ORDER BY lower( album.name );" );
    }
    else
    {
        return query( "SELECT DISTINCT artist.name, album.name FROM tags, album, artist WHERE "
                      "tags.album = album.id AND tags.artist = artist.id " +
                      ( withUnknown ? QString::null : "AND album.name <> '' AND artist.name <> '' " ) +
                      ( withCompilations ? QString::null : "AND tags.sampler = " + boolF() ) + deviceidSelection() +
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

    command += '\'' + escapeString( pcb.url().url() )  + "',";
    command += ( title.isEmpty() ?       "NULL" : '\'' + escapeString( title ) + '\'' ) + ',';
    command += ( link.isEmpty() ?        "NULL" : '\'' + escapeString( link.url() ) + '\'' ) + ',';
    command += ( image.isEmpty() ?       "NULL" : '\'' + escapeString( image.url() ) + '\'' ) + ',';
    command += ( description.isEmpty() ? "NULL" : '\'' + escapeString( description ) + '\'' ) + ',';
    command += ( copyright.isEmpty() ?   "NULL" : '\'' + escapeString( copyright ) + '\'' ) + ',';
    command += QString::number( pcb.parentId() ) + ",'";
    command += escapeString( pcb.saveLocation() ) + "',";
    command += pcb.autoscan() ? boolT() + ',' : boolF() + ',';
    command += QString::number( pcb.fetchType() ) + ',';
    command += pcb.autotransfer() ? boolT() + ',' : boolF() + ',';
    command += pcb.hasPurge() ? boolT() + ',' : boolF() + ',';
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
        command += QString::number( idToUpdate ) + ',';

    command += '\'' + escapeString( episode.url().url() )   + "',";
    command += ( localurl.isEmpty()       ? "NULL" : '\'' + escapeString( localurl )       + '\'' ) + ',';
    command += '\'' + escapeString( episode.parent().url()) + "',";
    command += ( title.isEmpty()       ? "NULL" : '\'' + escapeString( title )       + '\'' ) + ',';
    command += ( subtitle.isEmpty()    ? "NULL" : '\'' + escapeString( subtitle )    + '\'' ) + ',';
    command += ( author.isEmpty()      ? "NULL" : '\'' + escapeString( author )      + '\'' ) + ',';
    command += ( description.isEmpty() ? "NULL" : '\'' + escapeString( description ) + '\'' ) + ',';
    command += ( type.isEmpty()        ? "NULL" : '\'' + escapeString( type )        + '\'' ) + ',';
    command += ( date.isEmpty()        ? "NULL" : '\'' + escapeString( date )        + '\'' ) + ',';
    command += ( guid.isEmpty()        ? "NULL" : '\'' + escapeString( guid )        + '\'' ) + ',';
    command += QString::number( duration ) + ',';
    command += QString::number( size ) + ',';
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
        pcb.setSaveLocation( *++it );
        pcb.setAutoScan    ( boolFromSql( *++it ) );
        pcb.setFetchType   ( (*++it).toInt() );
        pcb.setAutoTransfer( boolFromSql( *++it ) );
        pcb.setPurge       ( boolFromSql( *++it ) );
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
    command += ';';

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
        peb.setNew         ( boolFromSql( *++it ) );

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
        peb.setNew         ( boolFromSql( *++it ) );
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
        pcb->setSaveLocation( *++it );
        pcb->setAutoScan    ( boolFromSql( *++it ) );
        pcb->setFetchType   ( (*++it).toInt() );
        pcb->setAutoTransfer( boolFromSql( *++it ) );
        pcb->setPurge       ( boolFromSql( *++it ) );
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
		       << escapeString( b.saveLocation() )
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
                  << ( b.localUrl().isValid() ? escapeString( b.localUrl().url() ) : "NULL" )
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
                      "( url, dir, deviceid, createdate, modifydate, album, artist, composer, genre, year, title, "
                      "comment, track, discnumber, bpm, sampler, length, bitrate, "
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

    int deviceId = MountPointManager::instance()->getIdForUrl( bundle->url() );
    KURL relativePath;
    MountPointManager::instance()->getRelativePath( deviceId, bundle->url(), relativePath );
    //debug() << "File has deviceId " << deviceId << ", relative path " << relativePath.path() << ", absolute path " << bundle->url().path() << endl;

    command += escapeString( relativePath.path() ) + "','";
    command += escapeString( relativePath.directory() ) + "',";
    command += QString::number( deviceId ) + ',';
    command += QString::number( QFileInfo( bundle->url().path() ).created().toTime_t() ) + ',';
    command += QString::number( QFileInfo( bundle->url().path() ).lastModified().toTime_t() ) + ',';

    command += escapeString( QString::number( albumID( bundle->album(),   true, !incremental, true ) ) ) + ',';
    command += escapeString( QString::number( artistID( bundle->artist(), true, !incremental, true ) ) ) + ',';
    command += escapeString( QString::number( composerID( bundle->composer(), true, !incremental, true ) ) ) + ',';
    command += escapeString( QString::number( genreID( bundle->genre(),   true, !incremental, true ) ) ) + ",'";
    command += escapeString( QString::number( yearID( QString::number( bundle->year() ), true, !incremental, true ) ) ) + "','";

    command += escapeString( bundle->title() ) + "','";
    command += escapeString( bundle->comment() ) + "', ";
    command += escapeString( QString::number( bundle->track() ) ) + " , ";
    command += escapeString( QString::number( bundle->discNumber() ) ) + " , ";
    command += escapeString( QString::number( bundle->bpm() ) ) + " , ";
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
    command += ',';

    // NOTE any of these may be -1 or -2, this is what we want
    //      see MetaBundle::Undetermined
    command += QString::number( bundle->length() ) + ',';
    command += QString::number( bundle->bitrate() ) + ',';
    command += QString::number( bundle->sampleRate() ) + ',';
    command += QString::number( bundle->filesize() ) + ',';
    command += QString::number( bundle->fileType() ) + ')';

    //FIXME: currently there's no way to check if an INSERT query failed or not - always return true atm.
    // Now it might be possible as insert returns the rowid.
    insert( command, NULL );

    doAFTStuff( bundle, true );

    return true;
}

void
CollectionDB::doAFTStuff( MetaBundle* bundle, const bool tempTables )
{
    if( bundle->uniqueId().isEmpty() || bundle->url().path().isEmpty() )
        return;

    MountPointManager *mpm = MountPointManager::instance();
    //const to make sure one isn't later modified without the other being changed
    const int deviceIdInt = mpm->getIdForUrl( bundle->url().path() );
    const QString currdeviceid = QString::number( deviceIdInt );
    QString currid = escapeString( bundle->uniqueId() );
    QString currurl = escapeString( mpm->getRelativePath( deviceIdInt, bundle->url().path() ) );
    QString currdir = escapeString( mpm->getRelativePath( deviceIdInt, bundle->url().directory() ) );
    //debug() << "Checking currid = " << currid << ", currdir = " << currdir << ", currurl = " << currurl << endl;
    //debug() << "tempTables = " << (tempTables?"true":"false") << endl;


    QStringList urls = query( QString(
            "SELECT url, uniqueid "
            "FROM uniqueid%1 "
            "WHERE deviceid = %2 AND url = '%3';" )
                .arg( tempTables ? "_temp" : ""
                 , currdeviceid
                 , currurl ) );

    QStringList uniqueids = query( QString(
            "SELECT url, uniqueid, deviceid "
            "FROM uniqueid%1 "
            "WHERE uniqueid = '%2';" )
                .arg( tempTables ? "_temp" : ""
                , currid ) );

    QStringList nonTempIDs = query( QString(
            "SELECT url, uniqueid, deviceid "
            "FROM uniqueid "
            "WHERE uniqueid = '%1';" )
                .arg( currid ) );

    QStringList nonTempURLs = query( QString(
            "SELECT url, uniqueid "
            "FROM uniqueid "
            "WHERE deviceid = %1 AND url = '%2';" )
                .arg( currdeviceid
                , currurl ) );

    bool tempTablesAndInPermanent = false;
    bool permanentFullMatch = false;

    //if we're not using temp tables here, i.e. tempTables is false,
    //then the results from both sets of queries above should be equal,
    //so behavior should be the same
    if( tempTables && ( nonTempURLs.count() > 0 || nonTempIDs.count() > 0 ) )
            tempTablesAndInPermanent = true;
    if( tempTablesAndInPermanent && nonTempURLs.count() > 0 && nonTempIDs.count() > 0 )
            permanentFullMatch = true;

    //debug() << "tempTablesAndInPermanent = " << (tempTablesAndInPermanent?"true":"false") << endl;
    //debug() << "permanentFullMatch = " << (permanentFullMatch?"true":"false") << endl;

    //debug() << "Entering checks" << endl;
    //first case: not in permanent table or temporary table
    if( !tempTablesAndInPermanent && urls.empty() && uniqueids.empty() )
    {
        //debug() << "first case" << endl;
        QString insertline = QStringx( "INSERT INTO uniqueid%1 (deviceid, url, uniqueid, dir) "
                                      "VALUES ( %2,'%3', '%4', '%5');" )
                .args( QStringList()
                 << ( tempTables ? "_temp" : "" )
                 << currdeviceid
                 << currurl
                 << currid
                 << currdir );
        insert( insertline, NULL );
        //debug() << "aftCheckPermanentTables #1" << endl;
        aftCheckPermanentTables( currdeviceid, currid, currurl );
        return;
    }

    //next case: not in permanent table, but a match on one or the other in the temporary table
    //OR, we are using permanent tables (and not considering temp ones)
    if( !tempTablesAndInPermanent )
    {
        if( urls.empty() ) //uniqueid already found in temporary table but not url; check the old URL
        {
            //stat the original URL
            QString absPath = mpm->getAbsolutePath( uniqueids[2].toInt(), uniqueids[0] );
            //debug() << "At doAFTStuff, stat-ing file " << absPath << endl;
            bool statSuccessful = false;
            bool pathsSame = absPath == bundle->url().path();
            if( !pathsSame )
                statSuccessful = QFile::exists( absPath );
            if( statSuccessful ) //if true, new one is a copy
                warning() << "Already-scanned file at " << absPath << " has same UID as new file at " << bundle->url().path() << endl;
            else  //it's a move, not a copy, or a copy and then both files were moved...can't detect that
            {
                //debug() << "stat was NOT successful, updating tables with: " << endl;
                //debug() << QString( "UPDATE uniqueid%1 SET url='%2', dir='%3' WHERE uniqueid='%4';" ).arg( ( tempTables ? "_temp" : "" ), currurl, currdir, currid ) << endl;
                query( QStringx( "UPDATE uniqueid%1 SET deviceid = %2, url='%3', dir='%4' WHERE uniqueid='%5';" )
                      .args( QStringList()
                      << ( tempTables ? "_temp" : "" )
                      << currdeviceid
                      << currurl
                      << currdir
                      << currid ) );
                if( !pathsSame )
                    emit fileMoved( absPath, bundle->url().path(), bundle->uniqueId() );
            }
        }
        //okay then, url already found in temporary table but different uniqueid
        //a file exists in the same place as before, but new uniqueid...assume
        //that this is desired user behavior
        //NOTE: this should never happen during an incremental scan with temporary tables...!
        else if( uniqueids.empty() )
        {
            //debug() << "file exists in same place as before, new uniqueid" << endl;
            query( QString( "UPDATE uniqueid%1 SET uniqueid='%2' WHERE deviceid = %3 AND url='%4';" )
                    .arg( tempTables ? "_temp" : ""
                   , currid
                   , currdeviceid
                   , currurl ) );
            emit uniqueIdChanged( bundle->url().path(), urls[1], bundle->uniqueId() );
        }
        //else uniqueid and url match; nothing happened, so safely exit
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
            QString insertline = QString( "INSERT INTO uniqueid_temp (deviceid, url, uniqueid, dir) "
                                          "VALUES ( %1, '%2'" )
                                            .arg( currdeviceid
                                            , currurl );
            insertline += QString( ", '%1', '%2');" ).arg( currid ).arg( currdir );
            //debug() << "running command: " << insertline << endl;
            insert( insertline, NULL );
            //debug() << "aftCheckPermanentTables #2" << endl;
            aftCheckPermanentTables( currdeviceid, currid, currurl );
            return;
        }

        //second case...full match exists in permanent table, but path is different
        if( nonTempURLs.empty() )
        {
            //stat the original URL
            QString absPath = mpm->getAbsolutePath( nonTempIDs[2].toInt(), nonTempIDs[0] );
            //debug() << "At doAFTStuff part 2, stat-ing file " << absPath << endl;
            bool statSuccessful = false;
            bool pathsSame = absPath == bundle->url().path();
            if( !pathsSame )
                statSuccessful = QFile::exists( absPath );
            if( statSuccessful ) //if true, new one is a copy
                warning() << "Already-scanned file at " << absPath << " has same UID as new file at " << currurl << endl;
            else  //it's a move, not a copy, or a copy and then both files were moved...can't detect that
            {
                //debug() << "stat part 2 was NOT successful, updating tables with: " << endl;
                query( QString( "DELETE FROM uniqueid WHERE uniqueid='%1';" )
                      .arg( currid ) );
                query( QString( "INSERT INTO uniqueid_temp (deviceid, url, uniqueid, dir) "
                                "VALUES ( %1, '%2', '%3', '%4')" )
                                .arg( currdeviceid
                                , currurl
                                , currid
                                , currdir ) );
                if( !pathsSame )
                    emit fileMoved( absPath, bundle->url().path(), bundle->uniqueId() );
            }
        }
        else if( nonTempIDs.empty() )
        {
            //debug() << "file exists in same place as before, part 2, new uniqueid" << endl;
            query( QString( "DELETE FROM uniqueid WHERE deviceid = %1 AND url='%2';" )
                      .arg( currdeviceid )
                      .arg( currurl ) );
            query( QString( "INSERT INTO uniqueid_temp (deviceid, url, uniqueid, dir) VALUES ( %1, '%2', '%3', '%4')" )
                .arg( currdeviceid
                , currurl
                , currid
                , currdir ) );
            emit uniqueIdChanged( bundle->url().path(), nonTempURLs[1], bundle->uniqueId() );
        }
        //else do nothing...really this case should never happen
        return;
    }
}

void
CollectionDB::emitFileDeleted( const QString &absPath, const QString &uniqueid )
{
  if( uniqueid.isEmpty() )
      emit fileDeleted( absPath );
  else
      emit fileDeleted( absPath, uniqueid );
}

void
CollectionDB::emitFileAdded( const QString &absPath, const QString &uniqueid )
{
    if( uniqueid.isEmpty() )
        emit fileAdded( absPath );
    else
        emit fileAdded( absPath, uniqueid );
}

QString
CollectionDB::urlFromUniqueId( const QString &id )
{
    bool scanning = ( ScanController::instance() && ScanController::instance()->tablesCreated() );
    QStringList urls = query( QString(
            "SELECT deviceid, url "
            "FROM uniqueid%1 "
            "WHERE uniqueid = '%2';" )
                .arg( scanning ? "_temp" : QString::null )
                .arg( id ), true );

    if( urls.empty() && scanning )
        urls = query( QString(
                    "SELECT deviceid, url "
                    "FROM uniqueid "
                    "WHERE uniqueid = '%1';" )
                        .arg( id ) );

    if( urls.empty() )
        return QString();

    return MountPointManager::instance()->getAbsolutePath( urls[0].toInt(), urls[1] );
}

QString
CollectionDB::uniqueIdFromUrl( const KURL &url )
{
    MountPointManager *mpm = MountPointManager::instance();
    int currdeviceid = mpm->getIdForUrl( url.path() );
    QString currurl = escapeString( mpm->getRelativePath( currdeviceid, url.path() ) );

    bool scanning = ( ScanController::instance() && ScanController::instance()->tablesCreated() );
    QStringList uid = query( QString(
            "SELECT uniqueid "
            "FROM uniqueid%1 "
            "WHERE deviceid = %2 AND url = '%3';" )
                .arg( scanning ? "_temp" : QString::null )
                .arg( currdeviceid )
                .arg( currurl ), true );

    if( uid.empty() && scanning )
        uid = query( QString(
                "SELECT uniqueid "
                "FROM uniqueid "
                "WHERE deviceid = %1 AND url = '%2';" )
                    .arg( currdeviceid )
                    .arg( currurl ) );

    if( uid.empty() )
        return QString();

    return uid[0];
}

QString
CollectionDB::getURL( const MetaBundle &bundle )
{
    uint artID = artistID( bundle.artist(), false );
    if( !artID )
        return QString();

    uint albID = albumID( bundle.album(), false );
    if( !albID )
        return QString();

    QString q = QString( "SELECT tags.deviceid, tags.url "
            "FROM tags "
            "WHERE tags.album = '%1' AND tags.artist = '%2' AND tags.track = '%3' AND tags.title = '%4'" +
            deviceidSelection() + ';' )
        .arg( albID )
        .arg( artID )
        .arg( bundle.track() )
        .arg( escapeString( bundle.title() ) );

    QStringList urls = URLsFromQuery( query( q ) );

    if( urls.empty() )
        return QString();

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

// Helper function to convert the "tags.sampler" column to a MetaBundle::Collection value
//
// We use the first char of boolT / boolF as not all DBs store true/false as
// numerics (and it's only a single-char column)
static int
samplerToCompilation( const QString &it )
{
    if( it == CollectionDB::instance()->boolT().mid( 0, 1 ) )
    {
         return MetaBundle::CompilationYes;
    }
    else if( it == CollectionDB::instance()->boolF().mid( 0, 1 ) )
    {
        return MetaBundle::CompilationNo;
    }
    return MetaBundle::CompilationUnknown;
}

MetaBundle
CollectionDB::bundleFromQuery( QStringList::const_iterator *iter )
{
    QStringList::const_iterator &it = *iter;
    MetaBundle b;
    //QueryBuilder automatically inserts the deviceid as return value if asked for the path
    QString rpath = *it;
    int deviceid = (*++it).toInt();
    b.setPath      ( MountPointManager::instance()->getAbsolutePath( deviceid, rpath ) );
    b.setAlbum     (  *++it );
    b.setArtist    (  *++it );
    b.setComposer  (  *++it );
    b.setGenre     (  *++it );
    b.setTitle     (  *++it );
    b.setYear      ( (*++it).toInt() );
    b.setComment   (  *++it );
    b.setTrack     ( (*++it).toInt() );
    b.setBitrate   ( (*++it).toInt() );
    b.setDiscNumber( (*++it).toInt() );
    b.setLength    ( (*++it).toInt() );
    b.setSampleRate( (*++it).toInt() );
    b.setFilesize  ( (*++it).toInt() );

    b.setCompilation( samplerToCompilation( *it ) );
    ++it;
    b.setFileType( (*++it).toInt() );
    b.setBpm       ( (*++it).toFloat() );

    b.setScore     ( (*++it).toFloat() );
    b.setRating    ( (*++it).toInt() );
    b.setPlayCount ( (*++it).toInt() );
    b.setLastPlay  ( (*++it).toInt() );

    if( false && b.length() <= 0 ) {
        // we try to read the tags, despite the slow-down
        debug() << "Audioproperties not known for: " << b.url().fileName() << endl;
        b.readTags( TagLib::AudioProperties::Fast);
    }

    return b;
}

static void
fillInBundle( QStringList values, MetaBundle &bundle )
{
    //TODO use this whenever possible

    // crash prevention
    while( values.count() < 16 )
        values += "IF YOU CAN SEE THIS THERE IS A BUG!";

    QStringList::ConstIterator it = values.begin();

    bundle.setAlbum     ( *it ); ++it;
    bundle.setArtist    ( *it ); ++it;
    bundle.setComposer  ( *it ); ++it;
    bundle.setGenre     ( *it ); ++it;
    bundle.setTitle     ( *it ); ++it;
    bundle.setYear      ( (*it).toInt() ); ++it;
    bundle.setComment   ( *it ); ++it;
    bundle.setDiscNumber( (*it).toInt() ); ++it;
    bundle.setTrack     ( (*it).toInt() ); ++it;
    bundle.setBitrate   ( (*it).toInt() ); ++it;
    bundle.setLength    ( (*it).toInt() ); ++it;
    bundle.setSampleRate( (*it).toInt() ); ++it;
    bundle.setFilesize  ( (*it).toInt() ); ++it;
    bundle.setFileType  ( (*it).toInt() ); ++it;
    bundle.setBpm       ( (*it).toFloat() ); ++it;

    bundle.setCompilation( samplerToCompilation( *it ) );
    ++it;

    bundle.setUniqueId(*it);
}

bool
CollectionDB::bundleForUrl( MetaBundle* bundle )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( bundle->url() );
    KURL rpath;
    MountPointManager::instance()->getRelativePath( deviceid, bundle->url(), rpath );
    QStringList values = query( QString(
            "SELECT album.name, artist.name, composer.name, genre.name, tags.title, "
            "year.name, tags.comment, tags.discnumber, "
            "tags.track, tags.bitrate, tags.length, tags.samplerate, "
            "tags.filesize, tags.filetype, tags.bpm, tags.sampler, uniqueid.uniqueid "
            "FROM tags LEFT OUTER JOIN uniqueid ON tags.url = uniqueid.url AND tags.deviceid = uniqueid.deviceid,"
            "album, artist, composer, genre, year "
            "WHERE album.id = tags.album AND artist.id = tags.artist AND composer.id = tags.composer AND "
            "genre.id = tags.genre AND year.id = tags.year AND tags.url = '%2' AND tags.deviceid = %1;" )
                .arg( deviceid )
                .arg( escapeString( rpath.path( ) ) ) );

    bool valid = false;

    if ( !values.empty() )
    {
        fillInBundle( values, *bundle );
        valid = true;
    }
    else if( MediaBrowser::instance() && MediaBrowser::instance()->getBundle( bundle->url(), bundle ) )
    {
        valid = true;
    }
    else
    {
        // check if it's a podcast
        PodcastEpisodeBundle peb;
        if( getPodcastEpisodeBundle( bundle->url(), &peb ) )
        {
            if( bundle->url().protocol() == "file" && QFile::exists( bundle->url().path() ) )
            {
                MetaBundle mb( bundle->url(), true /* avoid infinite recursion */ );
                *bundle = mb;
            }
            bundle->copyFrom( peb );
            valid = true;
        }
    }

    return valid;
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
            qb.addReturnValue( QueryBuilder::tabComposer, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBitrate );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valSamplerate );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFilesize );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFileType );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBPM );
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
                b.setComposer  (  *++it );
                b.setGenre     (  *++it );
                b.setTitle     (  *++it );
                b.setYear      ( (*++it).toInt() );
                b.setComment   (  *++it );
                b.setTrack     ( (*++it).toInt() );
                b.setBitrate   ( (*++it).toInt() );
                b.setDiscNumber( (*++it).toInt() );
                b.setLength    ( (*++it).toInt() );
                b.setSampleRate( (*++it).toInt() );
                b.setFilesize  ( (*++it).toInt() );
                b.setFileType  ( (*++it).toInt() );
                b.setBpm       ( (*++it).toFloat() );
                b.setPath      (  *++it );

                b.setCompilation( samplerToCompilation( *it ) );
                ++it;

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

                    if( !MediaBrowser::instance()->getBundle( url, &b ) )
                    {
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

                        // check if it's a podcast
                        PodcastEpisodeBundle peb;
                        if( getPodcastEpisodeBundle( url, &peb ) )
                        {
                            b.copyFrom( peb );
                        }
                        else if( b.url().protocol() == "audiocd" || b.url().protocol() == "cdda" )
                        {
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
    int deviceid = MountPointManager::instance()->getIdForUrl( bundle.url() );
    KURL rpath;
    MountPointManager::instance()->getRelativePath( deviceid, bundle.url(), rpath );
    query( QString( "UPDATE tags SET bitrate='%1', length='%2', samplerate='%3' WHERE url='%5' AND deviceid = %4;" )
                    .arg( bundle.bitrate() )
                    .arg( bundle.length() )
                    .arg( bundle.sampleRate() )
                    .arg( deviceid )
                    .arg( escapeString( rpath.path() ) ) );
}


void
CollectionDB::addSongPercentage( const QString &url, float percentage,
        const QString &reason, const QDateTime *playtime )
{
    //the URL must always be inserted last! an escaped URL can contain Strings like %1->bug
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
    //statistics table might not have those values, but we need them later, so keep them
    int statDevId = deviceid;
    QString statRPath = rpath;
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, percentage, rating FROM statistics "
            "WHERE url = '%2' AND deviceid = %1;" )
            .arg( statDevId ).arg( escapeString( statRPath ) ) );

    //handle corner case: deviceid!=-1 but there is a statistics row for that song with deviceid -1
    if ( values.isEmpty() )
    {
        QString rpath2 = '.' + url;
        values = query( QString(
            "SELECT playcounter, createdate, percentage, rating FROM statistics "
            "WHERE url = '%1' AND deviceid = -1;" )
            .arg( escapeString( rpath2 ) ) );
        if ( !values.isEmpty() )
        {
            statRPath = rpath2;
            statDevId = -1;
        }
    }

    uint atime = playtime ? playtime->toTime_t() : QDateTime::currentDateTime().toTime_t();

    // check boundaries
    if ( percentage > 100.f ) percentage = 100.f;
    if ( percentage < 1.f )   percentage = 1.f;

    if ( !values.isEmpty() )
    {

        // increment playcounter and update accesstime
        query( QString( "UPDATE statistics SET playcounter=%1, accessdate=%2 WHERE url='%4' AND deviceid= %3;" )
                        .arg( values[0] + " + 1" )
                        .arg( atime )
                        .arg( statDevId )
                        .arg( escapeString( statRPath ) ) );
    }
    else
    {
        insert( QString( "INSERT INTO statistics ( url, deviceid, createdate, accessdate, percentage, playcounter, rating, uniqueid, deleted ) "
                        "VALUES ( '%6', %5, %1, %2, 0, 1, 0, %3, %4 );" )
                        .arg( atime )
                        .arg( atime )
                        .arg( ( getUniqueId( url ).isNull() ? "NULL" : '\'' + escapeString( getUniqueId( url ) ) + '\'' ) )
                        .arg( boolF() )
                        .arg( statDevId )
                        .arg( escapeString( statRPath ) ), 0 );
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
    const QStringList v = query( QString( "SELECT length FROM tags WHERE url = '%2' AND deviceid = %1;" )
                            .arg( deviceid ).arg( escapeString( rpath ) ) );
    const int length = v.isEmpty() ? 0 : v.first().toInt();

    ScriptManager::instance()->requestNewScore( url, prevscore, playcount, length, percentage, reason );
}


float
CollectionDB::getSongPercentage( const QString &url )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, url );

    QStringList values = qb.run();

    if( !values.isEmpty() )
        return values.first().toFloat();

    return 0;
}

int
CollectionDB::getSongRating( const QString &url )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
    qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, url );

    QStringList values = qb.run();

    if( values.count() )
        return kClamp( values.first().toInt(), 0, 10 );

    return 0;
}

void
CollectionDB::setSongPercentage( const QString &url , float percentage)
{
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, accessdate, rating FROM statistics WHERE url = '%2' AND deviceid = %1;" )
            .arg( deviceid ).arg( escapeString( rpath ) ) );

    //handle corner case: deviceid!=-1 but there is a statistics row for that song with deviceid -1
    if ( values.isEmpty() )
    {
        QString rpath2 = '.' + url;
        values = query( QString(
            "SELECT playcounter, createdate, accessdate, rating FROM statistics "
            "WHERE url = '%1' AND deviceid = -1;" )
            .arg( escapeString( rpath2 ) ) );
        if ( !values.isEmpty() )
        {
            rpath = rpath2;
            deviceid = -1;
        }
    }

    // check boundaries
    if ( percentage > 100.f ) percentage = 100.f;
    if ( percentage < 0.f )   percentage = 0.f;

    if ( !values.isEmpty() )
    {
        query( QString( "UPDATE statistics SET percentage=%1 WHERE url='%3' AND deviceid = %2;" )
                        .arg( percentage )
                        .arg( deviceid ).arg( escapeString( rpath ) ) );
    }
    else
    {
        insert( QString( "INSERT INTO statistics ( url, deviceid, createdate, accessdate, percentage, playcounter, rating, uniqueid, deleted ) "
                         "VALUES ( '%7', %6, %2, %3, %1, 0, 0, %3, %4 );" )
                        .arg( percentage )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( 0 )
                        .arg( ( getUniqueId( url ).isNull() ? "NULL" : '\'' + escapeString( getUniqueId( url ) ) + '\'' ) )
                        .arg( boolF() )
                        .arg( deviceid )
                        .arg( escapeString( rpath ) ),0 );
    }

    emit scoreChanged( url, percentage );
}

void
CollectionDB::setSongRating( const QString &url, int rating, bool toggleHalf )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
    QStringList values =
        query( QString(
            "SELECT playcounter, createdate, accessdate, percentage, rating FROM statistics WHERE url = '%2' AND deviceid = %1;" )
            .arg( deviceid )
            .arg( escapeString( rpath ) ) );

    //handle corner case: deviceid!=-1 but there is a statistics row for that song with deviceid -1
    if( values.isEmpty() )
    {
        QString rpath2 = '.' + url;
        values = query( QString(
            "SELECT playcounter, createdate, accessdate, percentage, rating FROM statistics "
            "WHERE url = '%1' AND deviceid = -1;" )
            .arg( escapeString( rpath2 ) ) );
        if ( !values.isEmpty() )
        {
            rpath = rpath2;
            deviceid = -1;
        }
    }

    bool ok = true;
    if( !values.isEmpty() )
    {
        int prev = values[4].toInt( &ok );
        if( ok && toggleHalf && ( prev == rating || ( prev == 1 && rating == 2 ) ) )
        {
            if( prev == 1 && rating == 2 )
                rating = 0;
            else if( rating % 2 ) //.5
                rating++;
            else
                rating--;
        }
    }

    // check boundaries
    if ( rating > 10 ) rating = 10;
    if ( rating < 0 /*|| rating == 1*/ ) rating = 0; //ratings are 1-5

    if ( !values.isEmpty() )
    {
        query( QString( "UPDATE statistics SET rating=%1 WHERE url='%3' AND deviceid = %2;" )
                        .arg( rating )
                        .arg( deviceid )
                        .arg( escapeString( rpath ) ) );
    }
    else
    {
        insert( QString( "INSERT INTO statistics ( url, deviceid, createdate, accessdate, percentage, rating, playcounter, uniqueid, deleted ) "
                         "VALUES ( '%7', %6, %2, %3, 0, %1, 0, %4, %5 );" )
                        .arg( rating )
                        .arg( QDateTime::currentDateTime().toTime_t() )
                        .arg( 0 )
                        .arg( ( getUniqueId( url ).isNull() ? "NULL" : '\'' + escapeString( getUniqueId( url ) ) + '\'' ) )
                        .arg( boolF() )
                        .arg( deviceid )
                        .arg( escapeString( rpath ) ), NULL );
    }

    emit ratingChanged( url, rating );
}

int
CollectionDB::getPlayCount( const QString &url )
{
    //queryBuilder is good
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, url );
    QStringList values = qb.run();
    if( values.count() )
        return values.first().toInt();
    return 0;
}

QDateTime
CollectionDB::getFirstPlay( const QString &url )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valCreateDate );
    qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, url );
    QStringList values = qb.run();
    QDateTime dt;
    if( values.count() )
        dt.setTime_t( values.first().toUInt() );
    return dt;
}

QDateTime
CollectionDB::getLastPlay( const QString &url )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valAccessDate );
    qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, url );
    QStringList values = qb.run();
    QDateTime dt;
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
    int oldMediaid = MountPointManager::instance()->getIdForUrl( oldURL );
    QString oldRpath = MountPointManager::instance()->getRelativePath( oldMediaid, oldURL );

    int newMediaid = MountPointManager::instance()->getIdForUrl( newURL );
    QString newRpath = MountPointManager::instance()->getRelativePath( newMediaid, newURL );

    //  Ensure destination is clear.
    query( QString( "DELETE FROM tags WHERE url = '%2' AND deviceid = %1;" )
        .arg( newMediaid ).arg( escapeString( newRpath ) ) );

    query( QString( "DELETE FROM statistics WHERE url = '%2' AND deviceid = %1;" )
        .arg( newMediaid ).arg( escapeString( newRpath ) ) );

    query( QString( "DELETE FROM tags_labels WHERE url = '%2' and deviceid = %1;" )
        .arg( newMediaid).arg( escapeString( newRpath ) ) );

    if ( !getLyrics( oldURL ).isEmpty() )
        query( QString( "DELETE FROM lyrics WHERE url = '%2' AND deviceid = %1;" )
            .arg( newMediaid ).arg( escapeString( newRpath ) ) );
    //  Migrate
    //code looks ugly but prevents problems when the URL contains HTTP escaped characters
    query( QString( "UPDATE tags SET url = '%3', deviceid = %1" )
        .arg( newMediaid ).arg( escapeString( newRpath ) )
        + QString( " WHERE deviceid=%1 AND url = '%2';" )
        .arg( oldMediaid ).arg( escapeString( oldRpath ) ) );

    query( QString( "UPDATE statistics SET url = '%2', deviceid = %1" )
        .arg( newMediaid ).arg( escapeString( newRpath ) )
        + QString( " WHERE deviceid=%1 AND url = '%2';" )
        .arg( oldMediaid ).arg( escapeString( oldRpath ) ) );

    query( QString( "UPDATE lyrics SET url = '%2', deviceid = %1" )
        .arg( newMediaid ).arg( escapeString( newRpath ) )
        + QString( " WHERE deviceid=%1 AND url = '%2';" )
        .arg( oldMediaid ).arg( escapeString( oldRpath ) ) );

    query( QString( "UPDATE tags_labels SET url = '%2', deviceid = %1 WHERE deviceid = %3 AND url = '%4';" )
            .arg( QString::number( newMediaid ), escapeString( newRpath ), QString::number( oldMediaid ), escapeString( oldRpath ) ) );

    query( QString( "UPDATE uniqueid SET url = '%1', deviceid = %2 WHERE url = '%3' AND deviceid = %4;" )
        .arg( escapeString( newRpath ), QString::number( newMediaid ),
              escapeString( oldRpath ), QString::number( oldMediaid ) ) );

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

void CollectionDB::cancelMovingFileJob()
{
    m_moveFileJobCancelled = true;
}

bool
CollectionDB::organizeFile( const KURL &src, const OrganizeCollectionDialog &dialog, bool copy )
{
   if( !MetaBundle::isKioUrl( src ) )
       return false;

   bool overwrite = dialog.overwriteCheck->isChecked();
   bool localFile = src.isLocalFile();
   KURL tmpSrc = src;
   if( !localFile )
   {
      QString tmp;
      QString extension = src.url().section( '.', -1 );
      extension = extension.section("?", 0, 0);  // remove trailling stuff lead by ?, if any

      int count = 0;
      do
      {
         tmp = QString( dialog.folderCombo->currentText() + "/amarok-tmp-%1." + extension ).arg( count );
         count++;
      } while( QFile::exists( tmp ) );
      tmpSrc = KURL::fromPathOrURL( tmp );

      KIO::FileCopyJob *job = 0;
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
        if( m_moveFileJobCancelled )
        {
            disconnect( job, SIGNAL(result( KIO::Job * )), this, SLOT(fileOperationResult( KIO::Job * )) );

            QString partFile = QString( "%1.part" ).arg( (job->destURL()).path() );
            job->kill();
            QFile file( partFile );
            if( file.exists() ) file.remove();

            m_waitForFileOperation = false;
            m_fileOperationFailed = true;
            continue;
        }

         usleep( 10000 );
         kapp->processEvents( 100 );
      }

      if( m_fileOperationFailed )
      {
         debug() << "failed to transfer " << src.url() << " to " << tmpSrc << endl;

         m_moveFileJobCancelled = false;
         return false;
      }
   }

   //Building destination here.
   MetaBundle mb( tmpSrc );
   QString dest = dialog.buildDestination( dialog.buildFormatString(), mb );

   debug() << "Destination: " << dest << endl;

   if( !m_moveFileJobCancelled && tmpSrc.path() != dest ) //suppress error warning that file couldn't be moved
   {
      if( !CollectionDB::instance()->moveFile( tmpSrc.url(), dest, overwrite, copy && localFile ) )
      {
         if( !localFile )
            QFile::remove( tmpSrc.path() );

         m_moveFileJobCancelled = false;
         return false;
      }
   }

   //Use cover image for folder icon
   if( !m_moveFileJobCancelled && dialog.coverCheck->isChecked() && !mb.artist().isEmpty() && !mb.album().isEmpty() )
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
            config.writeEntry( "Icon", cover );
            config.sync();
         }
         //}         //Not amazon nice.
      }
   }

   if( localFile && isDirInCollection( src.directory() ) && QDir().rmdir( src.directory() ) )
   {
      debug() << "removed: " << src.directory() << endl;
   }

   m_moveFileJobCancelled = false;

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

    m_fileOperationFailed = false;
    KIO::FileCopyJob *job = 0;
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
        if( m_moveFileJobCancelled )
        {
            disconnect( job, SIGNAL(result( KIO::Job * )), this, SLOT(fileOperationResult( KIO::Job * )) );

            QString partFile = QString( "%1.part" ).arg( (job->destURL()).path() );
            job->kill();
            QFile file( partFile );
            if( file.exists() ) file.remove();

            m_waitForFileOperation = false;
            m_fileOperationFailed = true;
            continue;
        }

        usleep( 10000 );
        kapp->processEvents( 100 );
    }

    if( !m_fileOperationFailed )
    {
        if( copy )
        {
            MetaBundle bundle( dstURL );
            if( bundle.isValidMedia() )
            {
                addSong( &bundle, true );
                return true;
            }
        }
        else
        {
            emit fileMoved( src, dest );
            migrateFile( srcURL.path(), dstURL.path() );

            if( isFileInCollection( srcURL.path() ) )
            {
                return true;
            }
            else
            {
                MetaBundle bundle( dstURL );
                if( bundle.isValidMedia() )
                {
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

    int deviceid = MountPointManager::instance()->getIdForUrl( path );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, path );

    if (getDbConnectionType() == DbConnection::postgresql)
    {
        // REPLACE INTO is not valid SQL for postgres, so we need to check whether we
        // should UPDATE() or INSERT()
        QStringList values = query( QString("SELECT * FROM directories%1 WHERE dir='%3' AND deviceid=%2;")
            .arg( temporary ? "_temp" : "")
            .arg( deviceid )
            .arg( escapeString( rpath ) ) );

        if(values.count() > 0 )
        {
            query( QString( "UPDATE directories%1 SET changedate=%2 WHERE dir='%4'AND deviceid=%3;")
            .arg( temporary ? "_temp" : "" )
            .arg( datetime )
            .arg( deviceid )
            .arg( escapeString( rpath ) ) );
        }
        else
        {

            query( QString( "INSERT INTO directories%1 (dir, deviceid,changedate) VALUES ('%4', %3, '%2');")
            .arg( temporary ? "_temp" : "")
            .arg( datetime )
            .arg( deviceid )
            .arg( escapeString( rpath ) ) );
        }
    }
    else
    {
        query( QString( "REPLACE INTO directories%1 ( dir, deviceid, changedate ) VALUES ( '%4', %3, %2 );" )
                  .arg( temporary ? "_temp" : "" )
                  .arg( datetime )
                  .arg( deviceid )
                  .arg( escapeString( rpath ) ) );
    }

    INotify::instance()->watchDir( path );
}


void
CollectionDB::removeSongsInDir( QString path, QMap<QString,QString> *tagsRemoved )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );
    int deviceid = MountPointManager::instance()->getIdForUrl( path );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, path );

    // Pass back the list of tags we actually delete if requested.
    if( tagsRemoved )
    {
        QStringList result
          = query( QString( "SELECT tags.deviceid, tags.url, uniqueid.uniqueid FROM tags "
                            "LEFT JOIN uniqueid ON uniqueid.url = tags.url "
                            "AND uniqueid.deviceid = tags.deviceid "
                            "WHERE tags.dir = '%2' AND tags.deviceid = %1" )
                   .arg( deviceid )
                   .arg( escapeString( rpath ) ) );
        QStringList::ConstIterator it = result.begin(), end = result.end();
        while( it != end )
        {
            int deviceid2    = (*(it++)).toInt();
            QString rpath2   =  *(it++);
            QString uniqueid =  *(it++);
            (*tagsRemoved)[uniqueid] = MountPointManager::instance()->getAbsolutePath(
                                deviceid2, rpath2 );
        }
    }

    query( QString( "DELETE FROM tags WHERE dir = '%2' AND deviceid = %1;" )
              .arg( deviceid )
              .arg( escapeString( rpath ) ) );

    query( QString( "DELETE FROM uniqueid WHERE dir = '%2' AND deviceid = %1;" )
              .arg( deviceid )
              .arg( escapeString( rpath ) ) );
}


bool
CollectionDB::isDirInCollection( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );
    int deviceid = MountPointManager::instance()->getIdForUrl( path );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, path );

    QStringList values =
        query( QString( "SELECT changedate FROM directories WHERE dir = '%2' AND deviceid = %1;" )
                  .arg( deviceid )
                  .arg( escapeString( rpath ) ) );

    return !values.isEmpty();
}


bool
CollectionDB::isFileInCollection( const QString &url  )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );

    QString sql = QString( "SELECT url FROM tags WHERE url = '%2' AND deviceid = %1" )
                             .arg( deviceid )
                             .arg( escapeString( rpath ) );
    if ( deviceid == -1 )
    {
        sql += ';';
    }
    else
    {
        QString rpath2 = '.' + url;
        sql += QString( " OR url = '%1' AND deviceid = -1;" )
                      .arg( escapeString( rpath2 ) );
    }
    QStringList values = query( sql );

    return !values.isEmpty();
}


void
CollectionDB::removeSongs( const KURL::List& urls )
{
    for( KURL::List::ConstIterator it = urls.begin(), end = urls.end(); it != end; ++it )
    {
        int deviceid = MountPointManager::instance()->getIdForUrl( *it );
        QString rpath = MountPointManager::instance()->getRelativePath( deviceid, (*it).path() );

        query( QString( "DELETE FROM tags WHERE url = '%2' AND deviceid = %1;" )
            .arg( deviceid )
            .arg( escapeString( rpath ) ) );
        query( QString( "DELETE FROM uniqueid WHERE url = '%2' AND deviceid = %1;" )
                .arg( deviceid )
                .arg( escapeString( rpath ) ) );
        query( QString( "UPDATE statistics SET deleted = %1 WHERE url = '%3' AND deviceid = %2;" )
                .arg( boolT() )
                .arg( deviceid )
                .arg( escapeString( rpath ) ) );
    }
}


QStringList
CollectionDB::similarArtists( const QString &artist, uint count )
{
    QStringList values;

    values = query( QString( "SELECT suggestion FROM related_artists WHERE artist = '%1' ORDER BY %2 LIMIT %3 OFFSET 0;" )
                                 .arg( escapeString( artist ), randomFunc(), QString::number( count ) ) );

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

    int deviceid = MountPointManager::instance()->getIdForUrl( path );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, path );

    albums = query( QString( "SELECT DISTINCT album.name FROM tags_temp, album%1 AS album WHERE tags_temp.dir = '%3' AND tags_temp.deviceid = %2 AND album.id = tags_temp.album AND tags_temp.sampler IS NULL;" )
              .arg( temporary ? "_temp" : "" )
              .arg( deviceid )
              .arg( escapeString( rpath ) ) );

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
            debug() << "Detected compilation: " << albums[ i ] << " - " << artists.count() << ':' << dirs.count() << endl;
        }
        query( QString( "UPDATE tags_temp SET sampler = %1 WHERE album = '%2' AND sampler IS NULL;" )
                         .arg(artists.count() > dirs.count() ? boolT() : boolF()).arg( album_id ) );
    }
}

void
CollectionDB::setCompilation( const KURL::List &urls, bool enabled, bool updateView )
{
    for ( KURL::List::const_iterator it = urls.begin(); it != urls.end(); ++it )
    {
        QString url( ( *it ).path() );

        int deviceid = MountPointManager::instance()->getIdForUrl( url );
        QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );

        query( QString( "UPDATE tags SET sampler = %1 WHERE tags.url = '%2' AND tags.deviceid = %3;" )
                .arg( ( enabled ? boolT() : boolF() ), escapeString( rpath ), QString::number( deviceid ) ) );
    }

    // Update the Collection-Browser view,
    // using QTimer to make sure we don't manipulate the GUI from a thread
    if ( updateView )
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
}


void
CollectionDB::removeDirFromCollection( QString path )
{
    //if ( path.endsWith( "/" ) )
    //    path = path.left( path.length() - 1 );
    int deviceid = MountPointManager::instance()->getIdForUrl( path );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, path );

    query( QString( "DELETE FROM directories WHERE dir = '%2' AND deviceid = %1;" )
                    .arg( deviceid )
                    .arg( escapeString( rpath ) ) );
}


QString
CollectionDB::IDFromExactValue( QString table, QString value, bool autocreate, bool temporary /* = false */ )
{
    if ( temporary )
    {
            table.append( "_temp" );
    }

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

void
CollectionDB::deleteRedundantName( const QString &table, const QString &id )
{
    QString querystr( QString( "SELECT %1 FROM tags WHERE tags.%1 = %2 LIMIT 1;" ).arg( table, id ) );
    QStringList result = query( querystr );
    if ( result.isEmpty() )
        query( QString( "DELETE FROM %1 WHERE id = %2;" ).arg( table,id ) );
}

void
CollectionDB::deleteAllRedundant( const QString &table )
{
    //This works with MySQL4. I thought it might not do, due to the comment in copyTempTables
    query( QString( "DELETE FROM %1 WHERE id NOT IN ( SELECT %2 FROM tags )" ).arg( table, table ) );
}


void
CollectionDB::updateTags( const QString &url, const MetaBundle &bundle, const bool updateView )
{
    DEBUG_BLOCK
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabComposer, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFilesize );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFileType );
    // [10] is above. [11] is below.
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBPM );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valID );
    qb.addReturnValue( QueryBuilder::tabComposer, QueryBuilder::valID );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valID );
    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valID );

    qb.addURLFilters ( QStringList( url ) );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    QStringList values = qb.run();

    if ( values.count() > 17 )
    {
        error() << "Query returned more than 1 song. Aborting updating metadata" << endl;
        return;
    }

    if ( !values.isEmpty() )
    {
        bool art=false, comp=false, alb=false, gen=false, year=false;

        QString command = "UPDATE tags SET ";
        if ( values[ 0 ] != bundle.title() )
            command += "title = '" + escapeString( bundle.title() ) + "', ";
        if ( values[ 1 ] != bundle.artist() )
        {
            art = true;
            command += "artist = " + IDFromExactValue( "artist", bundle.artist() ) + ", ";
        }
        if ( values[ 2 ] != bundle.composer() )
        {
            comp = true;
            command += "composer = " + IDFromExactValue( "composer", bundle.composer() ) + ", ";
        }
        if ( values[ 3 ] != bundle.album() )
        {
            alb = true;
            command += "album = "  + IDFromExactValue( "album", bundle.album() ) + ", ";
        }
        if ( values[ 4 ] != bundle.genre() )
        {
            gen = true;
            command += "genre = "  + IDFromExactValue( "genre", bundle.genre() ) + ", ";
        }
        if ( values[ 5 ] != QString::number( bundle.year() ) )
        {
            year = false;
            command += "year = "   + IDFromExactValue( "year", QString::number( bundle.year() ) ) + ", ";
        }
        if ( values[ 6 ] != QString::number( bundle.track() ) )
            command += "track = " + QString::number( bundle.track() ) + ", ";
        if ( values[ 7 ] != bundle.comment() )
            command += "comment = '" + escapeString( bundle.comment() ) + "', ";
        if ( values[ 8 ] != QString::number( bundle.discNumber() ) )
            command += "discnumber = '" + QString::number( bundle.discNumber() ) + "', ";
        if ( values[ 9 ] != QString::number( bundle.filesize() ) )
            command += "filesize = '" + QString::number( bundle.filesize() ) + "', ";
        if ( values[ 10 ] != QString::number( bundle.fileType() ) )
            command += "filetype = '" + QString::number( bundle.fileType() ) + "', ";
        if ( values[ 11 ] != QString::number( bundle.bpm() ) )
            command += "bpm = '" + QString::number( bundle.bpm() ) + "', ";

        if ( "UPDATE tags SET " == command )
        {
            debug() << "No tags selected to be changed" << endl;
        }
        else
        {
            int deviceid = MountPointManager::instance()->getIdForUrl( url );
            QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
            //We have to remove the trailing comma from command
            query( command.left( command.length() - 2 ) + " WHERE url = '" + escapeString( rpath ) +
                    "' AND deviceid = " + QString::number( deviceid ) + ';' );
        }

        //Check to see if we use the entry anymore. If not, delete it
        if ( art )
            deleteRedundantName( "artist", values[ 12 ] );
        if ( comp )
            deleteRedundantName( "composer", values[ 13 ] );
        if ( alb )
            deleteRedundantName( "album", values[ 14 ] );
        if ( gen )
            deleteRedundantName( "genre", values[ 15 ] );
        if ( year )
            deleteRedundantName( "year", values[ 16 ] );

        // Update the Collection-Browser view,
        // using QTimer to make sure we don't manipulate the GUI from a thread
        if ( updateView )
            QTimer::singleShot( 0, CollectionView::instance(), SLOT( databaseChanged() ) );

        if( art || alb )
            emit tagsChanged( values[12], values[14] );
    }

    if ( EngineController::instance()->bundle().url() == bundle.url() )
    {
        debug() << "Current song edited, updating widgets: " << bundle.title() << endl;
        EngineController::instance()->currentTrackMetaDataChanged( bundle );
    }

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
    doAFTStuff( &bundle, false );
}

QString
CollectionDB::getUniqueId( const QString &url )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
    QStringList values = query( QString( "SELECT uniqueid FROM uniqueid WHERE deviceid = %1 AND url = '%2';" )
                                .arg( deviceid )
                                .arg( escapeString( rpath ) ));
    if( !values.empty() )
        return values[0];
    else
        return QString();
}

void
CollectionDB::setLyrics( const QString &url, const QString &lyrics, const QString &uniqueid )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );

    QStringList values = query(QString("SELECT lyrics FROM lyrics WHERE url = '%2' AND deviceid = %1;")
                    .arg( deviceid ).arg( escapeString( rpath ) ) );
    if(values.count() > 0)
    {
        if ( !lyrics.isEmpty() )
            query( QString( "UPDATE lyrics SET lyrics = '%1' WHERE url = '%3' AND deviceid = %2;" )
                    .arg( escapeString( lyrics), QString::number(deviceid), escapeString( rpath ) ) );
        else
            query( QString( "DELETE FROM lyrics WHERE url = '%2' AND deviceid = %1;" )
                    .arg( deviceid).arg( escapeString( rpath ) ) );
    }
    else
    {
        insert( QString( "INSERT INTO lyrics (deviceid, url, lyrics, uniqueid) values ( %1, '%2', '%3', '%4' );" )
                .arg( QString::number(deviceid), escapeString( rpath ), escapeString( lyrics ), escapeString( uniqueid ) ), NULL);
    }
}


QString
CollectionDB::getLyrics( const QString &url )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
    QStringList values = query( QString( "SELECT lyrics FROM lyrics WHERE url = '%2' AND deviceid = %1;" )
                .arg( deviceid ).arg( escapeString( rpath ) ) );
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
        else if ( AmarokConfig::mySqlPassword2() != config->password() )
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
    QThread *currThread = ThreadManager::Thread::getRunning();

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

    //below check is necessary because if stop after current track is selected,
    //the url's path will be empty, so check the previous URL for the path that
    //had just played
    const KURL url = EngineController::instance()->bundle().url().path().isEmpty() ?
                            EngineController::instance()->previousURL() :
                            EngineController::instance()->bundle().url();
    PodcastEpisodeBundle peb;
    if( getPodcastEpisodeBundle( url.url(), &peb ) )
    {
        PodcastEpisode *p = PlaylistBrowser::instance()->findPodcastEpisode( peb.url(), peb.parent() );
        if ( p )
            p->setListened();

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
CollectionDB::fetchCover( QWidget* parent, const QString& artist, const QString& album, bool noedit, QListViewItem* item ) //SLOT
{
    #ifdef AMAZON_SUPPORT
    debug() << "Fetching cover for " << artist << " - " << album << endl;

    const bool isCompilation = albumIsCompilation( QString::number( albumID( album, false, false, true ) ) );
    CoverFetcher* fetcher;
    if( isCompilation )
        // avoid putting various artists in front of album title. this causes problems for locales other than US.
        fetcher = new CoverFetcher( parent, "", album );
    else
        fetcher = new CoverFetcher( parent, artist, album );
    if( item )
    {
        itemCoverMapMutex->lock();
        itemCoverMap->insert( item, fetcher );
        itemCoverMapMutex->unlock();
    }
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
    QStringList folders = MountPointManager::instance()->collectionFolders();

    if ( folders.isEmpty() )
    {
        //dropTables( false );
        //createTables( false );
        clearTables( false );
        emit scanDone( true );
    }
    else if( PlaylistBrowser::instance() )
    {
        emit scanStarted();
        ThreadManager::instance()->queueJob( new ScanController( this, false, folders ) );
    }
}


void
CollectionDB::stopScan() //SLOT
{
    ThreadManager::instance()->abortAllJobsNamed( "CollectionScanner" );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionDB::dirDirty( const QString& path )
{
    debug() << k_funcinfo << "Dirty: " << path << endl;

    ThreadManager::instance()->queueJob( new ScanController( this, false, path ) );
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

    //check the validity of the CollectionItem as it may have been deleted e.g. by a
    //collection scan while fetching the cover
    itemCoverMapMutex->lock();
    QMap<QListViewItem*, CoverFetcher*>::Iterator it;
    for( it = itemCoverMap->begin(); it != itemCoverMap->end(); ++it )
    {
        if( it.data() == fetcher )
        {
            if( it.key()->isOpen() )
                static_cast<CollectionItem*>(it.key())->setPixmap( 0, QPixmap() );
            itemCoverMap->erase( it );
        }
    }
    itemCoverMapMutex->unlock();
}

/**
 * This query is fairly slow with sqlite, and often happens just
 * after the OSD is shown. Threading it restores responsivity.
 */
class SimilarArtistsInsertionJob : public ThreadManager::DependentJob
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
            : ThreadManager::DependentJob( parent, "SimilarArtistsInsertionJob" )
            , artist( QDeepCopy<QString>(s) )
            , escapedArtist( parent->escapeString( QDeepCopy<QString>(s) ) )
            , suggestions( QDeepCopy<QStringList>(list) )
    {}
};

void
CollectionDB::similarArtistsFetched( const QString& artist, const QStringList& suggestions )
{
    debug() << "Received similar artists\n";

    ThreadManager::instance()->queueJob( new SimilarArtistsInsertionJob( this, artist, suggestions ) );
}

void
CollectionDB::aftCheckPermanentTables( const QString &currdeviceid, const QString &currid, const QString &currurl )
{
    //DEBUG_BLOCK
    //debug() << "deviceid = " << currdeviceid << endl << "url = " << currurl << endl << "uid = " << currid << endl;

    QStringList check1, check2;

    foreach( m_aftEnabledPersistentTables )
    {
        //debug() << "Checking " << (*it) << endl;;
        check1 = query( QString(
                "SELECT url, deviceid "
                "FROM %1 "
                "WHERE uniqueid = '%2';" )
                    .arg( escapeString( *it ) )
                    .arg( currid ) );

        check2 = query( QString(
                "SELECT url, uniqueid "
                "FROM %1 "
                "WHERE deviceid = %2 AND url = '%3';" )
                    .arg( escapeString( *it ) )
                    .arg( currdeviceid
                    , currurl ) );

        if( !check1.empty() )
        {
            //debug() << "uniqueid found, updating url" << endl;
            query( QString( "UPDATE %1 SET deviceid = %2, url = '%4' WHERE uniqueid = '%3';" )
                                .arg( escapeString( *it ) )
                                .arg( currdeviceid
                                , currid
                                , currurl ) );
        }
        else if( !check2.empty() )
        {
            //debug() << "url found, updating uniqueid" << endl;
            query( QString( "UPDATE %1 SET uniqueid = '%2' WHERE deviceid = %3 AND url = '%4';" )
                                .arg( escapeString( *it ) )
                                .arg( currid
                                , currdeviceid
                                , currurl ) );
        }
    }
}

void
CollectionDB::aftMigratePermanentTablesUrl( const QString& /*oldUrl*/, const QString& newUrl, const QString& uniqueid )
{
    //DEBUG_BLOCK
    int deviceid = MountPointManager::instance()->getIdForUrl( newUrl );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, newUrl );
    //NOTE: if ever do anything with "deleted" in the statistics table, set deleted to false in query
    //below; will need special case.
    //debug() << "deviceid = " << deviceid << endl << "newurl = " << newUrl << endl << "uid = " << uniqueid << endl;
    foreach( m_aftEnabledPersistentTables )
    {
        query( QString( "DELETE FROM %1 WHERE deviceid = %2 AND url = '%3';" )
                                .arg( escapeString( *it ) )
                                .arg( deviceid )
                                .arg( escapeString( rpath ) ) );
        query( QString( "UPDATE %1 SET deviceid = %2, url = '%4' WHERE uniqueid = '%3';" )
                                .arg( escapeString( *it ) )
                                .arg( deviceid )
                                .arg( escapeString( uniqueid ) )
                                .arg( escapeString( rpath ) ) );
    }
}

void
CollectionDB::aftMigratePermanentTablesUniqueId( const QString& /*url*/, const QString& oldid, const QString& newid )
{
    //DEBUG_BLOCK
    //debug() << "oldid = " << oldid << endl << "newid = " << newid << endl;
    //NOTE: if ever do anything with "deleted" in the statistics table, set deleted to false in query
    //below; will need special case.
    foreach( m_aftEnabledPersistentTables )
    {
        query( QString( "DELETE FROM %1 WHERE uniqueid = '%2';" )
                                .arg( escapeString( *it ) )
                                .arg( escapeString( newid ) ) );
        query( QString( "UPDATE %1 SET uniqueid = '%1' WHERE uniqueid = '%2';" )
                                .arg( escapeString( *it ) )
                                .arg( escapeString( newid ) )
                                .arg( escapeString( oldid ) ) );
    }
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
        QString appVersion = Amarok::config( "General Options" )->readEntry( "Version" );
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
                passwd = Amarok::config( "MySql" )->readEntry( "MySqlPassword" ); //read the field as plaintext
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
        QString appVersion = Amarok::config( "General Options" )->readEntry( "Version" );
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
                passwd = Amarok::config( "Postgresql" )->readEntry( "PostgresqlPassword" );
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
        m_dbConfig = new SqliteConfig(
                        Amarok::config( "Sqlite" )->readPathEntry( "location",
                                        Amarok::saveLocation() + "collection.db" ) );
    }

    DbConnection *dbConn = getMyConnection();

    if ( !dbConn->isConnected() || !dbConn->isInitialized() )
    {
        error() << "Failed to connect to or initialise database!" << endl;
        Amarok::MessageQueue::instance()->addMessage( dbConn->lastError() );
    }
    else
    {
        if ( !isValid() )
        {
            //No tables seem to exist (as doing a count(url) didn't even return any number, even 0).
            warning() << "Tables seem to not exist." << endl;
            warning() << "Attempting to create tables (this should be safe; ignore any errors)..." << endl;
            createTables(false);
            createPersistentTables();
            createPodcastTables();
            createStatsTable();
            warning() << "Tables should now definitely exist. (Stop ignoring errors)" << endl;

            //Since we have created the tables, we need to make sure the version numbers are
            //set to the correct values. If this is not done now, the database update code may
            //run, which could corrupt things.
            Amarok::config( "Collection Browser" )->writeEntry( "Database Version", DATABASE_VERSION );
            Amarok::config( "Collection Browser" )->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );
            Amarok::config( "Collection Browser" )->writeEntry( "Database Persistent Tables Version", DATABASE_PERSISTENT_TABLES_VERSION );
            Amarok::config( "Collection Browser" )->writeEntry( "Database Podcast Tables Version", DATABASE_PODCAST_TABLES_VERSION );    Amarok::config( "Collection Browser" )->writeEntry( "Database AFT Version", DATABASE_AFT_VERSION );

            setAdminValue( "Database Version", QString::number( DATABASE_VERSION ) );
            setAdminValue( "Database Stats Version", QString::number( DATABASE_STATS_VERSION ) );
            setAdminValue( "Database Persistent Tables Version", QString::number( DATABASE_PERSISTENT_TABLES_VERSION ) );
            setAdminValue( "Database Podcast Tables Version", QString::number( DATABASE_PODCAST_TABLES_VERSION ) );
            setAdminValue( "Database AFT Version", QString::number( DATABASE_AFT_VERSION ) );
        }


        // Due to a bug in our sqllite handling code, we have to recreate the indices.
        // We should rmeove this before 1.4.5
        if ( m_dbConnType == DbConnection::sqlite ) {
            QStringList indices = query( "SELECT name FROM sqlite_master WHERE type='index' ORDER BY name;" );
            if (!indices.contains("url_tag")) {
                createIndices();
            }
        }


        //updates for the Devices table go here
        //put all other update code into checkDatabase()
        //make sure that there is no call to MountPointManager in CollectionDB's ctor
        //or in methods called from the ctor.
        if ( adminValue( "Database Devices Version" ).isEmpty()
             && Amarok::config( "CollectionBrowser" )->readNumEntry( "Database Devices Version", 0 ) == 0 )
        {
            createDevicesTable();
        }
        else if ( adminValue( "Database Devices Version" ).toInt() != DATABASE_DEVICES_VERSION
              || Amarok::config( "Collection Browser" )->readNumEntry( "Database Devices Version", 0 ) != DATABASE_DEVICES_VERSION )
        {
            int prev = adminValue( "Database Devices Version" ).toInt();

            if ( prev > DATABASE_DEVICES_VERSION || prev < 0 )
            {
                error() << "Database devices version too new for this version of Amarok" << endl;
                exit( 1 );
                //dropDevicesTable();
            }
            else
            {
                debug() << "Updating DEVICES table" << endl;
                //add future Devices update code here
            }
        }
        Amarok::config( "Collection Browser" )->writeEntry( "Database Devices Version", DATABASE_DEVICES_VERSION );
        setAdminValue( "Database Devices Version", QString::number( DATABASE_DEVICES_VERSION ) );

        //make sure that all indices exist
        createIndices();
        createPermanentIndices();
    }

}

void
CollectionDB::checkDatabase()
{
    DEBUG_BLOCK
    if ( isValid() )
    {
        //Inform the user that he should attach as many devices with music as possible
        //Hopefully this won't be necessary soon.
        //
        //Currently broken, so disabled - seems to cause crashes as events are sent to
        //the Playlist - maybe it's not fully initialised?
        /*
        QString text = i18n( "Amarok has to update your database to be able to use the new Dynamic Collection(insert link) feature. Amarok now has to determine on which physical devices your collection is stored. Please attach all removable devices which contain part of your collection and continue. Cancelling will exit Amarok." );
        int result = KMessageBox::warningContinueCancel( 0, text, "Database migration" );
        if ( result != KMessageBox::Continue )
        {
            error() << "Dynamic Collection migration was aborted by user...exiting" << endl;
            exit( 1 );
        }
        */

        bool needsUpdate = ( adminValue( "Database Stats Version" ).toInt() != DATABASE_STATS_VERSION
                           || Amarok::config( "Collection Browser" )->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION
                           || Amarok::config( "Collection Browser" )->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION
                           || adminValue( "Database Version" ).toInt() != DATABASE_VERSION
                           || Amarok::config( "Collection Browser" )->readNumEntry( "Database Persistent Tables Version", 0 ) != DATABASE_PERSISTENT_TABLES_VERSION
                           || adminValue( "Database Persistent Tables Version" ).toInt() != DATABASE_PERSISTENT_TABLES_VERSION
                           || Amarok::config( "Collection Browser" )->readNumEntry( "Database Podcast Tables Version", 0 ) != DATABASE_PODCAST_TABLES_VERSION
                           || adminValue( "Database Podcast Tables Version" ).toInt() != DATABASE_PODCAST_TABLES_VERSION
                           || Amarok::config( "Collection Browser" )->readNumEntry( "Database AFT Version", 0 ) != DATABASE_AFT_VERSION
                           || adminValue( "Database AFT Version" ).toInt() != DATABASE_AFT_VERSION );

        if ( needsUpdate )
        {

            KDialogBase *dialog = new KDialogBase( KDialogBase::Swallow,
                                                   Qt::WType_TopLevel|Qt::WStyle_Customize|Qt::WStyle_DialogBorder,
                                                   0,
                                                   "Update database warning dialog",
                                                   false,
                                                   i18n( "Updating database" ),
                                                   0 );
            /* TODO: remove the standard window controls from the dialog window, the user should not be able
                     to close, minimize, maximize the dialog
                     add additional text, e.g. Amarok is currently updating your database. This may take a while.
                     Please wait.

                     Consider using a ProgressBarDialog
            */
            QLabel *label = new QLabel( i18n( "Updating database" ), dialog );
            dialog->setMainWidget( label );
            label->show();
            QTimer::singleShot( 0, dialog, SLOT( show() ) );
            //process events in the main event loop so that the dialog actually gets shown
            kapp->processEvents();
            debug() << "Beginning database update" << endl;

            updateStatsTables();

            updatePersistentTables();

            updatePodcastTables();

            //This is really a one-off call that fixes a Collection Browser glitch
            updateGroupBy();

            //remove database file if version is incompatible
            if ( Amarok::config( "Collection Browser" )->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION
                 || adminValue( "Database Version" ).toInt() != DATABASE_VERSION )
            {
                debug() << "Rebuilding database!" << endl;
                dropTables(false);
                createTables(false);
            }
            delete dialog;
        }
        emit databaseUpdateDone();
    }

    // TODO Should write to config in dtor, but it crashes...
    Amarok::config( "Collection Browser" )->writeEntry( "Database Version", DATABASE_VERSION );
    Amarok::config( "Collection Browser" )->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );
    Amarok::config( "Collection Browser" )->writeEntry( "Database Persistent Tables Version", DATABASE_PERSISTENT_TABLES_VERSION );
    Amarok::config( "Collection Browser" )->writeEntry( "Database Podcast Tables Version", DATABASE_PODCAST_TABLES_VERSION );
    Amarok::config( "Collection Browser" )->writeEntry( "Database AFT Version", DATABASE_AFT_VERSION );

    setAdminValue( "Database Version", QString::number( DATABASE_VERSION ) );
    setAdminValue( "Database Stats Version", QString::number( DATABASE_STATS_VERSION ) );
    setAdminValue( "Database Persistent Tables Version", QString::number( DATABASE_PERSISTENT_TABLES_VERSION ) );
    setAdminValue( "Database Podcast Tables Version", QString::number( DATABASE_PODCAST_TABLES_VERSION ) );
    setAdminValue( "Database AFT Version", QString::number( DATABASE_AFT_VERSION ) );

    initDirOperations();
}

void
CollectionDB::updateGroupBy()
{
    //This ugly bit of code makes sure the Group BY setting is preserved, after the
    //meanings of the values were changed due to the addition of the Composer table.
    int version = adminValue( "Database Version" ).toInt();
    if (!version) // an even older update
       version = Amarok::config( "Collection Browser" )->readNumEntry( "Database Version", 0 );

    if ( version && version < 32 )
    {
        KConfig* config = Amarok::config( "Collection Browser" );
        int m_cat1 = config->readNumEntry( "Category1" );
        int m_cat2 = config->readNumEntry( "Category2" );
        int m_cat3 = config->readNumEntry( "Category3" );
        m_cat1 = m_cat1 ? ( m_cat1 > 2 ? m_cat1 << 1 : m_cat1 ) : CollectionBrowserIds::IdArtist;
        m_cat2 = m_cat2 ? ( m_cat2 > 2 ? m_cat2 << 1 : m_cat2 ) : CollectionBrowserIds::IdAlbum;
        m_cat3 = m_cat3 ? ( m_cat3 > 2 ? m_cat3 << 1 : m_cat3 ) : CollectionBrowserIds::IdNone;
        config->writeEntry( "Category1", m_cat1 );
        config->writeEntry( "Category2", m_cat2 );
        config->writeEntry( "Category3", m_cat3 );
    }
}

void
CollectionDB::updateStatsTables()
{
    if ( adminValue( "Database Stats Version" ).toInt() != DATABASE_STATS_VERSION
          || Amarok::config( "Collection Browser" )->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION )
    {
        debug() << "Different database stats version detected! Stats table will be updated or rebuilt." << endl;

            #if 0 // causes mysterious crashes
            if( getType() == DbConnection::sqlite && QFile::exists( Amarok::saveLocation()+"collection.db" ) )
            {
                debug() << "Creating a backup of the database in "
                        << Amarok::saveLocation()+"collection-backup.db" << '.' << endl;

                bool copied = KIO::NetAccess::file_copy( Amarok::saveLocation()+"collection.db",
                                                         Amarok::saveLocation()+"collection-backup.db",
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
        if( !prev || ( Amarok::config( "Collection Browser" )->readNumEntry( "Database Stats Version", 0 )
                  && Amarok::config( "Collection Browser" )->readNumEntry( "Database Stats Version", 0 ) <= 3  ) )
            prev = Amarok::config( "Collection Browser" )->readNumEntry( "Database Stats Version", 0 );

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

        if ( prev < 3 ) //it is from before 1.2, or our poor user is otherwise fucked
        {
            debug() << "Rebuilding stats-database!" << endl;
            dropStatsTableV1();
            createStatsTable();
        }
        else        //Incrementally update the stats table to reach the present version
        {
            if( prev < 4 ) //every version from 1.2 forward had a stats version of 3
            {
                debug() << "Updating stats-database!" << endl;
                query( "ALTER TABLE statistics ADD rating INTEGER DEFAULT 0;" );
                query( "CREATE INDEX rating_stats ON statistics( rating );" );
                query( "UPDATE statistics SET rating=0 WHERE " + boolT() + ';' );
            }
            if( prev < 5 )
            {
                debug() << "Updating stats-database!" << endl;
                query( "UPDATE statistics SET rating = rating * 2;" );
            }
            if( prev < 8 )  //Versions 6, 7 and 8 all were all attempts to add columns for ATF. his code should do it all.
            {
                query( QString( "CREATE TABLE statistics_fix ("
                    "url " + textColumnType() + " UNIQUE,"
                    "createdate INTEGER,"
                    "accessdate INTEGER,"
                    "percentage FLOAT,"
                    "rating INTEGER DEFAULT 0,"
                    "playcounter INTEGER);" ) );

                insert( "INSERT INTO statistics_fix (url, createdate, accessdate, percentage, playcounter, rating)"
                        "SELECT url, createdate, accessdate, percentage, playcounter, rating FROM statistics;"
                        , NULL );

                dropStatsTableV1();
                createStatsTableV8();

                insert( "INSERT INTO statistics (url, createdate, accessdate, percentage, playcounter, rating)"
                        "SELECT url, createdate, accessdate, percentage, playcounter, rating FROM statistics_fix;"
                        , NULL );
                query( "DROP TABLE statistics_fix" );
            }
            if( prev < 9 )
            {
                //Update for Dynamic Collection:

                //This is not technically for the stats table, but it is part of the same
                //update, so put it here anyway.
                MountPointManager::instance()->setCollectionFolders( Amarok::config( "Collection" )->readPathListEntry( "Collection Folders" ) );

                query( "ALTER TABLE statistics ADD deviceid INTEGER;" );

                //FIXME: (max) i know this is bad but its fast
                QStringList oldURLs = query( "SELECT url FROM statistics;" );
                //it might be necessary to use batch updates to improve speed
                debug() << "Updating " << oldURLs.count() << " rows in statistics" << endl;
                foreach( oldURLs )
                {
                    bool exists = QFile::exists( *it );
                    int deviceid = exists ? MountPointManager::instance()->getIdForUrl( *it ) : -2;
                    QString rpath = exists ? MountPointManager::instance()->getRelativePath( deviceid, *it ) : *it;
                    QString update = QString( "UPDATE statistics SET deviceid = %1, url = '%2' WHERE " )
                                     .arg( deviceid )
                                     .arg( escapeString( rpath ) );
                    update += QString( "url = '%1';" ).arg( escapeString( *it ) );
                    query ( update );
                }
            }
            if ( prev < 12 )
            {
                //re-using old method cause just a slight change to one column...
                //if people are upgrading from earlier than 11, just get the new column
                //earlier  :-)
                createStatsTableV10( true );
                query( "INSERT INTO statistics_fix_ten SELECT url,deviceid,createdate,"
                       "accessdate,percentage,rating,playcounter,uniqueid,deleted FROM "
                       "statistics;" );
                dropStatsTableV1();
                createStatsTableV10( false );
                query( "INSERT INTO statistics SELECT * FROM statistics_fix_ten;" );
                query( "UPDATE statistics SET uniqueid=NULL;" );
            }
            else if( prev > DATABASE_STATS_VERSION )
            {
                error() << "Database statistics version too new for this version of Amarok. Quitting..." << endl;
                exit( 1 );
            }
        }
    }
}

void
CollectionDB::updatePersistentTables()
{
    QString PersistentVersion = adminValue( "Database Persistent Tables Version" );
    if ( PersistentVersion.isEmpty() )
    {
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
    else if ( PersistentVersion == "1" || PersistentVersion == "2" )
    {
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
    else
    {
        if ( PersistentVersion.toInt() < 5 )
        {
            debug() << "Updating podcast tables" << endl;
            query( "ALTER TABLE podcastchannels ADD image " + textColumnType() + ';' );
            query( "ALTER TABLE podcastepisodes ADD localurl " + textColumnType() + ';' );
            query( "ALTER TABLE podcastepisodes ADD subtitle " + textColumnType() + ';' );
            query( "ALTER TABLE podcastepisodes ADD size INTEGER;" );
            query( "ALTER TABLE podcastepisodes DROP comment;" );
            query( "ALTER TABLE podcastepisodes ADD comment " + longTextColumnType() + ';' );
            query( "CREATE INDEX localurl_podepisode ON podcastepisodes( localurl );" );
        }
        if ( PersistentVersion.toInt() < 6 )
        {
            debug() << "Updating podcast tables" << endl;
            query( "ALTER TABLE podcastchannels ADD image " + textColumnType() + ';' );
            query( "ALTER TABLE podcastepisodes ADD subtitle " + textColumnType() + ';' );
            query( "ALTER TABLE podcastepisodes ADD size INTEGER;" );
            query( "ALTER TABLE podcastepisodes DROP comment;" );
            query( "ALTER TABLE podcastepisodes ADD comment " + longTextColumnType() + ';' );
        }
        if ( PersistentVersion.toInt() < 11 )
        {
            debug() << "This is used to handle problems from uniqueid changeover and should not do anything" << endl;
        }
        if ( PersistentVersion.toInt() < 12 )
        {
            debug() << "Adding playlists table..." << endl;
            createPersistentTablesV12();
        }
        if ( PersistentVersion.toInt() < 13 )
        {
            //Update for Dynamic Collection:
            query( "ALTER TABLE lyrics ADD deviceid INTEGER;" );

            //FIXME: (max) i know this is bad but its fast
            QStringList oldURLs = query( "SELECT url FROM lyrics;" );
            //it might be necessary to use batch updates to improve speed
            debug() << "Updating " << oldURLs.count() << " rows in lyrics" << endl;
            foreach( oldURLs )
            {
                int deviceid = MountPointManager::instance()->getIdForUrl( *it );
                QString rpath = MountPointManager::instance()->getRelativePath( deviceid, *it );
                QString update = QString( "UPDATE lyrics SET deviceid = %1, url = '%2' WHERE " )
                                 .arg( deviceid )
                                 .arg( escapeString( rpath ) );
                update += QString( "url = '%1';" ).arg( escapeString( *it ) );
                query ( update );
            }
        }
        if ( PersistentVersion.toInt() < 15 )
        {
            createPersistentTablesV14( true );
            query( "INSERT INTO amazon_fix SELECT asin,locale,filename,refetchdate FROM amazon;" );
            query( "INSERT INTO lyrics_fix SELECT url,deviceid,lyrics FROM lyrics;" );
            query( "INSERT INTO playlists_fix SELECT playlist,url,tracknum FROM playlists;" );
            dropPersistentTablesV14();
            createPersistentTablesV14( false );
            query( "INSERT INTO amazon SELECT * FROM amazon_fix;" );
            query( "INSERT INTO lyrics SELECT * FROM lyrics_fix;" );
            query( "INSERT INTO playlists SELECT * FROM playlists_fix;" );
        }
        if ( PersistentVersion.toInt() < 17 )
        {
            //drop old labels and label tables, they were never used anyway and just confuse things
            query( "DROP TABLE label;" );
            query( "DROP TABLE labels;" );
            query( "DROP TABLE tags_labels;" );
            //update for label support
            QString labelsAutoIncrement = "";
            if ( getDbConnectionType() == DbConnection::postgresql )
            {
                query( QString( "CREATE SEQUENCE labels_seq;" ) );

                labelsAutoIncrement = QString("DEFAULT nextval('labels_seq')");
            }
            else if ( getDbConnectionType() == DbConnection::mysql )
            {
                labelsAutoIncrement = "AUTO_INCREMENT";
            }

            query( QString( "CREATE TABLE labels ("
                            "id INTEGER PRIMARY KEY " + labelsAutoIncrement + ", "
                            "name " + textColumnType() + ", "
                            "type INTEGER);" ) );

            query( QString( "CREATE TABLE tags_labels ("
                            "deviceid INTEGER,"
                            "url " + exactTextColumnType() + ", "
                            "uniqueid " + exactTextColumnType(32) + ", "      //m:n relationship, DO NOT MAKE UNIQUE!
                            "labelid INTEGER REFERENCES labels( id ) ON DELETE CASCADE );" ) );

            query( "CREATE UNIQUE INDEX labels_name ON labels( name, type );" );
            query( "CREATE INDEX tags_labels_uniqueid ON tags_labels( uniqueid );" ); //m:n relationship, DO NOT MAKE UNIQUE!
            query( "CREATE INDEX tags_labels_url ON tags_labels( url, deviceid );" ); //m:n relationship, DO NOT MAKE UNIQUE!
        }
        if ( PersistentVersion.toInt() < 18 )
        {
            query( "ALTER TABLE lyrics ADD uniqueid " + exactTextColumnType(32) + ';' );
            query( "CREATE INDEX lyrics_uniqueid ON lyrics( uniqueid );" );
        }
        if ( PersistentVersion.toInt() < 19 )
        {
            query( "CREATE INDEX tags_labels_labelid ON tags_labels( labelid );" );   //m:n relationship, DO NOT MAKE UNIQUE!
        }
        //Up to date. Keep this number   \/   in sync!
        if ( PersistentVersion.toInt() > 19 || PersistentVersion.toInt() < 0 )
        {
            //Something is horribly wrong
            if ( adminValue( "Database Persistent Tables Version" ).toInt() != DATABASE_PERSISTENT_TABLES_VERSION )
            {
                error() << "There is a bug in Amarok: instead of destroying your valuable"
                        << " database tables, I'm quitting" << endl;
                exit( 1 );

                debug() << "Rebuilding persistent tables database!" << endl;
                dropPersistentTables();
                createPersistentTables();
            }
        }
    }
}

void
CollectionDB::updatePodcastTables()
{
    QString PodcastVersion = adminValue( "Database Podcast Tables Version" );
    if ( PodcastVersion.toInt() < 2 )
    {
        createPodcastTablesV2( true );
        query( "INSERT INTO podcastchannels_fix SELECT url,title,weblink,image,comment,"
               "copyright,parent,directory,autoscan,fetchtype,autotransfer,haspurge,"
               "purgecount FROM podcastchannels;" );
        query( "INSERT INTO podcastepisodes_fix SELECT id,url,localurl,parent,guid,title,"
               "subtitle,composer,comment,filetype,createdate,length,size,isNew FROM "
               "podcastepisodes;" );
        query( "INSERT INTO podcastfolders_fix SELECT id,name,parent,isOpen FROM podcastfolders;" );
        dropPodcastTablesV2();
        createPodcastTablesV2( false );
        query( "INSERT INTO podcastchannels SELECT * FROM podcastchannels_fix;" );
        query( "INSERT INTO podcastepisodes SELECT * FROM podcastepisodes_fix;" );
        query( "INSERT INTO podcastfolders SELECT * FROM podcastfolders_fix;" );
    }

    //Keep this number in sync    \/
    if ( PodcastVersion.toInt() > 2 )
    {
        error() << "Something is very wrong with the Podcast Tables. Aborting" << endl;
        exit( 1 );
        dropPodcastTables();
        createPodcastTables();
    }
}

void
CollectionDB::vacuum()
{
    if ( DbConnection::sqlite == getDbConnectionType() ||
         DbConnection::postgresql == getDbConnectionType() )
    {
        //Clean up DB and free unused space.
        debug() << "Running VACUUM" << endl;
        query( "VACUUM;" );
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
    if ( !m_scanInProgress
            && ( !CollectionView::instance() || !CollectionView::instance()->isOrganizingFiles() )
            && ( !MediaBrowser::instance() || !MediaBrowser::instance()->isTranscoding() ) )
    {
        //we check if a job is pending because we don't want to abort incremental collection readings
        if ( !ThreadManager::instance()->isJobPending( "CollectionScanner" ) && PlaylistBrowser::instance() )
        {
            m_scanInProgress = true;
            m_rescanRequired = false;
            emit scanStarted();

            ThreadManager::instance()->onlyOneJob( new ScanController( this, true ) );
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
    return QString();
}


bool
CollectionDB::extractEmbeddedImage( const MetaBundle &trackInformation, QCString& hash )
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

QStringList
CollectionDB::getLabels( const QString &url, const uint type )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
    return query( QString( "SELECT labels.name FROM labels "
                           "LEFT JOIN tags_labels ON labels.id = tags_labels.labelid "
                           "WHERE labels.type = %1 AND tags_labels.deviceid = %2 AND tags_labels.url = '%3';" )
                         .arg( type ).arg( deviceid ).arg( escapeString( rpath ) ) );
}

void
CollectionDB::cleanLabels()
{
    DEBUG_BLOCK
    QStringList labelIds = query( "select labels.id "
                                  "from labels left join tags_labels on labels.id = tags_labels.labelid "
                                  "where tags_labels.labelid is NULL;" );
    if ( !labelIds.isEmpty() )
    {
        QString ids;
        foreach( labelIds )
        {
            if ( !ids.isEmpty() )
                ids += ',';
            ids += *it;
        }
        query( QString( "DELETE FROM labels "
                        "WHERE labels.id IN ( %1 );" )
                        .arg( ids ) );
    }
}

void
CollectionDB::setLabels( const QString &url, const QStringList &labels, const QString &uid, const uint type )
{
    DEBUG_BLOCK
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = escapeString( MountPointManager::instance()->getRelativePath( deviceid, url ) );
    QStringList labelIds = query( QString( "SELECT id FROM labels WHERE type = %1;" ).arg( type ) );
    QString ids;
    if ( !labelIds.isEmpty() )
    {
        foreach( labelIds )
        {
            if ( !ids.isEmpty() )
                ids += ',';
            ids += *it;
        }
        //TODO: max: add uniqueid handling
        query( QString( "DELETE FROM tags_labels "
                        "WHERE tags_labels.labelid IN (%1) AND tags_labels.deviceid = %2 AND tags_labels.url = '%3';" )
                        .arg( ids,  QString::number(deviceid),  rpath ) );
    }

    foreach( labels )
    {
        int id = query( QString( "SELECT id FROM labels WHERE type = %1 AND name = '%2';" )
                                         .arg( type ).arg( escapeString( *it ) ) ).first().toInt();
        if ( !id )
        {
            id = insert( QString( "INSERT INTO labels( name, type ) VALUES ( '%2', %1 );" )
                                  .arg( type ).arg( escapeString( *it ) ), "labels" );
        }
        insert( QString( "INSERT INTO tags_labels( labelid, deviceid, url, uniqueid ) VALUES ( %1, %2, '%3', '%4' );" )
                         .arg( QString::number(id), QString::number(deviceid),  rpath, escapeString( uid ) ), 0 );
    }

    emit labelsChanged( url );
}

void
CollectionDB::removeLabels( const QString &url, const QStringList &labels, const uint type )
{
    DEBUG_BLOCK
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = escapeString( MountPointManager::instance()->getRelativePath( deviceid, url ) );
    QString sql = QString( "DELETE FROM tags_labels "
                             "FROM tags_labels AS t LEFT JOIN labels AS l ON t.labelid = l.id "
                             "WHERE l.type = %1 AND t.deviceid = %2 AND t.url = '%3' AND ( 0" )
                             .arg( type ).arg( deviceid ).arg( rpath );
    foreach( labels )
    {
        sql += QString( " OR l.name = '%1'" ).arg( escapeString( *it ) );
    }
    sql += ");";
    query( sql );

    emit labelsChanged( url );
}

bool
CollectionDB::addLabel( const QString &url, const QString &label, const QString &uid, const uint type )
{
    DEBUG_BLOCK
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = escapeString( MountPointManager::instance()->getRelativePath( deviceid, url ) );

    int id = query( QString( "SELECT id FROM labels WHERE type = %1 AND name = '%2';" )
                             .arg( type ).arg( escapeString( label ) ) ).first().toInt();
    bool labelAlreadyExists = id > 0;
    if ( !id )
    {
        id = insert( QString( "INSERT INTO labels( name, type ) VALUES ( '%2', %1 );" )
                              .arg( type ).arg( escapeString( label ) ), "labels" );
    }
    if ( labelAlreadyExists )
    {
        //we can return if the link between the tags row and the labels row already exists
        int count = query( QString( "SELECT COUNT(*) FROM tags_labels WHERE labelid = %1 AND deviceid = %2 AND url = '%3';" )
                                    .arg( id ).arg( deviceid ).arg( rpath ) ).first().toInt();
        if ( count )
            return false;
    }
    insert( QString( "INSERT INTO tags_labels( labelid, deviceid, url, uniqueid ) VALUES ( %1, %2, '%3', '%4' );" )
                     .arg( QString::number(id), QString::number(deviceid), rpath, escapeString( uid ) ), "tags_labels" );

    emit labelsChanged( url );
    return true;
}

QStringList
CollectionDB::favoriteLabels( int type, int count )
{
    return query( QString( "SELECT labels.name, count( tags_labels.labelid ) "
                           "FROM labels LEFT JOIN tags_labels ON labels.id = tags_labels.labelid "
                           "WHERE labels.type = %1 GROUP BY labels.name "
                           "ORDER BY count(tags_labels.labelid) DESC LIMIT %2;" )
                           .arg( QString::number( type ), QString::number( count ) ) );
}

QDir
CollectionDB::largeCoverDir() //static
{
    return QDir( Amarok::saveLocation( "albumcovers/large/" ) );
}


QDir
CollectionDB::tagCoverDir()  //static
{
    return QDir( Amarok::saveLocation( "albumcovers/tagcover/" ) );
}


QDir
CollectionDB::cacheCoverDir()  //static
{
    return QDir( Amarok::saveLocation( "albumcovers/cache/" ) );
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

SqliteConnection::SqliteConnection( const SqliteConfig* config )
    : DbConnection()
    , m_db( 0 )
{

    DEBUG_BLOCK

    const QCString path = QFile::encodeName( config->dbFile() );

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
        if ( sqlite3_create_function(m_db, "like", 2, SQLITE_UTF8, NULL, sqlite_like_new, NULL, NULL) != SQLITE_OK )
            m_initialized = false;
        if ( sqlite3_create_function(m_db, "like", 3, SQLITE_UTF8, NULL, sqlite_like_new, NULL, NULL) != SQLITE_OK )
            m_initialized = false;
    }

    //optimization for speeding up SQLite
    query( "PRAGMA default_synchronous = OFF;" );
}


SqliteConnection::~SqliteConnection()
{
    if ( m_db ) sqlite3_close( m_db );
}


QStringList SqliteConnection::query( const QString& statement, bool /*suppressDebug*/ )
{

    QStringList values;
    int error;
    int rc = 0;
    const char* tail;
    sqlite3_stmt* stmt;
    int busyCnt = 0;
    int retryCnt = 0;

    do {
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
            break;
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
            //deallocate vm resources
            rc = sqlite3_finalize( stmt );

            if ( error != SQLITE_DONE && rc != SQLITE_SCHEMA )
            {
                Debug::error() << k_funcinfo << "sqlite_step error.\n";
                Debug::error() << sqlite3_errmsg( m_db ) << endl;
                Debug::error() << "on query: " << statement << endl;
                values = QStringList();
            }
            if ( rc == SQLITE_SCHEMA )
            {
                retryCnt++;
                debug() << "SQLITE_SCHEMA error occurred on query: " << statement << endl;
                if ( retryCnt < 10 )
                    debug() << "Retrying now." << endl;
                else
                {
                    Debug::error() << "Retry-Count has reached maximum. Aborting this SQL statement!" << endl;
                    Debug::error() << "SQL statement: " << statement << endl;
                    values = QStringList();
                }
            }
        }
    }
    while ( rc == SQLITE_SCHEMA && retryCnt < 10 );

    return values;
}


int SqliteConnection::insert( const QString& statement, const QString& /* table */ )
{
    int error;
    int rc = 0;
    const char* tail;
    sqlite3_stmt* stmt;
    int busyCnt = 0;
    int retryCnt = 0;

    do {
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
            break;
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
            //deallocate vm resources
            rc = sqlite3_finalize( stmt );

            if ( error != SQLITE_DONE && rc != SQLITE_SCHEMA)
            {
                Debug::error() << k_funcinfo << "sqlite_step error.\n";
                Debug::error() << sqlite3_errmsg( m_db ) << endl;
                Debug::error() << "on insert: " << statement << endl;
            }
            if ( rc == SQLITE_SCHEMA )
            {
                retryCnt++;
                debug() << "SQLITE_SCHEMA error occurred on insert: " << statement << endl;
                if ( retryCnt < 10 )
                    debug() << "Retrying now." << endl;
                else
                {
                    Debug::error() << "Retry-Count has reached maximum. Aborting this SQL insert!" << endl;
                    Debug::error() << "SQL statement: " << statement << endl;
                }
            }
        }
    }
    while ( SQLITE_SCHEMA == rc && retryCnt < 10 );
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

// this implements a LIKE() function that overrides the default string comparison function
// Reason: default function is case-sensitive for utf8 strings (BUG: 116458, ...)
void SqliteConnection::sqlite_like_new( sqlite3_context *context, int argc, sqlite3_value **argv )
{

    const unsigned char *zA = sqlite3_value_text( argv[0] );
    const unsigned char *zB = sqlite3_value_text( argv[1] );

    QString pattern = QString::fromUtf8( (const char*)zA );
    QString text = QString::fromUtf8( (const char*)zB );

    int begin = pattern.startsWith( "%" ), end = pattern.endsWith( "%" );
    if (begin)
        pattern = pattern.right( pattern.length() - 1 );
    if (end)
        pattern = pattern.left( pattern.length() - 1 );

    if( argc == 3 ) // The function is given an escape character. In likeCondition() it defaults to '/'
        pattern.replace( "/%", "%" ).replace( "/_", "_" ).replace( "//", "/" );

    int result = 0;
    if ( begin && end ) result = ( text.find( pattern, 0, 0 ) != -1);
    else if ( begin ) result = text.endsWith( pattern, 0 );
    else if ( end ) result = text.startsWith( pattern, 0 );
    else result = ( text.lower() == pattern.lower() );

    sqlite3_result_int( context, result );
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
                    { m_connected = true; m_initialized = true; }
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


QStringList MySqlConnection::query( const QString& statement, bool suppressDebug )
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
        }
        else
        {
            if ( mysql_field_count( m_db ) != 0 )
            {
                if ( !suppressDebug )
                    debug() << "MYSQL QUERY FAILED: " << mysql_error( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
                values = QStringList();
            }
        }
        mysql_free_result( result );
    }
    else
    {
        if ( !suppressDebug )
            debug() << "MYSQL QUERY FAILED: " << mysql_error( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
        values = QStringList();
    }

    return values;
}


int MySqlConnection::insert( const QString& statement, const QString& /* table */ )
{
    mysql_query( m_db, statement.utf8() );
    if ( mysql_errno( m_db ) )
    {
        debug() << "MYSQL INSERT FAILED: " << mysql_error( m_db ) << "\n" << "FAILED INSERT: " << statement << endl;
    }
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
      "' password='" + config->password() + '\'';

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


QStringList PostgresqlConnection::query( const QString& statement, bool suppressDebug )
{
    QStringList values;
    PGresult* result;
    ExecStatusType status;

    result = PQexec(m_db, statement.utf8());
    if (result == NULL)
    {
        if ( !suppressDebug )
            debug() << "POSTGRESQL QUERY FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
        return values;
    }

    status = PQresultStatus(result);
    if ((status != PGRES_COMMAND_OK) && (status != PGRES_TUPLES_OK))
    {
        if ( !suppressDebug )
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
{
    m_OR.push(false);
    clear();
    // there are a few string members with a large number of appends. to
    // avoid reallocations, pre-reserve 1024 bytes and try never to assign
    // it, instead doing setLength(0) and appends
    // Note: unfortunately, QT3's setLength(), which is also called from append,
    // squeezes the string if it's less than 4x the length. So this is useless.
    // Uncomment after porting to QT4 if it's smarter about this, as the docs say.
//     m_query.reserve(1024);
//     m_values.reserve(1024);
//     m_tables.reserve(1024);
}


void
QueryBuilder::linkTables( int tables )
{
    m_tables.setLength(0);

    m_tables += tableName( tabSong );

    if ( !(tables & tabSong ) )
    {
        // check if only one table is selected (does somebody know a better way to check that?)
        if (tables == tabAlbum || tables==tabArtist || tables==tabGenre || tables == tabYear || tables == tabStats || tables == tabPodcastEpisodes || tables == tabPodcastFolders || tables == tabPodcastChannels || tables == tabLabels) {
        m_tables.setLength( 0 );
        m_tables += tableName(tables);
    }
        else
            tables |= tabSong;
    }

    if ( tables & tabSong )
    {
        if ( tables & tabAlbum )
            ((m_tables += " LEFT JOIN ") += tableName( tabAlbum)) += " ON album.id=tags.album";
        if ( tables & tabArtist )
            ((m_tables += " LEFT JOIN ") += tableName( tabArtist)) += " ON artist.id=tags.artist";
        if ( tables & tabComposer )
            ((m_tables += " LEFT JOIN ") += tableName( tabComposer)) += " ON composer.id=tags.composer";
        if ( tables & tabGenre )
            ((m_tables += " LEFT JOIN ") += tableName( tabGenre)) += " ON genre.id=tags.genre";
        if ( tables & tabYear )
            ((m_tables += " LEFT JOIN ") += tableName( tabYear)) += " ON year.id=tags.year";
        if ( tables & tabStats )
        {
            ((m_tables += " LEFT JOIN ") += tableName( tabStats))
                                      += " ON statistics.url=tags.url AND statistics.deviceid = tags.deviceid";
            //if ( !m_url.isEmpty() ) {
            //    QString url = QString( '.' ) + m_url;
            //    m_tables += QString( " OR statistics.deviceid = -1 AND statistics.url = '%1'" )
            //                                    .arg( CollectionDB::instance()->escapeString( url ) );
            //}
        }
        if ( tables & tabLyrics )
            ((m_tables += " LEFT JOIN ") += tableName( tabLyrics))
                                      += " ON lyrics.url=tags.url AND lyrics.deviceid = tags.deviceid";

        if ( tables & tabDevices )
            ((m_tables += " LEFT JOIN ") += tableName( tabDevices )) += " ON tags.deviceid = devices.id";
        if ( tables & tabLabels )
            ( m_tables += " LEFT JOIN tags_labels ON tags.url = tags_labels.url AND tags.deviceid = tags_labels.deviceid" )
                += " LEFT JOIN labels ON tags_labels.labelid = labels.id";
    }
}


void
QueryBuilder::addReturnValue( int table, Q_INT64 value, bool caseSensitive /* = false, unless value refers to a string */ )
{
    caseSensitive |= value == valName || value == valTitle || value == valComment;

    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ',';

    if ( value == valDummy )
        m_values += "''";
    else
    {
        if ( caseSensitive && CollectionDB::instance()->getType() == DbConnection::mysql )
            m_values += "BINARY ";
        m_values += tableName( table ) + '.';
        m_values += valueName( value );
    }

    m_linkTables |= table;
    m_returnValues++;
    if ( value & valURL )
    {
        // make handling of deviceid transparent to calling code
        m_deviceidPos = m_returnValues + 1;  //the return value after the url is the deviceid
        m_values += ',';
        m_values += tableName( table );
        m_values += '.';
        m_values += valueName( valDeviceId );
    }
}

void
QueryBuilder::addReturnFunctionValue( int function, int table, Q_INT64 value)
{
    // translate NULL and 0 values into the default value for percentage/rating
    // First translate 0 to NULL via NULLIF, then NULL to default via COALESCE
    bool defaults = function == funcAvg && ( value & valScore || value & valRating );

    if ( !m_values.isEmpty() && m_values != "DISTINCT " ) m_values += ',';
    m_values += functionName( function ) + '(';
    if ( defaults )
        m_values += "COALESCE(NULLIF(";
    m_values += tableName( table ) + '.';
    m_values += valueName( value );
    if ( defaults )
    {
        m_values += ", 0), ";
        if ( value & valScore )
            m_values += "50";
        else
            m_values += '6';
        m_values += ')';
    }
    m_values += ") AS ";
    m_values += functionName( function )+tableName( table )+valueName( value );

    m_linkTables |= table;
    if ( !m_showAll ) m_linkTables |= tabSong;
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
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';

        for ( uint i = 0; i < filter.count(); i++ )
        {
                int deviceid = MountPointManager::instance()->getIdForUrl( filter[i] );
                QString rpath = MountPointManager::instance()->getRelativePath( deviceid , filter[i] );
                m_where += "OR (tags.url = '" + CollectionDB::instance()->escapeString( rpath ) + "' ";
                m_where += QString( "AND tags.deviceid = %1 ) " ).arg( QString::number( deviceid ) );
                //TODO MountPointManager fix this
        }

        m_where += " ) ";
    }

    m_linkTables |= tabSong;
}

void
QueryBuilder::setGoogleFilter( int defaultTables, QString query )
{
    //TODO MountPointManager fix google syntax
    //no clue about what needs to be done atm
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

            int table = -1;
            Q_INT64 value = -1;
            if( e.field == "artist" )
                table = tabArtist;
            else if( e.field == "composer" )
                table = tabComposer;
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
            else if( e.field == "filetype" || e.field == "type" )
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
            else if( e.field == "bpm" )
            {
                table = tabSong;
                value = valBPM;
                exact = true;
            }
            else if( e.field == "lyrics" )
            {
                table = tabLyrics;
                value = valLyrics;
            }
            else if( e.field == "device" )
            {
                table = tabDevices;
                value = valDeviceLabel;
            }
            else if( e.field == "mountpoint" )
            {
                table = tabDevices;
                value = valMountPoint;
            }
            else if( e.field == "label" )
            {
                table = tabLabels;
                value = valName;
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
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';

        if ( tables & tabAlbum )
            m_where += "OR album.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabArtist )
            m_where += "OR artist.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabComposer )
            m_where += "OR composer.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabGenre )
            m_where += "OR genre.name " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabYear )
            m_where += "OR year.name " + CollectionDB::likeCondition( filter, false, false );
        if ( tables & tabSong )
            m_where += "OR tags.title " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabLabels )
            m_where += "OR labels.name " + CollectionDB::likeCondition( filter, true, true );

        if ( i18n( "Unknown" ).contains( filter, false ) )
        {
            if ( tables & tabAlbum )
                m_where += "OR album.name = '' ";
            if ( tables & tabArtist )
                m_where += "OR artist.name = '' ";
            if ( tables & tabComposer )
                m_where += "OR composer.name = '' ";
            if ( tables & tabGenre )
                m_where += "OR genre.name = '' ";
            if ( tables & tabYear )
                m_where += "OR year.name = '' ";
            if ( tables & tabSong )
                m_where += "OR tags.title = '' ";
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
    //true for INTEGER fields (see comment of coalesceField(int, Q_INT64)
    bool useCoalesce = coalesceField( tables, value );
    m_where += ANDslashOR() + " ( ";

    QString m, s;
    if (mode == modeLess || mode == modeGreater)
    {
        QString escapedFilter;
        if (useCoalesce && DbConnection::sqlite == CollectionDB::instance()->getDbConnectionType())
            escapedFilter = CollectionDB::instance()->escapeString( filter );
        else
            escapedFilter = "'" + CollectionDB::instance()->escapeString( filter ) + "' ";
        s = ( mode == modeLess ? "< " : "> " ) + escapedFilter;
    }
    else
    {
        if (exact)
            if (useCoalesce && DbConnection::sqlite == CollectionDB::instance()->getDbConnectionType())
                s = " = " +CollectionDB::instance()->escapeString( filter ) + ' ';
            else
                s = " = '" + CollectionDB::instance()->escapeString( filter ) + "' ";
        else
            s = CollectionDB::likeCondition( filter, mode != modeBeginMatch, mode != modeEndMatch );
    }

    if( coalesceField( tables, value ) )
        m_where += QString( "COALESCE(%1.%2,0) " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;
    else
        m_where += QString( "%1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;

    if ( !exact && (value & valName) && mode == modeNormal && i18n( "Unknown").contains( filter, false ) )
        m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

    m_where += " ) ";

    m_linkTables |= tables;
}

void
QueryBuilder::addFilters( int tables, const QStringList& filter )
{
    if ( !filter.isEmpty() )
    {
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + ' ';

        for ( uint i = 0; i < filter.count(); i++ )
        {
            m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';

            if ( tables & tabAlbum )
                m_where += "OR album.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabArtist )
                m_where += "OR artist.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabComposer )
                m_where += "OR composer.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabGenre )
                m_where += "OR genre.name " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabYear )
                m_where += "OR year.name " + CollectionDB::likeCondition( filter[i], false, false );
            if ( tables & tabSong )
                m_where += "OR tags.title " + CollectionDB::likeCondition( filter[i], true, true );
            if ( tables & tabLabels )
                m_where += "OR labels.name " + CollectionDB::likeCondition( filter[i], true, true );

            if ( i18n( "Unknown" ).contains( filter[i], false ) )
            {
                if ( tables & tabAlbum )
                    m_where += "OR album.name = '' ";
                if ( tables & tabArtist )
                    m_where += "OR artist.name = '' ";
                if ( tables & tabComposer )
                    m_where += "OR composer.name = '' ";
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
        m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + ' ';


        if ( tables & tabAlbum )
            m_where += "AND album.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabArtist )
            m_where += "AND artist.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabComposer )
            m_where += "AND composer.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabGenre )
            m_where += "AND genre.name NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabYear )
            m_where += "AND year.name NOT " + CollectionDB::likeCondition( filter, false, false );
        if ( tables & tabSong )
            m_where += "AND tags.title NOT " + CollectionDB::likeCondition( filter, true, true );
        if ( tables & tabLabels )
            m_where += "AND labels.name NOT " + CollectionDB::likeCondition( filter, true, true );

        if ( i18n( "Unknown" ).contains( filter, false ) )
        {
            if ( tables & tabAlbum )
                m_where += "AND album.name <> '' ";
            if ( tables & tabArtist )
                m_where += "AND artist.name <> '' ";
            if ( tables & tabComposer )
                m_where += "AND composer.name <> '' ";
            if ( tables & tabGenre )
                m_where += "AND genre.name <> '' ";
            if ( tables & tabYear )
                m_where += "AND year.name <> '' ";
            if ( tables & tabSong )
                m_where += "AND tags.title <> '' ";
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
    m_where += ANDslashOR() + " ( ";

    QString m, s;
    if (mode == modeLess || mode == modeGreater)
        s = ( mode == modeLess ? ">= '" : "<= '" ) + CollectionDB::instance()->escapeString( filter ) + "' ";
    else
    {
        if (exact)
        {
            bool isNumber;
            filter.toInt( &isNumber );
            if (isNumber)
                s = " <> " + CollectionDB::instance()->escapeString( filter ) + " ";
            else
                s = " <> '" + CollectionDB::instance()->escapeString( filter ) + "' ";
        }
        else
            s = "NOT " + CollectionDB::instance()->likeCondition( filter, mode != modeBeginMatch, mode != modeEndMatch ) + ' ';
    }

    if( coalesceField( tables, value ) )
        m_where += QString( "COALESCE(%1.%2,0) " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;
    else
        m_where += QString( "%1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) ) + s;

    if ( !exact && (value & valName) && mode == modeNormal && i18n( "Unknown").contains( filter, false ) )
        m_where += QString( "AND %1.%2 <> '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

    m_where += " ) ";

    m_linkTables |= tables;
}

void
QueryBuilder::addMatch( int tables, const QString& match, bool interpretUnknown /* = true */, bool caseSensitive /* = true */ )
{
    QString matchCondition = caseSensitive ? CollectionDB::exactCondition( match ) : CollectionDB::likeCondition( match );

    (((m_where += ANDslashOR()) += " ( ") += CollectionDB::instance()->boolF()) += ' ';
    if ( tables & tabAlbum )
        (m_where += "OR album.name ") += matchCondition;
    if ( tables & tabArtist )
        (m_where += "OR artist.name ") += matchCondition;
    if ( tables & tabComposer )
        (m_where += "OR composer.name ") += matchCondition;
    if ( tables & tabGenre )
        (m_where += "OR genre.name ") += matchCondition;
    if ( tables & tabYear )
        (m_where += "OR year.name ") += matchCondition;
    if ( tables & tabSong )
        (m_where += "OR tags.title ") += matchCondition;
    if ( tables & tabLabels )
        (m_where += "OR labels.name ") += matchCondition;

    static QString i18nUnknown = i18n("Unknown");

    if ( interpretUnknown && match == i18nUnknown )
    {
        if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
        if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
        if ( tables & tabComposer ) m_where += "OR composer.name = '' ";
        if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
        if ( tables & tabYear ) m_where += "OR year.name = '' ";
    }
    if ( tables & tabLabels && match.isEmpty() )
        m_where += " OR labels.name IS NULL ";
    m_where += " ) ";

    m_linkTables |= tables;
}


void
QueryBuilder::addMatch( int tables, Q_INT64 value, const QString& match, bool interpretUnknown /* = true */, bool caseSensitive /* = true */  )
{
    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';
    if ( value & valURL )
        m_url = match;
    //FIXME max: doesn't work yet if we are querying for the mount point part of a directory
    if ( value & valURL || value & valDirectory )
    {
        int deviceid = MountPointManager::instance()->getIdForUrl( match );
        QString rpath = MountPointManager::instance()->getRelativePath( deviceid, match );
        //we are querying for a specific path, so we don't need the tags.deviceid IN (...) stuff
        //which is automatially appended if m_showAll = false
        m_showAll = true;
        m_where += QString( "OR %1.%2 " )
            .arg( tableName( tables ) )
            .arg( valueName( value ) );
        m_where += caseSensitive ? CollectionDB::exactCondition( rpath ) : CollectionDB::likeCondition( rpath );
        m_where += QString( " AND %1.deviceid = %2 " ).arg( tableName( tables ) ).arg( deviceid );
        if ( deviceid != -1 )
        {
            //handle corner case
            QString rpath2( '.' + match );
            m_where += QString( " OR %1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) );
            m_where += caseSensitive ? CollectionDB::exactCondition( rpath2 ) : CollectionDB::likeCondition( rpath2 );
            m_where += QString( " AND %1.deviceid = -1 " ).arg( tableName( tables ) );
        }
    }
    else
    {
        m_where += QString( "OR %1.%2 " ).arg( tableName( tables ) ).arg( valueName( value ) );
        m_where += caseSensitive ? CollectionDB::exactCondition( match ) : CollectionDB::likeCondition( match );
    }

    if ( ( value & valName ) && interpretUnknown && match == i18n( "Unknown" ) )
        m_where += QString( "OR %1.%2 = '' " ).arg( tableName( tables ) ).arg( valueName( value ) );

    m_where += " ) ";

    m_linkTables |= tables;
}


void
QueryBuilder::addMatches( int tables, const QStringList& match, bool interpretUnknown /* = true */, bool caseSensitive /* = true */ )
{
    QStringList matchConditions;
    for ( uint i = 0; i < match.count(); i++ )
        matchConditions << ( caseSensitive ? CollectionDB::exactCondition( match[i] ) : CollectionDB::likeCondition( match[i] ) );

    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolF() + ' ';

    for ( uint i = 0; i < match.count(); i++ )
    {
        if ( tables & tabAlbum )
            m_where += "OR album.name " + matchConditions[ i ];
        if ( tables & tabArtist )
            m_where += "OR artist.name " + matchConditions[ i ];
        if ( tables & tabComposer )
            m_where += "OR composer.name " + matchConditions[ i ];
        if ( tables & tabGenre )
            m_where += "OR genre.name " + matchConditions[ i ];
        if ( tables & tabYear )
            m_where += "OR year.name " + matchConditions[ i ];
        if ( tables & tabSong )
            m_where += "OR tags.title " + matchConditions[ i ];
        if ( tables & tabStats )
            m_where += "OR statistics.url " + matchConditions[ i ];
        if ( tables & tabLabels )
            (m_where += "OR labels.name ") += matchConditions[ i ];


        if ( interpretUnknown && match[i] == i18n( "Unknown" ) )
        {
            if ( tables & tabAlbum ) m_where += "OR album.name = '' ";
            if ( tables & tabArtist ) m_where += "OR artist.name = '' ";
            if ( tables & tabComposer ) m_where += "OR composer.name = '' ";
            if ( tables & tabGenre ) m_where += "OR genre.name = '' ";
            if ( tables & tabYear ) m_where += "OR year.name = '' ";
        }
        if ( tables & tabLabels && match[i].isEmpty() )
            m_where += " OR labels.name IS NULL ";
    }

    m_where += " ) ";
    m_linkTables |= tables;
}

void
QueryBuilder::excludeMatch( int tables, const QString& match )
{
    m_where += ANDslashOR() + " ( " + CollectionDB::instance()->boolT() + ' ';
    if ( tables & tabAlbum ) m_where += "AND album.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabArtist ) m_where += "AND artist.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabComposer ) m_where += "AND composer.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabGenre ) m_where += "AND genre.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabYear ) m_where += "AND year.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabSong ) m_where += "AND tags.title <> '" + CollectionDB::instance()->escapeString( match ) + "' ";
    if ( tables & tabLabels ) m_where += "AND labels.name <> '" + CollectionDB::instance()->escapeString( match ) + "' ";

    if ( match == i18n( "Unknown" ) )
    {
        if ( tables & tabAlbum ) m_where += "AND album.name <> '' ";
        if ( tables & tabArtist ) m_where += "AND artist.name <> '' ";
        if ( tables & tabComposer ) m_where += "AND composer.name <> '' ";
        if ( tables & tabGenre ) m_where += "AND genre.name <> '' ";
        if ( tables & tabYear ) m_where += "AND year.name <> '' ";
    }
    m_where += " ) ";

    m_linkTables |= tables;
}


void
QueryBuilder::exclusiveFilter( int tableMatching, int tableNotMatching, Q_INT64 value )
{
    m_where += " AND ";
    m_where += tableName( tableNotMatching ) + '.';
    m_where += valueName( value );
    m_where += " IS null ";

    m_linkTables |= tableMatching;
    m_linkTables |= tableNotMatching;
}


void
QueryBuilder::addNumericFilter(int tables, Q_INT64 value, const QString &n,
                               int mode /* = modeNormal */,
                               const QString &endRange /* = QString::null */ )
{
    m_where.append( ANDslashOR() ).append( " ( " );

    if ( coalesceField( tables, value) )
        m_where.append("COALESCE(");

    m_where.append( tableName( tables ) ).append( '.' ).append( valueName( value ) );

    if ( coalesceField( tables, value) )
        m_where.append(",0)");

    switch (mode) {
    case modeNormal:
        m_where.append( " = " ); break;
    case modeLess:
        m_where.append( " < " ); break;
    case modeGreater:
        m_where.append( " > " ); break;
    case modeBetween:
        m_where.append( " BETWEEN " ); break;
    case modeNotBetween:
        m_where.append(" NOT BETWEEN "); break;
    default:
        qWarning( "Unhandled mode in addNumericFilter, using equals: %d", mode );
        m_where.append( " = " );
    }

    m_where.append( n );
    if ( mode == modeBetween || mode == modeNotBetween )
        m_where.append( " AND " ).append( endRange );

    m_where.append( " ) " );
    m_linkTables |= tables;
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
                m_sort += ',';
            m_sort += CollectionDB::instance()->randomFunc() + ' ';
    }
    else
    {
            if ( options & optRemoveDuplicates )
                m_values = "DISTINCT " + m_values;
            if ( options & optRandomize )
            {
                if ( !m_sort.isEmpty() ) m_sort += ',';
                m_sort += CollectionDB::instance()->randomFunc() + ' ';
            }
    }

    if ( options & optShowAll ) m_showAll = true;
}


void
QueryBuilder::sortBy( int table, Q_INT64 value, bool descending )
{
    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valRating || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate ||
         value & valFilesize || value & valDiscNumber ||
         table & tabYear )
        b = false;

	// only coalesce for certain columns
	bool c = false;
    if ( value & valScore || value & valRating || value & valPlayCounter || value & valAccessDate || value & valCreateDate )
		c = true;

    if ( !m_sort.isEmpty() ) m_sort += ',';
    if ( b ) m_sort += "LOWER( ";
    if ( c ) m_sort += "COALESCE( ";

    m_sort += tableName( table ) + '.';
    m_sort += valueName( value );

    if ( c ) m_sort += ", 0 )";

    if ( b ) m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        if (!m_values.isEmpty()) m_values += ',';
        if ( b ) m_values += "LOWER( ";
        m_values += tableName( table ) + '.';
        m_values += valueName( value );
        if ( b ) m_values += ')';
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
    bool defaults = function == funcAvg && ( value & valScore || value & valRating );

    //shall we sort case-sensitively? (not for integer columns!)
    bool b = true;
    if ( value & valID || value & valTrack || value & valScore || value & valRating || value & valLength || value & valBitrate ||
         value & valSamplerate || value & valPlayCounter || value & valAccessDate || value & valCreateDate ||
         value & valFilesize || value & valDiscNumber ||
         table & tabYear )
        b = false;

    // only coalesce for certain columns
    bool c = false;
    if ( !defaults && ( value & valScore || value & valRating || value & valPlayCounter || value & valAccessDate || value & valCreateDate ) )
        c = true;

    if ( !m_sort.isEmpty() ) m_sort += ',';
    //m_sort += functionName( function ) + '(';
    if ( b ) m_sort += "LOWER( ";
    if ( c && CollectionDB::instance()->getType() != DbConnection::mysql) m_sort += "COALESCE( ";

    QString columnName;

    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
    {
        columnName = functionName( function ) + '(';
        if ( defaults )
            columnName += "COALESCE(NULLIF(";
        columnName += tableName( table )+'.'+valueName( value );
        if ( defaults )
        {
            columnName += ", 0), ";
            if ( value & valScore )
                columnName += "50";
            else
                columnName += '6';
            columnName += ')';
        }
        columnName += ')';
    }
    else
        columnName = functionName( function )+tableName( table )+valueName( value );

    m_sort += columnName;

    if ( c && CollectionDB::instance()->getType() != DbConnection::mysql) m_sort += ", 0 )";

    if ( b ) m_sort += " ) ";
    //m_sort += " ) ";
    if ( descending ) m_sort += " DESC ";

    m_linkTables |= table;
}

void
QueryBuilder::groupBy( int table, Q_INT64 value )
{
    if ( !m_group.isEmpty() ) m_group += ',';

    //Do case-sensitive comparisons for MySQL too. See also QueryBuilder::addReturnValue
    if ( DbConnection::mysql == CollectionDB::instance()->getDbConnectionType() &&
         ( value == valName || value == valTitle || value == valComment ) )
    {
        m_group += "BINARY ";
    }

    m_group += tableName( table ) + '.';
    m_group += valueName( value );

    m_linkTables |= table;
}

void
QueryBuilder::having( int table, Q_INT64 value, int function, int mode, const QString& match )
{
    if( !m_having.isEmpty() ) m_having += " AND ";

    QString fn = functionName( function );
    fn.isEmpty() ?
        m_having += tableName( table ) + '.' + valueName( value ) :
        m_having += functionName( function )+'('+tableName( table )+'.'+valueName( value )+')';

    switch( mode )
    {
        case modeNormal:
            m_having += '=' + match;
            break;

        case modeLess:
            m_having += '<' + match;
            break;

        case modeGreater:
            m_having += '>' + match;

        default:
            break;
    }
}

void
QueryBuilder::setLimit( int startPos, int length )
{
    m_limit = QString( " LIMIT %2 OFFSET %1 " ).arg( startPos ).arg( length );
}

void
QueryBuilder::shuffle( int table, Q_INT64 value )
{
    if ( !m_sort.isEmpty() ) m_sort += " ,  ";
    if ( table == 0 || value == 0 ) {
        // simple random
        m_sort += CollectionDB::instance()->randomFunc();
    } else {
        // This is the score weighted random order.

        // The RAND() function returns random values equally distributed between 0.0
        // (inclusive) and 1.0 (exclusive).  The obvious way to get this order is to
        // put every track <score> times into a list, sort the list by RAND()
        // (i.e. shuffle it) and discard every occurrence of every track but the very
        // first of each.  By putting every track into the list only once but applying
        // a transfer function T_s(x) := 1-(1-x)^(1/s) where s is the score, to RAND()
        // before sorting the list, exactly the same distribution of tracks can be
        // achieved (for a proof write to Stefan Siegel <kde@sdas.de>)

        // In the query below a simplified function is used: The score is incremented
        // by one to prevent division by zero, RAND() is used instead of 1-RAND()
        // because it doesn't matter if it becomes zero (the exponent is always
        // non-zero), and finally POWER(...) is used instead of 1-POWER(...) because it
        // only changes the order type.
        m_sort += QString("POWER( %1, 1.0 / (%2.%3 + 1) ) DESC")
            .arg( CollectionDB::instance()->randomFunc() )
            .arg( tableName( table ) )
            .arg( valueName( value ) );

        m_linkTables |= table;
    }
}


/* NOTE: It's important to keep these two functions and the const in sync! */
/* NOTE: It's just as important to keep tags.url first! */
const int
QueryBuilder::dragFieldCount = 21;

QString
QueryBuilder::dragSQLFields()
{
    return "tags.url, tags.deviceid, album.name, artist.name, composer.name, "
           "genre.name, tags.title, year.name, "
           "tags.comment, tags.track, tags.bitrate, tags.discnumber, "
           "tags.length, tags.samplerate, tags.filesize, "
           "tags.sampler, tags.filetype, tags.bpm, "
           "statistics.percentage, statistics.rating, statistics.playcounter, "
           "statistics.accessdate";
}

void
QueryBuilder::initSQLDrag()
{
    clear();
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    addReturnValue( QueryBuilder::tabComposer, QueryBuilder::valName );
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
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valIsCompilation );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFileType );
    addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBPM );
    addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
    addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    addReturnValue( QueryBuilder::tabStats, QueryBuilder::valAccessDate );
}


void
QueryBuilder::buildQuery( bool withDeviceidPlaceholder )
{
    if ( m_query.isEmpty() )
    {
        linkTables( m_linkTables );
        m_query += "SELECT ";
        m_query += m_values;
        m_query += " FROM ";
        m_query += m_tables;
        m_query += ' ';
        m_query += m_join;
        m_query += " WHERE ";
        m_query += CollectionDB::instance()->boolT();
        m_query += ' ';
        m_query += m_where;
        if ( !m_showAll && ( m_linkTables & tabSong || m_tables.contains( tableName( tabSong) ) ) )     //Only stuff on mounted devices, unless you use optShowAll
        {
            if ( withDeviceidPlaceholder )
                m_query += "(*MountedDeviceSelection*)";
            else
            {
                IdList list = MountPointManager::instance()->getMountedDeviceIds();
                //debug() << "number of device ids " << list.count() << endl;
                m_query += " AND tags.deviceid IN (";
                foreachType( IdList, list )
                {
                    if ( it != list.begin() ) m_query += ',';
                    m_query += QString::number( *it );
                }
                m_query += ')';
            }
        }
        // GROUP BY must be before ORDER BY for sqlite
        // HAVING must be between GROUP BY and ORDER BY
        if ( !m_group.isEmpty() )  { m_query += " GROUP BY "; m_query += m_group; }
        if ( !m_having.isEmpty() ) { m_query += " HAVING "; m_query += m_having; }
        if ( !m_sort.isEmpty() )   { m_query += " ORDER BY "; m_query += m_sort; }
        m_query += m_limit;
        m_query += ';';
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
    QStringList rs = CollectionDB::instance()->query( m_query );
    //calling code is unaware of the dynamic collection implementation, it simply expects an URL
    if( m_deviceidPos > 0 )
        return cleanURL( rs );
    else
        return rs;
}


void
QueryBuilder::clear()
{
    m_query.setLength(0);
    m_values.setLength(0);
    m_tables.setLength(0);
    m_join.setLength(0);
    m_where.setLength(0);
    m_sort.setLength(0);
    m_group.setLength(0);
    m_limit.setLength(0);
    m_having.setLength(0);

    m_linkTables = 0;
    m_returnValues = 0;

    m_showAll = false;
    m_deviceidPos = 0;
}


Q_INT64
QueryBuilder::valForFavoriteSorting() {
    Q_INT64 favSortBy = valRating;
    if ( !AmarokConfig::useScores() && !AmarokConfig::useRatings() )
        favSortBy = valPlayCounter;
    else if( !AmarokConfig::useRatings() )
        favSortBy = valScore;
    return favSortBy;
}

void
QueryBuilder::sortByFavorite() {
    if ( AmarokConfig::useRatings() )
        sortBy(tabStats, valRating, true );
    if ( AmarokConfig::useScores() )
        sortBy(tabStats, valScore, true );
    sortBy(tabStats, valPlayCounter, true );

}

void
QueryBuilder::sortByFavoriteAvg() {
    // Due to MySQL4 weirdness, we need to add the function we're using to sort
    // as return values as well.
    if ( AmarokConfig::useRatings() ) {
        sortByFunction(funcAvg, tabStats, valRating, true );
        addReturnFunctionValue( funcAvg, tabStats, valRating );
    }
    if ( AmarokConfig::useScores() ) {
        sortByFunction(funcAvg, tabStats, valScore, true );
        addReturnFunctionValue( funcAvg, tabStats, valScore );
    }
    sortByFunction(funcAvg, tabStats, valPlayCounter, true );
    addReturnFunctionValue( funcAvg, tabStats, valPlayCounter );

    //exclude unrated and unplayed
    if( !m_having.isEmpty() )
        m_having += " AND ";
    m_having += " (";
    if (AmarokConfig::useRatings() )
        m_having += QString("%1(%2.%3) > 0 OR ")
                   .arg( functionName( funcAvg ), tableName(tabStats), valueName(valRating) );
    m_having += QString("%1(%2.%3) > 0")
                   .arg( functionName( funcAvg ), tableName(tabStats), valueName(valPlayCounter) );
    m_having += ")";
}

// Helper method -- given a value, returns the index of the bit that is
// set, if only one, otherwise returns -1
// Binsearch seems appropriate since the values enum has 40 members
template<class ValueType>
static inline int
searchBit( ValueType value, int numBits ) {
   int low = 0, high = numBits - 1;
   while( low <= high ) {
       int mid = (low + high) / 2;
       ValueType compare = static_cast<ValueType>( 1 ) << mid;
       if ( value == compare ) return mid;
       else if ( value < compare ) high = mid - 1;
       else low = mid + 1;
   }

   return -1;
}

QString
QueryBuilder::tableName( int table )
{
    // optimize for 1 table which is by far the most frequent case
    static const char tabNames[][16] = {
        "album",
        "artist",
        "composer",
        "genre",
        "year",
        "<unused>",             // 32 is missing from the enum
        "tags",
        "statistics",
        "lyrics",
        "podcastchannels",
        "podcastepisodes",
        "podcasttables",
        "devices",
        "labels"
    };

    int oneBit = searchBit( table, sizeof( tabNames ) / sizeof( QString ) );
    if ( oneBit >= 0 ) return tabNames[oneBit];

    // slow path: multiple tables. This seems to be unneeded at the moment,
    // but leaving it here since it appears to be intended usage
    QString tables;

    if ( CollectionDB::instance()->getType() != DbConnection::postgresql )
    {
        if ( table & tabSong )   tables += ",tags";
    }
    if ( table & tabArtist ) tables += ",artist";
    if ( table & tabComposer ) tables += ",composer";
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

    if ( table & tabDevices ) tables += ",devices";
    if ( table & tabLabels ) tables += ",labels";
    // when there are multiple tables involved, we always need table tags for linking them
    return tables.mid( 1 );
}


const QString &
QueryBuilder::valueName( Q_INT64 value )
{
   static const QString values[] = {
       "id",
       "name",
       "url",
       "title",
       "track",
       "percentage",
       "comment",
       "bitrate",
       "length",
       "samplerate",
       "playcounter",
       "createdate",
       "accessdate",
       "percentage",
       "artist",
       "album",
       "year",
       "genre",
       "dir",
       "lyrics",
       "rating",
       "composer",
       "discnumber",
       "filesize",
       "filetype",
       "sampler",
       "bpm",
       "copyright",
       "parent",
       "weblink",
       "autoscan",
       "fetchtype",
       "autotransfer",
       "haspurge",
       "purgeCount",
       "isNew",
       "deviceid",
       "url",
       "label",
       "lastmountpoint",
       "type"
   };

   int oneBit = searchBit( value, sizeof( values ) / sizeof( QString ) );
   if ( oneBit >= 0 ) return values[oneBit];

   static const QString error( "<ERROR valueName>" );
   return error;
}

/*
 * Return true if we should call COALESCE(..,0) for this DB field
 * (field names sourced from the old smartplaylistbrowser.cpp code)
 * Warning: addFilter( int, Q_INT64, const QString&, int bool )
 * expects this method to return true for all statistics table clomuns of type INTEGER
 * Sqlite doesn't like comparing strings to an INTEGER column.
 */
bool
QueryBuilder::coalesceField( int table, Q_INT64 value )
{
    if( tableName( table ) == "statistics" &&
        ( valueName( value ) == "playcounter" ||
          valueName( value ) == "rating" ||
          valueName( value ) == "percentage" ||
          valueName( value ) == "accessdate" ||
          valueName( value ) == "createdate"
        )
    )
       return true;
   return false;
}

QString
QueryBuilder::functionName( int function )
{
    QString functions;

    if ( function & funcCount )     functions += "Count";
    if ( function & funcMax )       functions += "Max";
    if ( function & funcMin )       functions += "Min";
    if ( function & funcAvg )       functions += "Avg";
    if ( function & funcSum )       functions += "Sum";

    return functions;
}

// FIXME: the two functions below are inefficient, but this patch is getting too
// big already. They are not on any critical path right now. Ovy
int
QueryBuilder::getTableByName(const QString &name)
{
    for ( int i = 1; i <= tabLabels; i <<= 1 )
    {
        if (tableName(i) == name) return i;
    }
    return -1;
}

Q_INT64
QueryBuilder::getValueByName(const QString &name)
{
    for ( Q_INT64 i = 1; i <= valType; i <<= 1 ) {
        if (valueName(i) == name) return i;
    }

    return -1;
}

bool
QueryBuilder::getField(const QString &tableValue, int *table, Q_INT64 *value)
{
    int dotIndex = tableValue.find( '.' ) ;
    if ( dotIndex < 0 ) return false;
    int tmpTable = getTableByName( tableValue.left(dotIndex) );
    Q_UINT64 tmpValue = getValueByName( tableValue.mid( dotIndex + 1 ) );
    if ( tmpTable >= 0 && value ) {
        *table = tmpTable;
        *value = tmpValue;
        return true;
    }
    else
    {
        qFatal("invalid table.value: %s", tableValue.ascii());
        return false;
    }
}



QStringList
QueryBuilder::cleanURL( QStringList result )
{
    //this method replaces the fields for relative path and devive/media id with a
    //single field containing the absolute path for each row
    //TODO Max improve this code
    int count = 1;
    for( QStringList::Iterator it = result.begin(), end = result.end(); it != end; )
    {
        QString rpath;
        if ( (count % (m_returnValues + 1)) + 1== m_deviceidPos )
        {
            //this block is reached when the iterator points at the relative path
            //deviceid is next
            QString rpath = *it;
            int deviceid = (*(++it)).toInt();
            QString abspath = MountPointManager::instance()->getAbsolutePath( deviceid, rpath );
            it = result.remove(--it);
            result.insert( it, abspath );
            it = result.remove( it );
            //we advanced the iterator over two fields in this iteration
            ++count;
        }
        else
            ++it;
        ++count;
    }
    return result;
}


#include "collectiondb.moc"
