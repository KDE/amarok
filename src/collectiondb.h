// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#ifndef AMAROK_COLLECTIONDB_H
#define AMAROK_COLLECTIONDB_H

#include "engineobserver.h"

#include <qdir.h>            //stack allocated
#include <qobject.h>         //baseclass
#include <qstringlist.h>     //stack allocated

#ifdef USE_MYSQL
#include <qdatetime.h>
namespace mysql
{
#include <mysql/mysql.h>
}
#else
#include "sqlite/sqlite3.h"
#endif

class CollectionEmitter;
class MetaBundle;
class ThreadWeaver;

class CollectionDB : public QObject
{
    Q_OBJECT

    public:
        //attributes:
        enum qBuilderTables  { tabAlbum = 1, tabArtist = 2, tabGenre = 4, tabYear = 8 };
        enum qBuilderOptions { optNoCompilations = 1, optOnlyCompilations = 2, optNoUnknowns = 4 };

        CollectionDB();
        ~CollectionDB();

        static CollectionEmitter* emitter() { return s_emitter; }

        //sql helper methods
        QStringList query( const QString& statement, QStringList& names, bool debug = false);
        // no away to add a default argument for a Object& in gcc3, hack around it
        QStringList query( const QString& statement ) { QStringList sl; return query( statement, sl, false ); }
        int sqlInsertID();
        QString escapeString( QString string );

        //table management methods
        bool isValid();
        bool isEmpty();
        void createTables( const bool temporary = false );
        void dropTables( const bool temporary = false );
        void moveTempTables();
        void createStatsTable();
        void dropStatsTable();

        void updateTags( const QString &url, const MetaBundle &bundle, bool updateCB=true );
        void updateURL( const QString &url );

        //general management methods
        void scan( const QStringList& folders, bool recursively, bool importPlaylists );
        void scanModifiedDirs( bool recursively, bool importPlaylists );
        bool isDirInCollection( QString path );
        bool isFileInCollection( const QString &url  );
        void removeDirFromCollection( QString path );
        void updateDirStats( QString path, const long datetime );
        void removeSongsInDir( QString path );
        void purgeDirCache();

        //song methods
        bool getMetaBundleForUrl( const QString &url , MetaBundle *bundle );
        void addAudioproperties( const MetaBundle& bundle );
        int addSongPercentage( const QString &url , const int percentage );
        int getSongPercentage( const QString &url  );
        void setSongPercentage( const QString &url , int percentage );

        //album methods
        bool isSamplerAlbum( const QString &album );
        QString albumSongCount( const QString &artist_id, const QString &album_id );
        QString getPathForAlbum( const uint artist_id, const uint album_id );
        QString getPathForAlbum( const QString &artist, const QString &album );

        //list methods
        QStringList artistList( int options = 0, const QString& filter = QString::null, int flags = 0 );
        QStringList albumList( int options = 0, const QString& filter = QString::null, int flags = 0 );
        QStringList genreList( int options = 0, const QString& filter = QString::null, int flags = 0 );
        QStringList yearList( int options = 0, const QString& filter = QString::null, int flags = 0 );

        QStringList albumListOfArtist( const QString &artist, bool withUnknown = true, bool withCompilations = true );
        QStringList artistAlbumList( bool withUnknown = true, bool withCompilations = true );

        //cover management methods
        /** Saves images located on the user's filesystem */
        bool setAlbumImage( const QString& artist, const QString& album, const KURL& url );
        /** Saves images obtained from CoverFetcher */
        bool setAlbumImage( const QString& artist, const QString& album, QImage img, const QString& amazonUrl = QString::null );

        QString albumImage( const uint artist_id, const uint album_id, const uint width = 1 );
        QString albumImage( const QString &artist, const QString &album, const uint width = 1 );

        bool removeAlbumImage( const uint artist_id, const uint album_id );
        bool removeAlbumImage( const QString &artist, const QString &album );

        void addImageToPath( const QString path, const QString image, bool temporary );
        QString getImageForPath( const QString path, uint width = 0 );

        uint artistID( QString value, bool autocreate = true, bool useTempTables = false );
        QString artistValue( uint id );
        uint albumID( QString value, bool autocreate = true, bool useTempTables = false );
        QString albumValue( uint id );
        uint genreID( QString value, bool autocreate = true, bool useTempTables = false );
        QString genreValue( uint id );
        uint yearID( QString value, bool autocreate = true, bool useTempTables = false );
        QString yearValue( uint id );

        //tree methods
        void retrieveFirstLevel( const QString& category1, const QString& category2, const QString& category3,
                                            QString filter, QStringList& values, QStringList& names );
        void retrieveSecondLevel( const QString& itemText, const QString& category1, const QString& category2, 
                                                 const QString& category3, QString filter, 
                                                 QStringList& values, QStringList& names );
        void retrieveThirdLevel( const QString& itemText1, const QString& itemText2, const QString& category1, 
                                             const QString& category2, const QString& category3, QString filter, 
                                            QStringList& values, QStringList& names );
        void retrieveFourthLevel( const QString& itemText1, const QString& itemText2, const QString& itemText3, 
                                               const QString& category1, const QString& category2, 
                                               const QString& category3, QString filter, 
                                               QStringList& values, QStringList& names );

        void retrieveFirstLevelURLs( const QString& itemText, const QString& category1, QString gcategory2, const QString& category3,
                                                    QString filter, QStringList& values, QStringList& names );
        void retrieveSecondLevelURLs( const QString& itemText1, const QString& itemText2, const QString& category1, 
                                                         const QString& category2, const QString& category3, QString filter,
                                                         QStringList& values, QStringList& names );
        void retrieveThirdLevelURLs( const QString& itemText1, const QString& itemText2, const QString& itemText3, 
                                                     const QString& category1, const QString& category2, const QString& category3, 
                                                     QString filter, QStringList& values, QStringList& names );

        //member variables
        QString m_amazonLicense;

        QStringList m_values;
        QStringList m_names;

    protected:
        QCString md5sum( const QString& artist, const QString& album );

    public slots:
        void fetchCover( QObject* parent, const QString& artist, const QString& album, bool noedit );
        void stopScan();

    private slots:
        void dirDirty( const QString& path );
        void saveCover( const QString& keyword, const QString& url, const QImage& image );
        void fetcherError();

    private:
        void customEvent( QCustomEvent* );

        QString qBuilderLinkTables( int tables );
        QString qBuilderAddFilter( int tables, const QString& filter );
        QString qBuilderExcludeFilter( int tables, const QString& filter );
        QString qBuilderSetOptions( int options );

        uint IDFromValue( QString name, QString value, bool autocreate = true, bool useTempTables = false );
        QString valueFromID( QString table, uint id );

        static CollectionEmitter* s_emitter;
#ifdef USE_MYSQL
        mysql::MYSQL* m_db;
#else
        sqlite3* m_db;
#endif
        ThreadWeaver* m_weaver;
        bool m_monitor;
        QDir m_cacheDir;
        QDir m_coverDir;

        QString m_cacheArtist;
        uint m_cacheArtistID;
        QString m_cacheAlbum;
        uint m_cacheAlbumID;
};


class CollectionEmitter : public QObject, public EngineObserver
{
    Q_OBJECT

    friend class CollectionDB;

    public:
        CollectionEmitter();

    protected:
        void engineTrackEnded( int finalPosition, int trackLength );

    signals:
        void scanStarted();
        void scanDone( bool changed );

        void scoreChanged( const QString &url, int score );

        void coverFetched( const QString &keyword );
        void coverFetched();
        void coverFetcherError();
};


#endif /* AMAROK_COLLECTIONDB_H */
