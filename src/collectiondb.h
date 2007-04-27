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
#include "threadmanager.h" //baseclass
#include "amarok_export.h"

#include <kurl.h>
#include <QDir>            //stack allocated
#include <QDateTime>
#include <QImage>
#include <QMutex>
#include <QObject>         //baseclass
#include <q3ptrqueue.h>       //baseclass
#include <q3semaphore.h>      //stack allocated
#include <QStringList>     //stack allocated
#include <q3ptrvector.h>
#include <QThread>
#include <q3valuestack.h>
//Added by qt3to4:
#include <QTimerEvent>
#include <QPixmap>
#include <Q3ValueList>
#include <QCustomEvent>
#include <QByteArray>

namespace KIO { class Job; }

class DbConnection;
class CoverFetcher;
class MetaBundle;
class OrganizeCollectionDialog;
class PodcastChannelBundle;
class PodcastEpisodeBundle;
class Q3ListViewItem;
class Scrobbler;

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


class AMAROK_EXPORT DbConnection
{
    public:
        enum DbConnectionType { sqlite = 0, mysql = 1, postgresql = 2 };

        DbConnection();
        virtual ~DbConnection() {}

        virtual QStringList query( const QString& /* statement */, bool suppressDebug ) = 0;
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

        QStringList query( const QString& /* statement */, bool suppressDebug = false );
        int insert( const QString& /* statement */, const QString& /* table */ );
        bool isConnected()const { return true; }
    private:
        static void sqlite_rand( sqlite3_context *context, int /*argc*/, sqlite3_value ** /*argv*/ );
        static void sqlite_power( sqlite3_context *context, int argc, sqlite3_value **argv );
        static void sqlite_like_new( sqlite3_context *context, int argc, sqlite3_value **argv );

        sqlite3* m_db;
};


#ifdef USE_MYSQL
typedef struct st_mysql MYSQL;

class MySqlConnection : public DbConnection
{
    public:
        MySqlConnection( const MySqlConfig* /* config */ );
       ~MySqlConnection();

        QStringList query( const QString& /* statement */, bool suppressDebug = false );
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

        QStringList query( const QString& /* statement */, bool suppressDebug = false );
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

        void databaseUpdateDone();

        void scoreChanged( const QString &url, float score );
        void ratingChanged( const QString &url, int rating );
        void labelsChanged( const QString &url );
        void fileMoved( const QString &srcUrl, const QString &dstUrl );
        void fileMoved( const QString &srcUrl, const QString &dstUrl, const QString &uniqueid );
        void fileDeleted( const QString &absPath );
        void fileDeleted( const QString &absPath, const QString &uniqueid );
        void fileAdded( const QString &absPath );
        void fileAdded( const QString &absPath, const QString &uniqueid );
        void filesAdded( const QMap<QString,QString> &map );
        void uniqueIdChanged( const QString &url, const QString &originalid, const QString &newid );
        void coverChanged( const QString &artist, const QString &album ); //whenever a cover changes
        void coverFetched( const QString &artist, const QString &album ); //only when fetching from amazon
        void coverRemoved( const QString &artist, const QString &album );
        void coverFetcherError( const QString &error );

        void similarArtistsFetched( const QString &artist );
        void tagsChanged( const MetaBundle &bundle );
        void tagsChanged( const QString &oldArtist, const QString &oldAlbum );
        void imageFetched( const QString &remoteURL ); //for fetching remote podcast images

    public:
        CollectionDB();
        ~CollectionDB();

        AMAROK_EXPORT static CollectionDB *instance();

        /**
         * performs all initializations which require directory or URL data stored in the
         * database.
         */
        void initDirOperations();

        enum labelTypes { typeUser = 1 };           //add new types add the end!

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
        //textColumnType should be used for normal strings, which need to be compared
        //either case-sensitively or -insensitively
        QString textColumnType( int length=255 ) const { if ( getDbConnectionType() == DbConnection::postgresql ) return "TEXT"; else return QString("VARCHAR(%1)").arg(length); }
        //exactTextColumnType should be used for strings that must be stored exactly, such
        //as URLs (necessary for holding control chars etc. if present in URL), except for
        //trailing spaces. Comparisions should always be done case-sensitively.
        //As we create indices on these columns, we have to restrict them to
        //<= 255 chars for mysql < 5.0.3
        QString exactTextColumnType( int length=1024 ) const { if ( getDbConnectionType() == DbConnection::mysql ) return QString( "VARBINARY(%1)" ).arg( length>255 ? 255 : length ); else return textColumnType( length ); }
        // We might consider using LONGTEXT type, as some lyrics could be VERY long..???
        QString longTextColumnType() const { if ( getDbConnectionType() == DbConnection::postgresql ) return "TEXT"; else return "TEXT"; }
        QString randomFunc() const { if ( getDbConnectionType() == DbConnection::postgresql ) return "random()"; else return "RAND()"; }

        static QString exactCondition( const QString &right );
        static QString likeCondition( const QString &right, bool anyBegin=false, bool anyEnd=false );
        int getType() { return getDbConnectionType(); }

        //sql helper methods
        AMAROK_EXPORT QStringList query( const QString& statement, bool suppressDebug = false );
        AMAROK_EXPORT int insert( const QString& statement, const QString& table );

        /**
         * TODO: write doc
         * @param showAll
         * @return a string which can be appended to an existing sql where statement
         */
        QString deviceidSelection( const bool showAll = false );

        /**
         * converts the result of a query which contains a deviceid and a relative path
         * to a list of absolute paths. the order of entries in each result row must be
         * deviceid first, relative path second.
         * @param result the result of the sql query, deviceid first, relative path second
         * @return a list of urls
         */
        QStringList URLsFromQuery( const QStringList &result ) const;

        /**
         * converts the result list of a amarok-sql query to a list of urls
         */
        KUrl::List URLsFromSqlDrag( const QStringList &values ) const;

        //table management methods
        bool isEmpty();
        bool isValid();
        QString adminValue( QString noption );
        void setAdminValue( QString noption, QString value );
        void createTables( const bool temporary = false );
        void createIndices(  );
        void createPermanentIndices();
        void dropTables( const bool temporary = false);
        void clearTables( const bool temporary = false);
        void copyTempTables(  );
        void prepareTempTables();

        uint artistID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );
        uint composerID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );
        uint albumID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );
        uint genreID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );
        uint yearID( QString value, bool autocreate = true, const bool temporary = false, bool exact = true );

        bool isDirInCollection( QString path );
        bool isFileInCollection( const QString &url );
        QString getURL( const MetaBundle &bundle );
        void removeDirFromCollection( QString path );
        void removeSongsInDir( QString path, QMap<QString,QString> *tagsRemoved = 0 );
        void removeSongs( const KUrl::List& urls );
        void updateDirStats( QString path, const long datetime, const bool temporary = false );

        //song methods
        bool addSong( MetaBundle* bundle, const bool incremental = false );
        void aftCheckPermanentTables( const QString &currdeviceid, const QString &currid, const QString &currurl );
        void doAFTStuff( MetaBundle *bundle, const bool tempTables = true );
        void emitFileAdded( const QString &absPath,
                            const QString &uniqueid = QString() );
        void emitFilesAdded( const QMap<QString,QString> &map ) { emit filesAdded( map ); }
        void emitFileDeleted( const QString &absPath,
                              const QString &uniqueid = QString() );
        bool newUniqueIdForFile( const QString &path );
        bool removeUniqueIdFromFile( const QString &path );
        QString urlFromUniqueId( const QString &id );
        QString uniqueIdFromUrl( const KUrl &url );

        //podcast methods
        /// Insert a podcast channel into the database.  If @param replace is true, replace the row
        /// use updatePodcastChannel() always in preference
        bool addPodcastChannel( const PodcastChannelBundle &pcb, const bool &replace=false );
        /// Insert a podcast episode into the database.  If @param idToUpdate is provided, replace the row
        /// use updatePodcastEpisode() always in preference
        int  addPodcastEpisode( const PodcastEpisodeBundle &episode, const int idToUpdate=0 );
        int  addPodcastFolder( const QString &name, const int parent_id=0, const bool isOpen=false );
        Q3ValueList<PodcastChannelBundle> getPodcastChannels();
        PodcastEpisodeBundle getPodcastEpisodeById( int id );
        Q3ValueList<PodcastEpisodeBundle> getPodcastEpisodes( const KUrl &parent, bool newOnly=false, int limit=-1 );
        void removePodcastChannel( const KUrl &url ); // will remove all episodes too
        void removePodcastEpisode( const int id );
        void removePodcastFolder( const int id );
        void updatePodcastChannel( const PodcastChannelBundle &b );
        void updatePodcastEpisode( const int id, const PodcastEpisodeBundle &b );
        void updatePodcastFolder( const int folder_id, const QString &name, const int parent_id=0, const bool isOpen=false );
        // these return false when no bundle was available
        bool getPodcastChannelBundle( const KUrl &url, PodcastChannelBundle *channel );
        bool getPodcastEpisodeBundle( const KUrl &url, PodcastEpisodeBundle *channel );

        MetaBundle bundleFromQuery( QStringList::const_iterator *iter );
        /**
         * The @p bundle parameter's url() will be looked up in the Collection
         * @param bundle this will be filled in with tags for you
         * @return true if in the collection
         */
        bool bundleForUrl( MetaBundle* bundle );
        Q3ValueList<MetaBundle> bundlesByUrls( const KUrl::List& urls );
        void addAudioproperties( const MetaBundle& bundle );

        //Helper function for updateTags
        void deleteRedundantName( const QString &table, const QString &id );

        void deleteAllRedundant( const QString &table );

        void updateTags( const QString &url, const MetaBundle &bundle, const bool updateView = true);
        void updateURL( const QString &url, const bool updateView = true );
        QString getUniqueId( const QString &url );

        //statistics methods
        void addSongPercentage( const QString &url, float percentage,
                const QString &reason, const QDateTime *playtime = 0 );
        float getSongPercentage( const QString &url );
        int getSongRating( const QString &url );
        void setSongPercentage( const QString &url, float percentage );
        void setSongRating( const QString &url, int percentage, bool toggleHalf = false );
        int getPlayCount( const QString &url );
        QDateTime getFirstPlay( const QString &url );
        QDateTime getLastPlay( const QString &url );
        void migrateFile( const QString &oldURL, const QString &newURL );
        bool moveFile( const QString &src, const QString &dest, bool overwrite, bool copy = false );
        bool organizeFile( const KUrl &src, const OrganizeCollectionDialog &dialog, bool copy );

        //artist methods
        QStringList similarArtists( const QString &artist, uint count );

        //album methods
        void checkCompilations( const QString &path, const bool temporary = false );
        void setCompilation( const KUrl::List &urls, bool enabled, bool updateView );
        QString albumSongCount( const QString &artist_id, const QString &album_id );
        bool albumIsCompilation( const QString &album_id );
        void sanitizeCompilations();

        //label methods
        QStringList getLabels( const QString &url, const uint type );
        void removeLabels( const QString &url, const QStringList &labels, const uint type );
        bool addLabel( const QString &url, const QString &label, const QString &uid, const uint type );
        void setLabels( const QString &url, const QStringList &labels, const QString &uid, const uint type );

        void cleanLabels();

        QStringList favoriteLabels( int type = CollectionDB::typeUser, int count = 10 );

        //list methods
        QStringList artistList( bool withUnknowns = true, bool withCompilations = true );
        QStringList composerList( bool withUnknowns = true, bool withCompilations = true );
        QStringList albumList( bool withUnknowns = true, bool withCompilations = true );
        QStringList genreList( bool withUnknowns = true, bool withCompilations = true );
        QStringList yearList( bool withUnknowns = true, bool withCompilations = true );
        QStringList labelList();

        QStringList albumListOfArtist( const QString &artist, bool withUnknown = true, bool withCompilations = true );
        QStringList artistAlbumList( bool withUnknown = true, bool withCompilations = true );

        QStringList albumTracks( const QString &artist_id, const QString &album_id );
        QStringList albumDiscTracks( const QString &artist_id, const QString &album_id, const QString &discNumber );
        QStringList artistTracks( const QString &artist_id );

        //cover management methods
        /** Returns the image from a given URL, network-transparently.
         * You must run KIO::NetAccess::removeTempFile( tmpFile ) when you are finished using the image;
         **/
        static QImage fetchImage( const KUrl& url, QString &tmpFile );
        /** Saves images located on the user's filesystem */
        bool setAlbumImage( const QString& artist, const QString& album, const KUrl& url );
        /** Saves images obtained from CoverFetcher */
        bool setAlbumImage( const QString& artist, const QString& album, QImage img, const QString& amazonUrl = QString(), const QString& asin = QString() );

        QString findAmazonImage( const QString &artist, const QString &album, const uint width = 1 );
        QString findDirectoryImage( const QString& artist, const QString& album, uint width = 0 );
        QString findEmbeddedImage( const QString& artist, const QString& album, uint width = 1 );
        QString findMetaBundleImage( const MetaBundle &trackInformation, const uint = 1 );

        /// ensure the sql only return urls to tracks for efficiency
        static QPixmap createDragPixmapFromSQL( const QString &sql, QString textOverRide=QString() );
        static QPixmap createDragPixmap( const KUrl::List &urls, QString textOverRide=QString() );
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
        QString albumImage( const MetaBundle &trackInformation, const bool withShadow = false, uint width = 1, bool* embedded = 0 );
        QString albumImage( const uint artist_id, const uint album_id, const bool withShadow = false, uint width = 1, bool* embedded = 0 );
        QString albumImage( const QString &artist, const QString &album, const bool withShadow = false, uint width = 1, bool* embedded = 0 );
        QMap<Q3ListViewItem*, CoverFetcher*> * getItemCoverMap() { return itemCoverMap; }
        QMutex * getItemCoverMapMutex() { return itemCoverMapMutex; }

        bool removeAlbumImage( const uint artist_id, const uint album_id );
        bool removeAlbumImage( const QString &artist, const QString &album );

        static QString makeShadowedImage( const QString& albumImage, bool cache = true );

        //local cover methods
        void addImageToAlbum( const QString& image, Q3ValueList< QPair<QString, QString> > info, const bool temporary );
        QString notAvailCover( const bool withShadow = false, int width = 1 );

        //embedded cover methods
        void addEmbeddedImage( const QString& path, const QString& hash, const QString& description );
        void removeOrphanedEmbeddedImages();

        void applySettings();

        void setLyrics( const QString& url, const QString& lyrics, const QString &uniqueid = QString() );
        QString getLyrics( const QString& url );

        /** Remove from the amazon table the item with the specified md5sum **/
        void removeInvalidAmazonInfo( const QString& md5sum );
        void newAmazonReloadDate( const QString& asin, const QString& locale, const QString& md5sum );
        QStringList staleImages();

        AMAROK_EXPORT DbConnection::DbConnectionType getDbConnectionType() const { return m_dbConnType; }
        bool isConnected();
        void releasePreviousConnection(QThread *currThread);

        void invalidateArtistAlbumCache() { m_validArtistCache=false; m_validComposerCache=false; m_validAlbumCache=false; };

        void vacuum();

        /**
        * Cancel the underlying move/copy file action
        */
        void cancelMovingFileJob();

    protected:
        QByteArray md5sum( const QString& artist, const QString& album, const QString& file = QString() );
        void engineTrackEnded( int finalPosition, int trackLength, const QString &reason );
        /** Manages regular folder monitoring scan */
        void timerEvent( QTimerEvent* e );

    public slots:
        void fetchCover( QWidget* parent, const QString& artist, const QString& album, bool noedit, Q3ListViewItem* item = 0 );
        void scanMonitor();
        void startScan();
        void stopScan();
        void scanModifiedDirs();
        void disableAutoScoring( bool disable = true ) { m_autoScoring = !disable; }

        void checkDatabase();

    private slots:
        void dirDirty( const QString& path );
        void coverFetcherResult( CoverFetcher* );
        void similarArtistsFetched( const QString& artist, const QStringList& suggestions );
        void fileOperationResult( KIO::Job *job ); // moveFile depends on it
        void podcastImageResult( KIO::Job *job ); //for fetching remote podcast images
        void aftMigratePermanentTablesUrl( const QString& oldUrl, const QString& newUrl, const QString& uniqueid ); //AFT-enable stats
        void aftMigratePermanentTablesUniqueId( const QString& url, const QString& oldid, const QString& newid );

    private:
        //bump DATABASE_VERSION whenever changes to the table structure are made.
        // This erases tags, album, artist, composer, genre, year, images, embed, directory and related_artists tables.
        static const int DATABASE_VERSION;
        // Persistent Tables hold data that is somehow valuable to the user, and can't be erased when rescaning.
        // When bumping this, write code to convert the data!
        static const int DATABASE_PERSISTENT_TABLES_VERSION;
        // Bumping this erases stats table. If you ever need to, write code to convert the data!
        static const int DATABASE_STATS_VERSION;
        // When bumping this, you should provide code to convert the data.
        static const int DATABASE_PODCAST_TABLES_VERSION;
        static const int DATABASE_AFT_VERSION;
        // persistent table. you should provide code to convert the data when bumping this
        static const int DATABASE_DEVICES_VERSION;

        static const int MONITOR_INTERVAL; //sec

        static QDir largeCoverDir();
        static QDir tagCoverDir();
        static QDir cacheCoverDir();

        void initialize();
        void destroy();
        DbConnection* getMyConnection();

        //helper methods which perform updates of amarok's database
        void updateStatsTables();
        void updatePersistentTables();
        void updatePodcastTables();

        //A dirty hack to preserve Group By settings in Collection Browser after addition
        //of Composer table
        void updateGroupBy();

        void customEvent( QEvent * );

        // helpers for embedded images
        QString loadHashFile( const QByteArray& hash, uint width );
        bool extractEmbeddedImage( const MetaBundle &trackInformation, QByteArray& hash );

        //general management methods
        void createStatsTable();
        void dropStatsTable();
        void createPersistentTables();
        void dropPersistentTables();
        void createPodcastTables();
        void dropPodcastTables();
        void createDevicesTable();
        void dropDevicesTable();

        //Archived forms of the above. useful for providing a linear upgrade routine that
        //stays the same
        void createStatsTableV8();
        void createStatsTableV10( bool temp );
        void dropStatsTableV1();
        void createPersistentTablesV12();
        void createPersistentTablesV14( bool temp );
        void dropPersistentTablesV14();
        void createPodcastTablesV2( bool temp );
        void dropPodcastTablesV2();


        QByteArray makeWidthKey( uint width );
        QString artistValue( uint id );
        QString composerValue( uint id );
        QString albumValue( uint id );
        QString genreValue( uint id );
        QString yearValue( uint id );

        //These should be avoided as they will be slow and potentially unsafe.
        //Use the Exact version where possible (faster and safer).
        //To convert output from Exact version from QString to uint, use .toUInt()
        uint IDFromValue( QString name, QString value, bool autocreate = true, const bool temporary = false );
        QString IDFromExactValue(  QString table, QString value, bool autocreate = true, bool temporary = false );
        QString valueFromID( QString table, uint id );

        //member variables
        QString m_amazonLicense;
        bool    m_validArtistCache;
        bool    m_validComposerCache;
        bool    m_validAlbumCache;
        QString m_cacheArtist[2];
        uint    m_cacheArtistID[2];
        QString m_cacheComposer[2];
        uint    m_cacheComposerID[2];
        QString m_cacheAlbum[2];
        uint    m_cacheAlbumID[2];

        bool m_monitor;
        bool m_autoScoring;

        static QMap<Q3ListViewItem*, CoverFetcher*> *itemCoverMap;
        static QMutex *itemCoverMapMutex;
        QImage m_noCover;

        static QMap<QThread *, DbConnection *> *threadConnections;
        static QMutex *connectionMutex;
        DbConnection::DbConnectionType m_dbConnType;
        DbConfig *m_dbConfig;

        //organize files stuff
        bool m_waitForFileOperation;
        bool m_fileOperationFailed;
        bool m_scanInProgress;
        bool m_rescanRequired;

        QStringList m_aftEnabledPersistentTables;

        // Cancel move/copy job
        bool m_moveFileJobCancelled;

        // for handling podcast image url redirects
        QMap<KIO::Job *, QString> m_podcastImageJobs;

        // protect against multiple simultaneous queries/inserts
        QMutex m_mutex;
};

class INotify : public ThreadManager::DependentJob
{
    Q_OBJECT

    public:
        INotify( CollectionDB *parent, int fd );
        ~INotify();

        static INotify *instance() { return s_instance; }

        bool watchDir( const QString directory );
        int fd() { return m_fd; }

    private:
        virtual bool doJob();

        CollectionDB* m_parent;
        int m_fd;

        static INotify* s_instance;
};

#endif /* AMAROK_COLLECTIONDB_H */
