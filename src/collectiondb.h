// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// See COPYING file for licensing information.

#ifndef AMAROK_COLLECTIONDB_H
#define AMAROK_COLLECTIONDB_H

#include "engineobserver.h"

#include <qdir.h>            //stack allocated
#include <qobject.h>         //baseclass
#include <qptrqueue.h>
#include <qsemaphore.h>
#include <qstringlist.h>     //stack allocated

#ifdef USE_MYSQL
#include <qdatetime.h>
namespace mysql
{
#include <mysql.h>
}
#else
#include "sqlite/sqlite3.h"
#endif

class DbConnection;
class DbConnectionPool;
class CoverFetcher;
class MetaBundle;
class Scrobbler;


class DbConnection
{
    public:
        DbConnection();
        ~DbConnection();

#ifdef USE_MYSQL
        mysql::MYSQL* db();
#else
        sqlite3* db();
#endif
        const bool isInitialized() const { return m_initialized; }

    private:
#ifdef USE_MYSQL
        mysql::MYSQL* m_db;
#else
        sqlite3* m_db;
#endif
        bool m_initialized;

};


class DbConnectionPool : QPtrQueue<DbConnection>
{
    public:
        DbConnectionPool();
        ~DbConnectionPool();

        void createDbConnections();

        DbConnection *getDbConnection();
        void putDbConnection( const DbConnection* /* conn */ );

    private:
        static const int POOL_SIZE = 5;
        QSemaphore m_semaphore;
};


class CollectionDB : public QObject, public EngineObserver
{
    Q_OBJECT

    signals:
        void scanStarted();
        void scanDone( bool changed );

        void scoreChanged( const QString &url, int score );

        void coverFetched( const QString &artist, const QString &album );
        void coverRemoved( const QString &artist, const QString &album );
        void coverFetcherError( const QString &error );

        void similarArtistsFetched( const QString &artist );

    public:
        static CollectionDB *instance();

        static QString escapeString( QString string );

        /**
         * This method returns a static DbConnection for components that want to use
         * the same connection for the whole time. Should not be used anywhere else
         * but in CollectionReader.
         *
         * @return static DbConnection
         */
        DbConnection *getStaticDbConnection();

        /**
         * Returns the DbConnection back to connection pool.
         *
         * @param conn DbConnection to be returned
         */
        void returnStaticDbConnection( DbConnection *conn );

        //sql helper methods
        QStringList query( const QString& statement, DbConnection *conn = NULL );

        //table management methods
        bool isEmpty();
        bool isValid();
        void createTables( DbConnection *conn = NULL );
        void dropTables( DbConnection *conn = NULL );
        void clearTables( DbConnection *conn = NULL );
        void moveTempTables( DbConnection *conn );

        uint artistID( QString value, bool autocreate = true, const bool temporary = false, DbConnection *conn = NULL );
        uint albumID( QString value, bool autocreate = true, const bool temporary = false, DbConnection *conn = NULL );
        uint genreID( QString value, bool autocreate = true, const bool temporary = false, DbConnection *conn = NULL );
        uint yearID( QString value, bool autocreate = true, const bool temporary = false, DbConnection *conn = NULL );

        bool isDirInCollection( QString path );
        bool isFileInCollection( const QString &url  );
        void removeDirFromCollection( QString path );
        void removeSongsInDir( QString path );
        void updateDirStats( QString path, const long datetime, DbConnection *conn = NULL );

        //song methods
        bool addSong( MetaBundle* bundle, const bool temporary = false, DbConnection *conn = NULL );
        bool getMetaBundleForUrl( const QString& url , MetaBundle* bundle );
        void addAudioproperties( const MetaBundle& bundle );

        void updateTags( const QString &url, const MetaBundle &bundle, const bool updateView = true );
        void updateURL( const QString &url, const bool updateView = true );

        //statistics methods
        int addSongPercentage( const QString &url , const int percentage );
        int getSongPercentage( const QString &url  );
        void setSongPercentage( const QString &url , int percentage );

        //artist methods
        QStringList similarArtists( const QString &artist, uint count );

        //album methods
        void checkCompilations( const QString &path, DbConnection *conn = NULL );
        QString albumSongCount( const QString &artist_id, const QString &album_id );

        //list methods
        QStringList artistList( bool withUnknowns = true, bool withCompilations = true );
        QStringList albumList( bool withUnknowns = true, bool withCompilations = true );
        QStringList genreList( bool withUnknowns = true, bool withCompilations = true );
        QStringList yearList( bool withUnknowns = true, bool withCompilations = true );

        QStringList albumListOfArtist( const QString &artist, bool withUnknown = true, bool withCompilations = true );
        QStringList artistAlbumList( bool withUnknown = true, bool withCompilations = true );

        //cover management methods
        /** Saves images located on the user's filesystem */
        bool setAlbumImage( const QString& artist, const QString& album, const KURL& url );
        /** Saves images obtained from CoverFetcher */
        bool setAlbumImage( const QString& artist, const QString& album, QImage img, const QString& amazonUrl = QString::null );

        QString findImageByMetabundle( MetaBundle trackInformation, const uint = 1 );
        QString findImageByArtistAlbum( const QString &artist, const QString &album, const uint width = 1 );
        QString albumImage( MetaBundle trackInformation, const uint width = 1 );
        QString albumImage( const uint artist_id, const uint album_id, const uint width = 1 );
        QString albumImage( const QString &artist, const QString &album, const uint width = 1 );

        bool removeAlbumImage( const uint artist_id, const uint album_id );
        bool removeAlbumImage( const QString &artist, const QString &album );

        //local cover methods
        void addImageToAlbum( const QString& image, QValueList< QPair<QString, QString> > info, DbConnection *conn = NULL );
        QString getImageForAlbum( const QString& artist, const QString& album, uint width = 0 );
        QString notAvailCover( int width = 0 );

    protected:
        QCString md5sum( const QString& artist, const QString& album, const QString& file = QString::null );
        void engineTrackEnded( int finalPosition, int trackLength );
        /** Manages regular folder monitoring scan */
        void timerEvent( QTimerEvent* e );

    public slots:
        void fetchCover( QWidget* parent, const QString& artist, const QString& album, bool noedit );
        void scanMonitor();
        void startScan();
        void stopScan();

    private slots:
        void dirDirty( const QString& path );
        void coverFetcherResult( CoverFetcher* );
        void similarArtistsFetched( const QString& artist, const QStringList& suggestions );

    private:
        //bump DATABASE_VERSION whenever changes to the table structure are made. will remove old db file.
        static const int DATABASE_VERSION = 17;
        static const int DATABASE_STATS_VERSION = 3;
        static const int MONITOR_INTERVAL = 60; //sec
        static const bool DEBUG = false;

        CollectionDB();
        ~CollectionDB();
        //general management methods
        void createStatsTable();
        void dropStatsTable();
        void scan( const QStringList& folders, bool recursively, bool importPlaylists );
        void scanModifiedDirs( bool recursively, bool importPlaylists );

        QCString makeWidthKey( uint width );
        int sqlInsertID( DbConnection *conn );
        QString artistValue( uint id );
        QString albumValue( uint id );
        QString genreValue( uint id );
        QString yearValue( uint id );
        uint IDFromValue( QString name, QString value, bool autocreate = true, const bool temporary = false, DbConnection *conn = NULL );
        QString valueFromID( QString table, uint id );
        void customEvent( QCustomEvent* );

        //member variables
        QString m_amazonLicense;
        QString m_cacheArtist;
        uint m_cacheArtistID;
        QString m_cacheAlbum;
        uint m_cacheAlbumID;

        DbConnectionPool m_dbConnPool;

        bool m_isScanning;
        bool m_monitor;
        QDir m_cacheDir;
        QDir m_coverDir;
};


class QueryBuilder : public QObject
{
    Q_OBJECT

    public:
        //attributes:
        enum qBuilderTables  { tabAlbum = 1, tabArtist = 2, tabGenre = 4, tabYear = 8, tabSong = 32, tabStats = 64 };
        enum qBuilderOptions { optNoCompilations = 1, optOnlyCompilations = 2, optRemoveDuplicates = 4, optRandomize = 8 };
        enum qBuilderValues  { valID = 1, valName = 2, valURL = 4, valTitle = 8, valTrack = 16, valScore = 32 };
        enum qBuilderFilter  { modeNormal = 0, modeFuzzy = 1 };

        QueryBuilder();

        void addReturnValue( int table, int value );
        uint countReturnValues();

        void addFilter( int tables, const QString& filter, int mode = modeNormal );
        void excludeFilter( int tables, const QString& filter );

        void addMatch( int tables, const QString& match );
        void addMatches( int tables, const QStringList& match );
        void excludeMatch( int tables, const QString& match );

        void setOptions( int options );
        void sortBy( int table, int value, bool descending = false );
        void setLimit( int startPos, int length );

        QStringList run();
        void clear();

    private:
        void linkTables( int tables );

        QString m_values;
        QString m_tables;
        QString m_where;
        QString m_sort;
        QString m_limit;

        int m_linkTables;
        uint m_returnValues;
};


#endif /* AMAROK_COLLECTIONDB_H */
