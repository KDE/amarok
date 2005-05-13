// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// See COPYING file for licensing information.

#ifndef AMAROK_COLLECTIONDB_H
#define AMAROK_COLLECTIONDB_H

#include "engineobserver.h"
#include "dbenginebase.h"
#include <kurl.h>
#include <qdir.h>            //stack allocated
#include <qobject.h>         //baseclass
#include <qptrqueue.h>       //baseclass
#include <qsemaphore.h>      //stack allocated
#include <qstringlist.h>     //stack allocated

class CoverFetcher;
class MetaBundle;
class Scrobbler;


class DbConnectionPool : QPtrQueue<DbConnection>
{
    public:
        DbConnectionPool();
       ~DbConnectionPool();

        const DbConnection::DbConnectionType getDbConnectionType() const { return m_dbConnType; }
        const DbConfig *getDbConfig() const { return m_dbConfig; }
        void createDbConnections();

        DbConnection *getDbConnection();
        void putDbConnection( const DbConnection* /* conn */ );

    private:
        static const int POOL_SIZE = 5;
        QSemaphore m_semaphore;
        DbConnection::DbConnectionType m_dbConnType;
        DbConfig *m_dbConfig;
};


class CollectionDB : public QObject, public EngineObserver
{
    Q_OBJECT

    friend class SimilarArtistsInsertionJob;

    signals:
        void scanStarted();
        void scanDone( bool changed );
        void databaseEngineChanged();

        void scoreChanged( const QString &url, int score );

        void coverFetched( const QString &artist, const QString &album );
        void coverRemoved( const QString &artist, const QString &album );
        void coverFetcherError( const QString &error );

        void similarArtistsFetched( const QString &artist );

    public:
        static CollectionDB *instance();

        QString escapeString( QString string ) { return m_dbConnPool->escapeString(string); }
        int getType() { return m_dbConnPool->getDbConnectionType(); }


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
        int insert( const QString& statement, const QString& table, DbConnection *conn = NULL );

        //table management methods
        bool isEmpty();
        bool isValid();
        void createTables( DbConnection *conn = NULL );
        void dropTables( DbConnection *conn = NULL );
        void clearTables( DbConnection *conn = NULL );
        void moveTempTables( DbConnection *conn );

        uint artistID( QString value, bool autocreate = true, const bool temporary = false, const bool updateSpelling = false, DbConnection *conn = NULL );
        uint albumID( QString value, bool autocreate = true, const bool temporary = false, const bool updateSpelling = false, DbConnection *conn = NULL );
        uint genreID( QString value, bool autocreate = true, const bool temporary = false, const bool updateSpelling = false, DbConnection *conn = NULL );
        uint yearID( QString value, bool autocreate = true, const bool temporary = false, const bool updateSpelling = false, DbConnection *conn = NULL );

        bool isDirInCollection( QString path );
        bool isFileInCollection( const QString &url );
        void removeDirFromCollection( QString path );
        void removeSongsInDir( QString path );
        void removeSongs( const KURL::List& urls );
        void updateDirStats( QString path, const long datetime, DbConnection *conn = NULL );

        //song methods
        bool addSong( MetaBundle* bundle, const bool incremental = false, DbConnection *conn = NULL );

        /**
         * The @p bundle parameter's url() will be looked up in the Collection
         * @param bundle this will be filled in with tags for you
         * @return true if in the collection
         */
        bool bundleForUrl( MetaBundle* bundle );
        QValueList<MetaBundle> bundlesByUrls( const KURL::List& urls );
        void addAudioproperties( const MetaBundle& bundle );

        void updateTags( const QString &url, const MetaBundle &bundle, const bool updateView = true );
        void updateURL( const QString &url, const bool updateView = true );

        //statistics methods
        int addSongPercentage( const QString &url, int percentage );
        int getSongPercentage( const QString &url  );
        void setSongPercentage( const QString &url , int percentage );

        //artist methods
        QStringList similarArtists( const QString &artist, uint count );

        //album methods
        void checkCompilations( const QString &path, const bool temporary = false, DbConnection *conn = NULL );
        void setCompilation( const QString &album, const bool enabled, const bool updateView = true );
        QString albumSongCount( const QString &artist_id, const QString &album_id );
        bool albumIsCompilation( const QString &album_id );

        //list methods
        QStringList artistList( bool withUnknowns = true, bool withCompilations = true );
        QStringList albumList( bool withUnknowns = true, bool withCompilations = true );
        QStringList genreList( bool withUnknowns = true, bool withCompilations = true );
        QStringList yearList( bool withUnknowns = true, bool withCompilations = true );

        QStringList albumListOfArtist( const QString &artist, bool withUnknown = true, bool withCompilations = true );
        QStringList artistAlbumList( bool withUnknown = true, bool withCompilations = true );

        QStringList albumTracks( const QString &artist_id, const QString &album_id );

        //cover management methods
        /** Returns the image from a given URL, network-transparently.
         * You must run KIO::NetAccess::removeTempFile( tmpFile ) when you are finished using the image;
         **/
        static QImage fetchImage(const KURL& url, QString &tmpFile);
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

        void applySettings();

    protected:
        CollectionDB();
        ~CollectionDB();

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
        static const int DATABASE_VERSION = 18;
        static const int DATABASE_STATS_VERSION = 3;
        static const int MONITOR_INTERVAL = 60; //sec
        static const bool DEBUG = false;

        void initialize();
        void destroy();
        void customEvent( QCustomEvent* );

        //general management methods
        void createStatsTable();
        void dropStatsTable();
        void scanModifiedDirs();

        QCString makeWidthKey( uint width );
        QString artistValue( uint id );
        QString albumValue( uint id );
        QString genreValue( uint id );
        QString yearValue( uint id );

        uint IDFromValue( QString name, QString value, bool autocreate = true, const bool temporary = false,
                          const bool updateSpelling = false, DbConnection *conn = NULL );

        QString valueFromID( QString table, uint id );

        //member variables
        QString m_amazonLicense;
        QString m_cacheArtist;
        uint m_cacheArtistID;
        QString m_cacheAlbum;
        uint m_cacheAlbumID;

        DBEngine* m_dbEngine;
        DbConnectionPool *m_dbConnPool;

        bool m_monitor;
        QDir m_cacheDir;
        QDir m_coverDir;
};


#endif /* AMAROK_COLLECTIONDB_H */
