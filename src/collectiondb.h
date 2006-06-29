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

#ifndef AMAROK_COLLECTIONDB_H
#define AMAROK_COLLECTIONDB_H

#include "engineobserver.h"
#include "threadweaver.h" //baseclass

#include <kurl.h>
#include <qdir.h>            //stack allocated
#include <qdatetime.h>
#include <qimage.h>
#include <qobject.h>         //baseclass
#include <qptrqueue.h>       //baseclass
#include <qsemaphore.h>      //stack allocated
#include <qstringlist.h>     //stack allocated
#include <qptrvector.h>
#include <qthread.h>

namespace KIO { class Job; }

class DbConnection;
class CoverFetcher;
class MetaBundle;
class OrganizeCollectionDialog;
class PodcastChannelBundle;
class PodcastEpisodeBundle;
class Scrobbler;

class QMutex;

class DbConfig
{};


class SqliteConfig : public DbConfig
{
    public:
        SqliteConfig( const QString& /* dbfile */ );

        QString dbFile() const { return m_dbfile; }

    private:
        QString m_dbfile;
};


class MySqlConfig : public DbConfig
{
    public:
        MySqlConfig(
            const QString& /* host */,
            const int /* port */,
            const QString& /* database */,
            const QString& /* username */,
            const QString& /* password */);

        QString host() const { return m_host; }
        int port() const { return m_port; }
        QString database() const { return m_database; }
        QString username() const { return m_username; }
        QString password() const { return m_password; }

    private:
        QString m_host;
        int m_port;
        QString m_database;
        QString m_username;
        QString m_password;
};


class PostgresqlConfig : public DbConfig
{
    public:
        PostgresqlConfig(
            const QString& /* host */,
            const int /* port */,
            const QString& /* database */,
            const QString& /* username */,
            const QString& /* password */);

        QString host() const { return m_host; }
        int port() const { return m_port; }
        QString database() const { return m_database; }
        QString username() const { return m_username; }
        QString password() const { return m_password; }

    private:
        QString m_host;
        int m_port;
        QString m_database;
        QString m_username;
        QString m_password;
};


class DbConnection
{
    public:
        enum DbConnectionType { sqlite = 0, mysql = 1, postgresql = 2 };

        DbConnection();
        virtual ~DbConnection() {}

        virtual QStringList query( const QString& /* statement */ ) = 0;
        virtual int insert( const QString& /* statement */, const QString& /* table */ ) = 0;
        bool isInitialized() const { return m_initialized; }
        virtual bool isConnected() const = 0;
        virtual QString lastError() const { return "None"; }

    protected:
        bool m_initialized;
};


typedef struct sqlite3 sqlite3;
typedef struct sqlite3_context sqlite3_context;
typedef struct Mem sqlite3_value;

class SqliteConnection : public DbConnection
{
    public:
        SqliteConnection( const SqliteConfig* /* config */ );
       ~SqliteConnection();

        QStringList query( const QString& /* statement */ );
        int insert( const QString& /* statement */, const QString& /* table */ );
        bool isConnected()const { return true; }
    private:
        static void sqlite_rand( sqlite3_context *context, int /*argc*/, sqlite3_value ** /*argv*/ );
        static void sqlite_power( sqlite3_context *context, int argc, sqlite3_value **argv );

        sqlite3* m_db;
};


#ifdef USE_MYSQL
typedef struct st_mysql MYSQL;

class MySqlConnection : public DbConnection
{
    public:
        MySqlConnection( const MySqlConfig* /* config */ );
       ~MySqlConnection();

        QStringList query( const QString& /* statement */ );
        int insert( const QString& /* statement */, const QString& /* table */ );
        bool isConnected()const { return m_connected; }
        QString lastError() const { return m_error; }
    private:
        void setMysqlError();
        MYSQL* m_db;
        bool m_connected;
        QString m_error;
};
#endif


#ifdef USE_POSTGRESQL
typedef struct pg_conn PGconn;

class PostgresqlConnection : public DbConnection
{
    public:
        PostgresqlConnection( const PostgresqlConfig* /* config */ );
       ~PostgresqlConnection();

        QStringList query( const QString& /* statement */ );
        int insert( const QString& /* statement */, const QString& /* table */ );
        bool isConnected()const { return m_connected; }
        QString lastError() const { return m_error; }
    private:
        void setPostgresqlError();
        PGconn* m_db;
        bool m_connected;
        QString m_error;
};
#endif


class CollectionDB : public QObject, public EngineObserver
{
    Q_OBJECT

    friend class SimilarArtistsInsertionJob;

    signals:
        void scanStarted();
        void scanDone( bool changed );
        void databaseEngineChanged();

        void scoreChanged( const QString &url, int score );
        void ratingChanged( const QString &url, int rating );
        void fileMoved( const QString &srcUrl, const QString &dstUrl );
        void fileMoved( const QString &srcUrl, const QString &dstUrl, const QString &uniqueid );
        void uniqueIdChanged( const QString &url, const QString &originalid, const QString &newid );
        void coverChanged( const QString &artist, const QString &album ); //whenever a cover changes
        void coverFetched( const QString &artist, const QString &album ); //only when fetching from amazon
        void coverRemoved( const QString &artist, const QString &album );
        void coverFetcherError( const QString &error );

        void similarArtistsFetched( const QString &artist );
        void tagsChanged( const MetaBundle &bundle );
        void imageFetched( const QString &remoteURL ); //for fetching remote podcast images

    public:
        CollectionDB();
        ~CollectionDB();

        static CollectionDB *instance();

        QString escapeString(QString string ) const
        {
            return
            #ifdef USE_MYSQL
                // We have to escape "\" for mysql, but can't do so for sqlite
                ( m_dbConnType == DbConnection::mysql )
                ? string.replace("\\", "\\\\").replace( '\'', "''" ) :
            #endif
                  string.replace( '\'', "''" );
        }

        QString boolT() const { if (getDbConnectionType() == DbConnection::postgresql) return "true"; else return "1"; }
        QString boolF() const { if (getDbConnectionType() == DbConnection::postgresql) return "false"; else return "0"; }
        QString textColumnType() const { if ( getDbConnectionType() == DbConnection::postgresql ) return "TEXT"; else return "VARCHAR(255)"; }
        QString textColumnType(int length) const { if ( getDbConnectionType() == DbConnection::postgresql ) return "TEXT"; else return QString("VARCHAR(%1)").arg(length); }
        // We might consider using LONGTEXT type, as some lyrics could be VERY long..???
        QString longTextColumnType() const { if ( getDbConnectionType() == DbConnection::postgresql ) return "TEXT"; else return "TEXT"; }
        QString randomFunc() const { if ( getDbConnectionType() == DbConnection::postgresql ) return "random()"; else return "RAND()"; }

        inline static QString exactCondition( const QString &right );
        static QString likeCondition( const QString &right, bool anyBegin=false, bool anyEnd=false );
        int getType() { return getDbConnectionType(); }

        //sql helper methods
        QStringList query( const QString& statement );
        int insert( const QString& statement, const QString& table );

        //table management methods
        bool isEmpty();
        bool isValid();
        QString adminValue( QString noption );
        void setAdminValue( QString noption, QString value );
        void createTables( const bool temporary = false );
        void createIndices(  );
        void dropTables( const bool temporary = false);
        void clearTables( const bool temporary = false);
        void copyTempTables(  );

        uint artistID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );
        uint albumID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );
        uint genreID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );
        uint yearID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );

        bool isDirInCollection( QString path );
        bool isFileInCollection( const QString &url );
        QString getURL( const MetaBundle &bundle );
        void removeDirFromCollection( QString path );
        void removeSongsInDir( QString path );
        void removeSongs( const KURL::List& urls );
        void updateDirStats( QString path, const long datetime, const bool temporary = false );

        //song methods
        bool addSong( MetaBundle* bundle, const bool incremental = false );
        void doATFStuff( MetaBundle *bundle, const bool tempTables = true );
        void newUniqueIdForFile( const QString &path );
        QString urlFromUniqueId( const QString &id );

        //podcast methods
        /// Insert a podcast channel into the database.  If @param replace is true, replace the row
        /// use updatePodcastChannel() always in preference
        bool addPodcastChannel( const PodcastChannelBundle &pcb, const bool &replace=false );
        /// Insert a podcast episode into the database.  If @param idToUpdate is provided, replace the row
        /// use updatePodcastEpisode() always in preference
        int  addPodcastEpisode( const PodcastEpisodeBundle &episode, const int idToUpdate=0 );
        int  addPodcastFolder( const QString &name, const int parent_id=0, const bool isOpen=false );
        QValueList<PodcastChannelBundle> getPodcastChannels();
        PodcastEpisodeBundle getPodcastEpisodeById( int id );
        QValueList<PodcastEpisodeBundle> getPodcastEpisodes( const KURL &parent, bool newOnly=false, int limit=-1 );
        void removePodcastChannel( const KURL &url ); // will remove all episodes too
        void removePodcastEpisode( const int id );
        void removePodcastFolder( const int id );
        void updatePodcastChannel( const PodcastChannelBundle &b );
        void updatePodcastEpisode( const int id, const PodcastEpisodeBundle &b );
        void updatePodcastFolder( const int folder_id, const QString &name, const int parent_id=0, const bool isOpen=false );
        // these return false when no bundle was available
        bool getPodcastChannelBundle( const KURL &url, PodcastChannelBundle *channel );
        bool getPodcastEpisodeBundle( const KURL &url, PodcastEpisodeBundle *channel );

        /**
         * The @p bundle parameter's url() will be looked up in the Collection
         * @param bundle this will be filled in with tags for you
         * @return true if in the collection
         */
        bool bundleForUrl( MetaBundle* bundle );
        QValueList<MetaBundle> bundlesByUrls( const KURL::List& urls );
        void addAudioproperties( const MetaBundle& bundle );

        //Helper function for updateTags
        void deleteRedundantName( const QString &table, QString ID );

        void updateTags( const QString &url, const MetaBundle &bundle, const bool updateView = true);
        void updateURL( const QString &url, const bool updateView = true );
        QString getUniqueId( const QString &url );

        //statistics methods
        void addSongPercentage( const QString &url, int percentage,
                const QString &reason, const QDateTime *playtime = 0 );
        int getSongPercentage( const QString &url );
        int getSongRating( const QString &url );
        void setSongPercentage( const QString &url, int percentage );
        void setSongRating( const QString &url, int percentage, bool toggleHalf = false );
        int getPlayCount( const QString &url );
        QDateTime getFirstPlay( const QString &url );
        QDateTime getLastPlay( const QString &url );
        void migrateFile( const QString &oldURL, const QString &newURL );
        bool moveFile( const QString &src, const QString &dest, bool overwrite, bool copy = false );
        bool organizeFile( const KURL &src, const OrganizeCollectionDialog &dialog, bool copy );

        //artist methods
        QStringList similarArtists( const QString &artist, uint count );

        //album methods
        void checkCompilations( const QString &path, const bool temporary = false );
        QStringList setCompilation( const QString &album, const bool enabled, const bool updateView = true );
        QString albumSongCount( const QString &artist_id, const QString &album_id );
        bool albumIsCompilation( const QString &album_id );
        void sanitizeCompilations();

        //list methods
        QStringList artistList( bool withUnknowns = true, bool withCompilations = true );
        QStringList albumList( bool withUnknowns = true, bool withCompilations = true );
        QStringList composerList( bool withUnknowns = true, bool withCompilations = true );
        QStringList genreList( bool withUnknowns = true, bool withCompilations = true );
        QStringList yearList( bool withUnknowns = true, bool withCompilations = true );

        QStringList albumListOfArtist( const QString &artist, bool withUnknown = true, bool withCompilations = true );
        QStringList artistAlbumList( bool withUnknown = true, bool withCompilations = true );

        QStringList albumTracks( const QString &artist_id, const QString &album_id, const bool isValue = false );
        QStringList albumDiscTracks( const QString &artist_id, const QString &album_id, const QString &discNumber );
        QStringList artistTracks( const QString &artist_id );

        //cover management methods
        /** Returns the image from a given URL, network-transparently.
         * You must run KIO::NetAccess::removeTempFile( tmpFile ) when you are finished using the image;
         **/
        static QImage fetchImage( const KURL& url, QString &tmpFile );
        /** Saves images located on the user's filesystem */
        bool setAlbumImage( const QString& artist, const QString& album, const KURL& url );
        /** Saves images obtained from CoverFetcher */
        bool setAlbumImage( const QString& artist, const QString& album, QImage img, const QString& amazonUrl = QString::null, const QString& asin = QString::null );

        QString findAmazonImage( const QString &artist, const QString &album, const uint width = 1 );
        QString findDirectoryImage( const QString& artist, const QString& album, uint width = 0 );
        QString findEmbeddedImage( const QString& artist, const QString& album, uint width = 1 );
        QString findMetaBundleImage( MetaBundle trackInformation, const uint = 1 );

        /// ensure the sql only return urls to tracks for efficiency
        static QPixmap createDragPixmapFromSQL( const QString &sql, QString textOverRide=QString::null );
        static QPixmap createDragPixmap( const KURL::List &urls, QString textOverRide=QString::null );
        static const int DRAGPIXMAP_OFFSET_X = -12;
        static const int DRAGPIXMAP_OFFSET_Y = -28;

        /*
         * Retrieves the path to the local copy of the image pointed to by url,
         * initiates fetching of the remote image if necessary.
         * @param width the size of the image. 0 == full size, 1 == preview size
         */
        QString podcastImage( const MetaBundle &bundle, const bool withShadow = false, uint width = 1 );
        QString podcastImage( const QString &remoteURL, const bool withShadow = false, uint width = 1 );

        /**
         * Retrieves the path to the image for the album of the requested item
         * @param width the size of the image. 0 == full size, 1 == preview size
         * @param embedded if not NULL, sets a bool indicating whether the path is an embedded image
         */
        QString albumImage( MetaBundle trackInformation, const bool withShadow = false, uint width = 1, bool* embedded = 0 );
        QString albumImage( const uint artist_id, const uint album_id, const bool withShadow = false, uint width = 1, bool* embedded = 0 );
        QString albumImage( const QString &artist, const QString &album, const bool withShadow = false, uint width = 1, bool* embedded = 0 );

        bool removeAlbumImage( const uint artist_id, const uint album_id );
        bool removeAlbumImage( const QString &artist, const QString &album );

        static QString makeShadowedImage( const QString& albumImage, bool cache = true );

        //local cover methods
        void addImageToAlbum( const QString& image, QValueList< QPair<QString, QString> > info, const bool temporary );
        QString notAvailCover( const bool withShadow = false, int width = 1 );

        //embedded cover methods
        void addEmbeddedImage( const QString& path, const QString& hash, const QString& description );
        void removeOrphanedEmbeddedImages();

        void applySettings();

        void setLyrics( const QString& url, const QString& lyrics );
        QString getLyrics( const QString& url );

        /** Remove from the amazon table the item with the specified md5sum **/
        void removeInvalidAmazonInfo( const QString& md5sum );
        void newAmazonReloadDate( const QString& asin, const QString& locale, const QString& md5sum );
        QStringList staleImages();

        DbConnection::DbConnectionType getDbConnectionType() const { return m_dbConnType; }
        bool isConnected();
        void releasePreviousConnection(QThread *currThread);

        void invalidateArtistAlbumCache() { m_validArtistCache=0; m_validAlbumCache=0; };

    protected:
        QCString md5sum( const QString& artist, const QString& album, const QString& file = QString::null );
        void engineTrackEnded( int finalPosition, int trackLength, const QString &reason );
        /** Manages regular folder monitoring scan */
        void timerEvent( QTimerEvent* e );

    public slots:
        void fetchCover( QWidget* parent, const QString& artist, const QString& album, bool noedit );
        void scanMonitor();
        void startScan();
        void stopScan();
        void scanModifiedDirs();
        void disableAutoScoring( bool disable = true ) { m_autoScoring = !disable; }

    private slots:
        void dirDirty( const QString& path );
        void coverFetcherResult( CoverFetcher* );
        void similarArtistsFetched( const QString& artist, const QStringList& suggestions );
        void fileOperationResult( KIO::Job *job ); // moveFile depends on it
        void podcastImageResult( KIO::Job *job ); //for fetching remote podcast images
        void atfMigrateStatisticsUrl( const QString& oldUrl, const QString& newUrl, const QString& uniqueid ); //ATF-enable stats
        void atfMigrateStatisticsUniqueId( const QString& url, const QString& oldid, const QString& newid );

    private:
        //bump DATABASE_VERSION whenever changes to the table structure are made.
        // This erases tags, album, artist, genre, year, images, embed, directory and related_artists tables.
        static const int DATABASE_VERSION = 28;
        // Persistent Tables hold data that is somehow valuable to the user, and can't be erased when rescaning.
        // When bumping this, write code to convert the data!
        static const int DATABASE_PERSISTENT_TABLES_VERSION = 12;
        // Bumping this erases stats table. If you ever need to, write code to convert the data!
        static const int DATABASE_STATS_VERSION = 5;
        // When bumping this, you should provide code to convert the data.
        static const int DATABASE_PODCAST_TABLES_VERSION = 1;
        static const int DATABASE_ATF_VERSION = 1;

        static const int MONITOR_INTERVAL = 60; //sec
        static const bool DEBUG = false;

        static QDir largeCoverDir();
        static QDir tagCoverDir();
        static QDir cacheCoverDir();

        void initialize();
        void destroy();
        DbConnection* getMyConnection();

        void customEvent( QCustomEvent * );

        // helpers for embedded images
        QString loadHashFile( const QCString& hash, uint width );
        bool extractEmbeddedImage( MetaBundle &trackInformation, QCString& hash );

        //general management methods
        void createStatsTable();
        void dropStatsTable();
        void createPersistentTables();
        void dropPersistentTables();
        void createPodcastTables();
        void dropPodcastTables();

        QCString makeWidthKey( uint width );
        QString artistValue( uint id );
        QString albumValue( uint id );
        QString genreValue( uint id );
        QString yearValue( uint id );

        //These should be avoided as they will be slow and potentially unsafe.
        //Use the Exact version where possible (faster and safer).
        //To convert output from Exact version from QString to uint, use .toUInt()
        uint IDFromValue( QString name, QString value, bool autocreate = true, const bool temporary = false );
        QString IDfromExactValue(  QString table, QString value, bool autocreate = true, bool temporary = false );
        QString valueFromID( QString table, uint id );

        //member variables
        QString m_amazonLicense;
        bool    m_validArtistCache;
        bool    m_validAlbumCache;
        QString m_cacheArtist[2];
        uint    m_cacheArtistID[2];
        QString m_cacheAlbum[2];
        uint    m_cacheAlbumID[2];

        bool m_monitor;
        bool m_autoScoring;

        static QMutex *connectionMutex;
        QImage m_noCover;

        static QMap<QThread *, DbConnection *> *threadConnections;
        DbConnection::DbConnectionType m_dbConnType;
        DbConfig *m_dbConfig;

        //organize files stuff
        bool m_waitForFileOperation;
        bool m_fileOperationFailed;
        bool m_atfEnabled;
        bool m_scanInProgress;
        bool m_rescanRequired;

        // for handling podcast image url redirects
        QMap<KIO::Job *, QString> m_podcastImageJobs;
};


class INotify : public ThreadWeaver::DependentJob
{
    Q_OBJECT

    public:
        INotify( CollectionDB *parent, int fd );
        ~INotify();

        static INotify *instance() { return s_instance; }

        bool watchDir( QString directory );
        int fd() { return m_fd; }

    private:
        virtual bool doJob();

        CollectionDB* m_parent;
        int m_fd;

        static INotify* s_instance;
};


class QueryBuilder
{
    public:
        //attributes:
        enum qBuilderTables  { tabAlbum = 1, tabArtist = 2, tabGenre = 4, tabYear = 8, tabSong = 32,
                               tabStats = 64, tabLyrics = 128, tabPodcastChannels = 256,
                               tabPodcastEpisodes = 512, tabPodcastFolders = 1024,
                               tabComposer = 2048 /* dummy table for filtering */, tabDummy = 0 };
        enum qBuilderOptions { optNoCompilations = 1, optOnlyCompilations = 2, optRemoveDuplicates = 4,
                               optRandomize = 8 };
        /* This has been an enum in the past, but 32 bits wasn't enough anymore :-( */
        static const Q_INT64 valDummy         = 0;
        static const Q_INT64 valID            = 1 << 0;
        static const Q_INT64 valName          = 1 << 1;
        static const Q_INT64 valURL           = 1 << 2;
        static const Q_INT64 valTitle         = 1 << 3;
        static const Q_INT64 valTrack         = 1 << 4;
        static const Q_INT64 valScore         = 1 << 5;
        static const Q_INT64 valComment       = 1 << 6;
        static const Q_INT64 valBitrate       = 1 << 7;
        static const Q_INT64 valLength        = 1 << 8;
        static const Q_INT64 valSamplerate    = 1 << 9;
        static const Q_INT64 valPlayCounter   = 1 << 10;
        static const Q_INT64 valCreateDate    = 1 << 11;
        static const Q_INT64 valAccessDate    = 1 << 12;
        static const Q_INT64 valPercentage    = 1 << 13;
        static const Q_INT64 valArtistID      = 1 << 14;
        static const Q_INT64 valAlbumID       = 1 << 15;
        static const Q_INT64 valYearID        = 1 << 16;
        static const Q_INT64 valGenreID       = 1 << 17;
        static const Q_INT64 valDirectory     = 1 << 18;
        static const Q_INT64 valLyrics        = 1 << 19;
        static const Q_INT64 valRating        = 1 << 20;
        static const Q_INT64 valComposer      = 1 << 21;
        static const Q_INT64 valDiscNumber    = 1 << 22;
        static const Q_INT64 valFilesize      = 1 << 23;
        static const Q_INT64 valFileType      = 1 << 24;
        static const Q_INT64 valIsCompilation = 1 << 25;
        // podcast relevant:
        static const Q_INT64 valCopyright     = 1 << 26;
        static const Q_INT64 valParent        = 1 << 27;
        static const Q_INT64 valWeblink       = 1 << 28;
        static const Q_INT64 valAutoscan      = 1 << 29;
        static const Q_INT64 valFetchtype     = 1 << 30;
        static const Q_INT64 valAutotransfer  = 1LL << 31;
        static const Q_INT64 valPurge         = 1LL << 32;
        static const Q_INT64 valPurgeCount    = 1LL << 33;
        static const Q_INT64 valIsNew         = 1LL << 34;

        enum qBuilderFunctions  { funcNone = 0, funcCount = 1, funcMax = 2, funcMin = 4, funcAvg = 8, funcSum = 16 };

        enum qBuilderFilter  { modeNormal = 0, modeLess = 1, modeGreater = 2, modeEndMatch = 3 };

        QueryBuilder();

        void addReturnValue( int table, Q_INT64 value, bool caseSensitive = false );
        void addReturnFunctionValue( int function, int table, Q_INT64 value);
        uint countReturnValues();

        void beginOR(); //filters will be ORed instead of ANDed
        void endOR();   //don't forget to end it!

        void setGoogleFilter( int defaultTables, QString query );

        void addURLFilters( const QStringList& filter );

        void addFilter( int tables, const QString& filter);
        void addFilter( int tables, Q_INT64 value, const QString& filter, int mode = modeNormal, bool exact = false );
        void addFilters( int tables, const QStringList& filter );
        void excludeFilter( int tables, const QString& filter );
        void excludeFilter( int tables, Q_INT64 value, const QString& filter, int mode = modeNormal, bool exact = false );

        void addMatch( int tables, const QString& match, bool interpretUnknown = true, bool caseSensitive = false );
        void addMatch( int tables, Q_INT64 value, const QString& match );
        void addMatches( int tables, const QStringList& match, bool interpretUnknown = true, bool caseSensitive = false );
        void excludeMatch( int tables, const QString& match );
        void having( int table, Q_INT64 value, int function, int mode, const QString& match );

        void exclusiveFilter( int tableMatching, int tableNotMatching, Q_INT64 value );

        void setOptions( int options );
        void sortBy( int table, Q_INT64 value, bool descending = false );
        void sortByFunction( int function, int table, Q_INT64 value, bool descending = false );
        void groupBy( int table, Q_INT64 value );
        void setLimit( int startPos, int length );

        static const int dragFieldCount;
        static QString dragSQLFields();
        void initSQLDrag();

        void buildQuery();
        QString getQuery();
        QString query() { buildQuery(); return m_query; };
        void clear();

        QStringList run();

    private:
        QString tableName( int table );
        QString valueName( Q_INT64 value );
        QString functionName( int functions );

        void linkTables( int tables );

        bool m_OR;
        QString ANDslashOR() const;

        QString m_query;
        QString m_values;
        QString m_tables;
        QString m_join;
        QString m_where;
        QString m_sort;
        QString m_group;
        QString m_limit;
        QString m_having;

        int m_linkTables;
        uint m_returnValues;
};

inline void QueryBuilder::beginOR()
{
    m_OR = true;
    m_where += "AND ( " + CollectionDB::instance()->boolF() + " ";
}
inline void QueryBuilder::endOR()
{
    m_OR = false;
    m_where += " ) ";
}
inline QString QueryBuilder::ANDslashOR() const { return m_OR ? "OR" : "AND"; }


#endif /* AMAROK_COLLECTIONDB_H */
