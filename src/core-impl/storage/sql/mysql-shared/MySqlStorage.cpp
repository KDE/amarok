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

#define DEBUG_PREFIX "MySqlStorage"

#include "MySqlStorage.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "amarokconfig.h"

#include <QMutexLocker>
#include <QThreadStorage>
#include <QVarLengthArray>


#include <mysql.h>

/**
 * This class is used by MySqlStorage to fulfill mysql's thread
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

        debug() << "Initialized thread, count ==" << threadsCount;

        countMutex.unlock();
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

        debug() << "Deinitialized thread, count ==" << threadsCount;

        if( threadsCount == 0 )
            mysql_library_end();

        countMutex.unlock();
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


MySqlStorage::MySqlStorage()
    : SqlStorage()
    , m_db( nullptr )
    , m_debugIdent( QStringLiteral("MySQL-none") )
{
    //Relevant code must be implemented in subclasses
}

MySqlStorage::~MySqlStorage()
{ }

QStringList MySqlStorage::query( const QString& statement )
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

    int rows = mysql_num_rows( pres );
    values.reserve( rows );
    MYSQL_ROW row = mysql_fetch_row( pres );
    while( row )
    {
        for( int i = 0; i < number; ++i )
        {
            values << QString::fromUtf8( (const char*) row[i] );
        }

        row = mysql_fetch_row( pres );
    }

    mysql_free_result( pres );
    return values;
}

int MySqlStorage::insert( const QString& statement, const QString& /* table */ )
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
MySqlStorage::escape( const QString &text ) const
{
    if( !m_db )
    {
        error() << "Tried to perform escape() on uninitialized MySQL";
        return QString();
    }

    const QByteArray utfText = text.toUtf8();
    const int length = utfText.length() * 2 + 1;
    QVarLengthArray<char, 1000> outputBuffer( length );

    {
        QMutexLocker locker( &m_mutex );
        mysql_real_escape_string( m_db, outputBuffer.data(), utfText.constData(), utfText.length() );
    }

    return QString::fromUtf8( outputBuffer.constData() );
}

QString
MySqlStorage::randomFunc() const
{
    return QStringLiteral("RAND()");
}

QString
MySqlStorage::boolTrue() const
{
    return QStringLiteral("1");
}

QString
MySqlStorage::boolFalse() const
{
    return QStringLiteral("0");
}

QString
MySqlStorage::idType() const
{
    return QStringLiteral("INTEGER PRIMARY KEY AUTO_INCREMENT");
}

QString
MySqlStorage::textColumnType( int length ) const
{
    return QStringLiteral( "VARCHAR(%1)" ).arg( length );
}

QString
MySqlStorage::exactTextColumnType( int length ) const
{
    return textColumnType( length );
}

QString
MySqlStorage::exactIndexableTextColumnType( int length ) const
{
    return textColumnType( length );
}

QString
MySqlStorage::longTextColumnType() const
{
    return QStringLiteral("TEXT");
}

QStringList
MySqlStorage::getLastErrors() const
{
    QMutexLocker locker( &m_mutex );
    return m_lastErrors;
}

void
MySqlStorage::clearLastErrors()
{
    QMutexLocker locker( &m_mutex );
    m_lastErrors.clear();
}

void
MySqlStorage::reportError( const QString& message )
{
    QMutexLocker locker( &m_mutex );
    QString errorMessage;
    if( m_db )
        errorMessage = m_debugIdent + QStringLiteral(" query failed! (") + QString::number( mysql_errno( m_db ) ) + QStringLiteral(") ") + QLatin1String(mysql_error( m_db )) + QStringLiteral(" on ") + message;
    else
        errorMessage = m_debugIdent + QStringLiteral(" something failed! on ") + message;
    error() << errorMessage;

    if( m_lastErrors.count() < 20 )
        m_lastErrors.append( errorMessage );
}


void
MySqlStorage::initThreadInitializer()
{
    ThreadInitializer::init();
}

bool
MySqlStorage::sharedInit( const QString &databaseName )
{
    QMutexLocker locker( &m_mutex );
    if( mysql_query( m_db, QStringLiteral( "SET NAMES 'utf8'" ).toUtf8() ) )
        reportError( QStringLiteral("SET NAMES 'utf8' died") );
    if( mysql_query( m_db, QStringLiteral( "CREATE DATABASE IF NOT EXISTS %1 DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_bin" ).arg( databaseName ).toUtf8() ) )
        reportError( QStringLiteral( "Could not create %1 database" ).arg( databaseName ) );
    if( mysql_query( m_db, QStringLiteral( "ALTER DATABASE %1 DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_bin" ).arg( databaseName ).toUtf8() ) )
        reportError( QStringLiteral("Could not alter database charset/collation") );
    if( mysql_query( m_db, QStringLiteral( "USE %1" ).arg( databaseName ).toUtf8() ) )
    {
        reportError( QStringLiteral("Could not select database") );
        return false; // this error is fatal
    }

    debug() << "Connected to MySQL server" << mysql_get_server_info( m_db );
    return true;
}
