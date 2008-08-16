/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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


#include "SqliteCollection.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"

#include <QMutexLocker>

#include <KRandom>

#include "sqlite/sqlite3.h"

#include <cmath>
#include <unistd.h>

SqliteCollection::SqliteCollection( const QString &id, const QString &prettyName )
    : SqlCollection( id, prettyName )
    , m_initialized( false )
    , m_mutex()
{
    QString dbFile = Amarok::config( "Sqlite" ).readEntry( "location",
                                        Amarok::saveLocation() + "collection.db" );

    const QByteArray path = QFile::encodeName( dbFile );

    // Open database file and check for correctness
    QFile file( path );
    if ( file.open( QIODevice::ReadOnly ) )
    {
        QByteArray format;
        format = file.readLine( 50 );
        if ( !format.startsWith( "SQLite format 3" ) )
        {
            warning() << "Database versions incompatible. Removing and rebuilding database.\n";
        }
        else if ( sqlite3_open( path, &m_db ) != SQLITE_OK )
        {
            warning() << "Database file corrupt. Removing and rebuilding database.\n";
            sqlite3_close( m_db );
        }
        else
            m_initialized = true;
        file.close();
    }

    if ( !m_initialized )
    {
        // Remove old db file; create new
        QFile::remove( path );
        if ( sqlite3_open( path, &m_db ) == SQLITE_OK )
        {
            m_initialized = true;
        }
    }
    if ( m_initialized )
    {
        if( sqlite3_create_function(m_db, "rand", 0, SQLITE_UTF8, NULL, sqlite_rand, NULL, NULL) != SQLITE_OK )
            m_initialized = false;
        if( sqlite3_create_function(m_db, "power", 2, SQLITE_UTF8, NULL, sqlite_power, NULL, NULL) != SQLITE_OK )
            m_initialized = false;
        if ( sqlite3_create_function(m_db, "like", 2, SQLITE_UTF8, NULL, sqlite_like_new, NULL, NULL) != SQLITE_OK )
            m_initialized = false;
        if ( sqlite3_create_function(m_db, "like", 3, SQLITE_UTF8, NULL, sqlite_like_new, NULL, NULL) != SQLITE_OK )
            m_initialized = false;
    }

    //optimization for speeding up SQLite
    query( "PRAGMA default_synchronous = OFF;" );
    init();
}

SqliteCollection::~SqliteCollection()
{
    if ( m_db ) sqlite3_close( m_db );
}

QString
SqliteCollection::type() const
{
    return "SQLite";
}

QStringList SqliteCollection::query( const QString& statement )
{
    QStringList values;
    int error;
    int rc = 0;
    const char* tail;
    sqlite3_stmt* stmt;
    int busyCnt = 0;
    int retryCnt = 0;

    QMutexLocker locker( &m_mutex );
    do {
        //compile SQL program to virtual machine, reattempting if busy
        do {
            if ( busyCnt )
            {
                ::usleep( 100000 );      // Sleep 100 msec
                debug() << "sqlite3_prepare: BUSY counter: " << busyCnt;
            }
            error = sqlite3_prepare( m_db, statement.toUtf8(), -1, &stmt, &tail );
        }
        while ( SQLITE_BUSY==error && busyCnt++ < 120 );

        if ( error != SQLITE_OK )
        {
            if ( SQLITE_BUSY==error )
                Debug::error() << "Gave up waiting for lock to clear";
            Debug::error() << " sqlite3_compile error:";
            Debug::error() << sqlite3_errmsg( m_db );
            Debug::error() << "on query: " << statement;
            values = QStringList();
            break;
        }
        else
        {
            busyCnt = 0;
            int number = sqlite3_column_count( stmt );
            //execute virtual machine by iterating over rows
            while ( true )
            {
                error = sqlite3_step( stmt );

                if ( error == SQLITE_BUSY )
                {
                    if ( busyCnt++ > 120 ) {
                        Debug::error() << "Busy-counter has reached maximum. Aborting this sql statement!\n";
                        break;
                    }
                    ::usleep( 100000 ); // Sleep 100 msec
                    debug() << "sqlite3_step: BUSY counter: " << busyCnt;
                    continue;
                }
                if ( error == SQLITE_MISUSE )
                    debug() << "sqlite3_step: MISUSE";
                if ( error == SQLITE_DONE || error == SQLITE_ERROR )
                    break;

                //iterate over columns
                for ( int i = 0; i < number; i++ )
                {
                    values << QString::fromUtf8( reinterpret_cast<const char*>( sqlite3_column_text( stmt, i ) ) );
                }
            }
            //deallocate vm resources
            rc = sqlite3_finalize( stmt );

            if ( error != SQLITE_DONE && rc != SQLITE_SCHEMA )
            {
                Debug::error() << "sqlite_step error.\n";
                Debug::error() << sqlite3_errmsg( m_db );
                Debug::error() << "on query: " << statement;
                values = QStringList();
            }
            if ( rc == SQLITE_SCHEMA )
            {
                retryCnt++;
                debug() << "SQLITE_SCHEMA error occurred on query: " << statement;
                if ( retryCnt < 10 )
                    debug() << "Retrying now.";
                else
                {
                    Debug::error() << "Retry-Count has reached maximum. Aborting this SQL statement!";
                    Debug::error() << "SQL statement: " << statement;
                    values = QStringList();
                }
            }
        }
    }
    while ( rc == SQLITE_SCHEMA && retryCnt < 10 );

    return values;
}

QString
SqliteCollection::idType() const
{
    return " INTEGER PRIMARY KEY AUTOINCREMENT";
}


int SqliteCollection::insert( const QString& statement, const QString& /* table */ )
{
    int error;
    int rc = 0;
    const char* tail;
    sqlite3_stmt* stmt;
    int busyCnt = 0;
    int retryCnt = 0;

    QMutexLocker locker( &m_mutex );
    do {
        //compile SQL program to virtual machine, reattempting if busy
        do {
            if ( busyCnt )
            {
                ::usleep( 100000 );      // Sleep 100 msec
                debug() << "sqlite3_prepare: BUSY counter: " << busyCnt;
            }
            error = sqlite3_prepare( m_db, statement.toUtf8(), -1, &stmt, &tail );
        }
        while ( SQLITE_BUSY==error && busyCnt++ < 120 );

        if ( error != SQLITE_OK )
        {
            if ( SQLITE_BUSY==error )
                Debug::error() << "Gave up waiting for lock to clear";
            Debug::error() << " sqlite3_compile error:";
            Debug::error() << sqlite3_errmsg( m_db );
            Debug::error() << "on insert: " << statement;
            break;
        }
        else
        {
            busyCnt = 0;
            //execute virtual machine by iterating over rows
            while ( true )
            {
                error = sqlite3_step( stmt );

                if ( error == SQLITE_BUSY )
                {
                    if ( busyCnt++ > 120 ) {
                        Debug::error() << "Busy-counter has reached maximum. Aborting this sql statement!\n";
                        break;
                    }
                    ::usleep( 100000 ); // Sleep 100 msec
                    debug() << "sqlite3_step: BUSY counter: " << busyCnt;
                }
                if ( error == SQLITE_MISUSE )
                    debug() << "sqlite3_step: MISUSE";
                if ( error == SQLITE_DONE || error == SQLITE_ERROR )
                    break;
            }
            //deallocate vm resources
            rc = sqlite3_finalize( stmt );

            if ( error != SQLITE_DONE && rc != SQLITE_SCHEMA)
            {
                Debug::error() << "sqlite_step error.\n";
                Debug::error() << sqlite3_errmsg( m_db );
                Debug::error() << "on insert: " << statement;
            }
            if ( rc == SQLITE_SCHEMA )
            {
                retryCnt++;
                debug() << "SQLITE_SCHEMA error occurred on insert: " << statement;
                if ( retryCnt < 10 )
                    debug() << "Retrying now.";
                else
                {
                    Debug::error() << "Retry-Count has reached maximum. Aborting this SQL insert!";
                    Debug::error() << "SQL statement: " << statement;
                }
            }
        }
    }
    while ( SQLITE_SCHEMA == rc && retryCnt < 10 );
    return sqlite3_last_insert_rowid( m_db );
}

// this implements a RAND() function compatible with the MySQL RAND() (0-param-form without seed)
void SqliteCollection::sqlite_rand(sqlite3_context *context, int /*argc*/, sqlite3_value ** /*argv*/)
{
    sqlite3_result_double( context, static_cast<double>(KRandom::random()) / (RAND_MAX+1.0) );
}

// this implements a POWER() function compatible with the MySQL POWER()
void SqliteCollection::sqlite_power(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    Q_ASSERT( argc==2 );
    if( sqlite3_value_type(argv[0])==SQLITE_NULL || sqlite3_value_type(argv[1])==SQLITE_NULL ) {
        sqlite3_result_null(context);
        return;
    }
    double a = sqlite3_value_double(argv[0]);
    double b = sqlite3_value_double(argv[1]);
    sqlite3_result_double( context, pow(a,b) );
}

// this implements a LIKE() function that overrides the default string comparison function
// Reason: default function is case-sensitive for utf8 strings (BUG: 116458, ...)
void SqliteCollection::sqlite_like_new( sqlite3_context *context, int argc, sqlite3_value **argv )
{

    const unsigned char *zA = sqlite3_value_text( argv[0] );
    const unsigned char *zB = sqlite3_value_text( argv[1] );

    QString pattern = QString::fromUtf8( (const char*)zA );
    QString text = QString::fromUtf8( (const char*)zB );

    int begin = pattern.startsWith( '%' ), end = pattern.endsWith( '%' );
    if (begin)
        pattern = pattern.right( pattern.length() - 1 );
    if (end)
        pattern = pattern.left( pattern.length() - 1 );

    if( argc == 3 ) // The function is given an escape character. In likeCondition() it defaults to '/'
        pattern.replace( "/%", "%" ).replace( "/_", "_" ).replace( "//", "/" );

    int result = 0;
    if ( begin && end ) result = ( text.indexOf( pattern, 0, Qt::CaseInsensitive ) != -1);
    else if ( begin ) result = text.endsWith( pattern, Qt::CaseInsensitive );
    else if ( end ) result = text.startsWith( pattern, Qt::CaseInsensitive );
    else result = ( text.toLower() == pattern.toLower() );

    sqlite3_result_int( context, result );
}

#include "SqliteCollection.moc"
