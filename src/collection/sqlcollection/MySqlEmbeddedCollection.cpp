/*
 *  Copyright (c) Edward Toroshchin <edward.hades@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "MySqlEmbeddedCollection.h"

#include "Amarok.h"
#include "Debug.h"
#include "amarokconfig.h"

#include <QMutexLocker>

#include <KRandom>

#include <pthread.h>

/**
 * This class is used to maintain thread safety in MySQL.
 *
 * It should be constructed after mysql_library_init() and before every
 * mysql_query() or better anything at all mysql_*
 */
class ThreadInitializer
{
#ifdef Q_OS_WIN32
    // This will not work in win32 and actually should not be needed there
#else
    static QList< pthread_t > m_inited;
public:
    ThreadInitializer( )
    {
        if( m_inited.isEmpty() )
            m_inited << pthread_self();
        else
        {
            bool inited = false;
            foreach( pthread_t t, m_inited )
            {
                if( pthread_equal( t, pthread_self() ) )
                {
                    inited = true;
                    break;
                }
            }
            if( !inited )
            {
                // pthread_t is not guaranteed to be int, so if it ever
                // breaks compilation, just remove this debug
                debug() << "Calling mysql_thread_init() for thread " 
                        << int( pthread_self() );
                mysql_thread_init();
                m_inited << pthread_self();
            }
        }
    }
#endif
};

#ifndef Q_OS_WIN32
QList< pthread_t > ThreadInitializer::m_inited;
#endif

MySqlEmbeddedCollection::MySqlEmbeddedCollection( const QString &id, 
                                                    const QString &prettyName )
    : SqlCollection( id, prettyName )
    , m_db( 0 )
{
    QString defaultsFile = Amarok::config( "MySQLe" ).readEntry( "config",
                    Amarok::saveLocation() + "my.cnf" ); 
    QString databaseDir = Amarok::config( "MySQLe" ).readEntry( "data",
                    Amarok::saveLocation() + "mysqle" );
    char* defaultsLine = qstrdup( QString( "--defaults-file=%1" ).arg( 
                    defaultsFile ).toAscii().data() );
    char* databaseLine = qstrdup( QString( "--datadir=%1" ).arg(
                    databaseDir ).toAscii().data() );

    if( !QFile::exists( defaultsFile ) )
    {
        QFile df( defaultsFile );
        df.open( QIODevice::WriteOnly );
    }

    if( !QFile::exists( databaseDir ) )
    {
        QDir dir( databaseDir );
        dir.mkpath( "." );
    }

    static const int num_elements = 3;
    char **server_options = new char* [ num_elements + 1 ];
    server_options[0] = "amarokmysqld";
    server_options[1] = defaultsLine;
    server_options[2] = databaseLine;
    server_options[3] = 0;

    char **server_groups = new char* [ 3 ];
    server_groups[0] = "amarokserver";
    server_groups[1] = "amarokclient";
    server_groups[2] = 0;

    mysql_library_init(num_elements, server_options, server_groups);
    m_db = mysql_init(NULL);
    delete [] server_options;
    delete [] server_groups;
    delete [] defaultsLine;
    delete [] databaseLine;
    if( !m_db )
    {
        error() << "MySQLe initialization failed";
    }
    else
    {
        mysql_options(m_db, MYSQL_READ_DEFAULT_GROUP, "amarokclient");
        mysql_options(m_db, MYSQL_OPT_USE_EMBEDDED_CONNECTION, NULL);
    
        if( !mysql_real_connect(m_db, NULL,NULL,NULL, 0, 0,NULL,0) )
        {
            error() << "Could not connect to mysql!";
            reportError();
            mysql_close( m_db );
            m_db = 0;
        }
        else
        {
    
            mysql_query(m_db, "CREATE DATABASE IF NOT EXISTS amarok");
            mysql_query(m_db, "CREATE DATABASE IF NOT EXISTS mysql");
            mysql_query(m_db, "USE amarok");
        }
    
        ThreadInitializer ti;
        init();
    }
}

MySqlEmbeddedCollection::~MySqlEmbeddedCollection()
{
    mysql_close(m_db);
    mysql_library_end();
}

QStringList MySqlEmbeddedCollection::query( const QString& statement )
{
    DEBUG_BLOCK
    debug() << "[ATTN!] MySqlEmbedded::query( " << statement << " )";

    ThreadInitializer ti;
    QMutexLocker locker( &m_mutex );

    QStringList values;
    if( !m_db )
    {
        error() << "Tried to perform query on uninitialized MySQLe";
        return values;
    }

    int res = mysql_query(m_db, statement.toUtf8() ); 
    
    if( res )
    {
        reportError();
        return values;
    }

    MYSQL_RES *pres = mysql_store_result( m_db );
    if( !pres )
    {
        if( mysql_field_count( m_db ) ) reportError();
        return values;
    }
    
    int number = mysql_num_fields( pres );
    if( number <= 0 )
    {
        warning() << "Errr... query returned but with no fields";
    }

    MYSQL_ROW row = mysql_fetch_row( pres );
    while( row )
    {
        for( int i = 0; i < number; i++ )
        {
            values << QString::fromUtf8( (const char*) row[i] );
        }
    
        row = mysql_fetch_row( pres );
    }

    mysql_free_result( pres );
    
    return values;
}

int MySqlEmbeddedCollection::insert( const QString& statement, const QString& /* table */ )
{
    DEBUG_BLOCK
    debug() << "[ATTN!] MySqlEmbedded::insert( " << statement << " )";

    ThreadInitializer ti;
    QMutexLocker locker( &m_mutex );

    if( !m_db )
    {
        error() << "Tried to perform insert on uninitialized MySQLe";
        return 0;
    }

    int res = mysql_query(m_db, statement.toUtf8() ); 
    if( res )
    {
        reportError();
        return 0;
    }

    MYSQL_RES *pres = mysql_store_result( m_db );
    if( pres )
    {
        warning() << "[IMPORTANT!] insert returned data";
        mysql_free_result( pres );
    }

    res = mysql_insert_id(m_db ); 
    
    return res;
}

QString
MySqlEmbeddedCollection::escape( QString text ) const
{
    return text.replace("\\", "\\\\").replace( '\'', "''" );
}

QString
MySqlEmbeddedCollection::randomFunc(  ) const
{
    return "RAND()";
}

void
MySqlEmbeddedCollection::reportError()
{
    error() << "GREPME MySQLe query failed!" << mysql_error( m_db );
}

QString
MySqlEmbeddedCollection::type() const
{
    return "MySQLe";
}

#include "MySqlEmbeddedCollection.moc"

