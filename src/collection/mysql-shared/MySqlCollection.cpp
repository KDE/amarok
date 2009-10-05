/****************************************************************************************
 * Copyright (c) 2008 Edward Toroshchin <edward.hades@gmail.com>                        *
 * Copyright (c) 2009 Jeff Mitchell <mitchell@kde.org>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MySqlCollection.h"

#include "Amarok.h"
#include "Debug.h"
#include "amarokconfig.h"

#include <QMutexLocker>
#include <QThreadStorage>
#include <QVarLengthArray>


#include <mysql.h>

/**
 * This class is used by MySqlCollection to fulfill mysql's thread
 * requirements. In every function that calls mysql_*, an init() method of
 * this class must be invoked.
 */
class ThreadInitializer
{
    static int threadsCount;
    static QMutex countMutex;
    static QThreadStorage< ThreadInitializer* > storage;

    /**
     * This should be called ONLY by init()
     */
    ThreadInitializer()
    {
        mysql_thread_init();

        countMutex.lock();
        threadsCount++;
        countMutex.unlock();

        debug() << "Initialized thread, count==" << threadsCount;
    }

public:
    /**
     * This is called by QThreadStorage when a thread is destroyed
     */
    ~ThreadInitializer()
    {
        mysql_thread_end();

        countMutex.lock();
        threadsCount--;
        countMutex.unlock();

        debug() << "Deinitialized thread, count==" << threadsCount;

        if( threadsCount == 0 )
            mysql_library_end();
    }

    static void init()
    {
        if( !storage.hasLocalData() )
            storage.setLocalData( new ThreadInitializer() );
    }
};

int ThreadInitializer::threadsCount = 0;
QMutex ThreadInitializer::countMutex;
QThreadStorage< ThreadInitializer* > ThreadInitializer::storage;


MySqlCollection::MySqlCollection( const QString &id, const QString &prettyName )
    : SqlCollection( id, prettyName )
    , m_db( 0 )
    , m_mutex( QMutex::Recursive )
{
    //Relevant code must be implemented in subclasses
}

MySqlCollection::~MySqlCollection()
{
    DEBUG_BLOCK

    if( m_db )
    {
        mysql_close( m_db );
        m_db = 0;
    }
}

QStringList MySqlCollection::query( const QString& statement )
{
    //DEBUG_BLOCK
    //debug() << "[ATTN!] MySql::query( " << statement << " )";

    initThreadInitializer();
    QMutexLocker locker( &m_mutex );

    QStringList values;
    if( !m_db )
    {
        error() << "Tried to perform query on uninitialized MySQL";
        return values;
    }

    int res = mysql_query( m_db, statement.toUtf8() ); 
    
    if( res )
    {
        reportError( statement );
        return values;
    }

    MYSQL_RES *pres = mysql_store_result( m_db );
    if( !pres ) // No results... check if any were expected
    {
        if( mysql_field_count( m_db ) )
            reportError( statement );
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

int MySqlCollection::insert( const QString& statement, const QString& /* table */ )
{
    //DEBUG_BLOCK
    //debug() << "[ATTN!] MySql::insert( " << statement << " )";

    initThreadInitializer();
    QMutexLocker locker( &m_mutex );

    if( !m_db )
    {
        error() << "Tried to perform insert on uninitialized MySQL";
        return 0;
    }

    int res = mysql_query( m_db, statement.toUtf8() ); 
    if( res )
    {
        reportError( statement );
        return 0;
    }

    MYSQL_RES *pres = mysql_store_result( m_db );
    if( pres )
    {
        warning() << "[IMPORTANT!] insert returned data";
        mysql_free_result( pres );
    }

    res = mysql_insert_id( m_db ); 
    
    return res;
}

QString
MySqlCollection::escape( QString text ) const
{
    if( !m_db )
    {
        error() << "Tried to perform escape() on uninitialized MySQL";
        return QString();
    }

    const QByteArray utfText = text.toUtf8();
    const int length = utfText.length() * 2 + 1;
    QVarLengthArray<char, 1000> outputBuffer( length );

    mysql_real_escape_string( m_db, outputBuffer.data(), utfText.constData(), utfText.length() );

    return QString::fromUtf8( outputBuffer.constData() );
}

QString
MySqlCollection::randomFunc() const
{
    return "RAND()";
}

void
MySqlCollection::reportError( const QString& message )
{
    error() << "GREPME MySQL query failed!" << mysql_error( m_db ) << " on " << message;
}

void
MySqlCollection::initThreadInitializer()
{
    ThreadInitializer::init();
}

#include "MySqlCollection.moc"

