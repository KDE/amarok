// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#ifndef AMAROK_COLLECTIONDB_H
#define AMAROK_COLLECTIONDB_H

#include <qobject.h>
#include <qstringlist.h>     //stack allocated

class sqlite;

class CollectionDB : public QObject
{
    Q_OBJECT
    
    public:
        CollectionDB( QCString path );
        ~CollectionDB();

        QString albumSongCount( const QString artist_id, const QString album_id );
        void addImageToPath( const QString path, const QString image, bool temporary );
        QString getImageForAlbum( const QString artist_id, const QString album_id, const QString defaultImage );
        void incSongCounter( const QString url );

        /**
         * Executes an SQL statement on the already opened database
         * @param statement SQL program to execute. Only one SQL statement is allowed.
         * @retval values   will contain the queried data, set to NULL if not used
         * @retval names    will contain all column names, set to NULL if not used
         * @return          true if successful
         */
        bool execSql( const QString& statement, QStringList* const values = 0, QStringList* const names = 0 );

        /**
         * Returns the rowid of the most recently inserted row
         * @return          int rowid
         */
        int sqlInsertID();
        QString escapeString( QString string );

        uint getValueID( QString name, QString value, bool autocreate = true );
        void createTables( const bool temporary = false );
        void dropTables( const bool temporary = false );
        void moveTempTables();
      
    private:
        sqlite* m_db;
};


#endif /* AMAROK_COLLECTIONDB_H */
