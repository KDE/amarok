// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// See COPYING file for licensing information.

#ifndef AMAROK_SQLITE_DBENGINE_H
#define AMAROK_SQLITE_DBENGINE_H

#include "dbenginebase.h"
#include <kurl.h>
#include <qdir.h>            //stack allocated
#include <qobject.h>         //baseclass
#include <qptrqueue.h>       //baseclass
#include <qsemaphore.h>      //stack allocated
#include <qstringlist.h>     //stack allocated

class DbConfig;
class DbConnection;
class DbConnectionPool;
class CoverFetcher;
class MetaBundle;
class Scrobbler;


class SqliteConfig : public DbConfig
{
    public:
        SqliteConfig( const QString& /* dbfile */ );

        const QString dbFile() const { return m_dbfile; }

    private:
        QString m_dbfile;
};


typedef struct sqlite3 sqlite3;
typedef struct sqlite3_context sqlite3_context;
typedef struct Mem sqlite3_value;

class SqliteDbEngine : public DbConnection
{
    public:
        SqliteDbEngine();
       ~SqliteDbEngine();

        QStringList query( const QString& /* statement */ );
        int insert( const QString& /* statement */, const QString& /* table */ );
        bool isConnected()const { return true; }
    private:
        static void sqlite_rand(sqlite3_context *context, int /*argc*/, sqlite3_value ** /*argv*/);
        static void sqlite_power(sqlite3_context *context, int argc, sqlite3_value **argv);

        sqlite3* m_db;
};


#endif /*SQLITE_DBENGINE_H*/
