/****************************************************************************************
 * Copyright (c) 2025 Amarok Team <amarok@kde.org>                                 *
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

#define DEBUG_PREFIX "SqliteStorage"

#include "SqliteStorage.h"

#include "core/support/Debug.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>


SqliteStorage::SqliteStorage()
    : SqlStorage()
{
}

SqliteStorage::~SqliteStorage()
{
    if( m_db.isOpen() )
        m_db.close();
    QSqlDatabase::removeDatabase( m_db.connectionName() );
}

bool
SqliteStorage::init( const QString &dbPath )
{
    QString connectionName = QStringLiteral("SqliteStorage-") + QUuid::createUuid().toString();
    m_db = QSqlDatabase::addDatabase( QStringLiteral("QSQLITE"), connectionName );
    m_db.setDatabaseName( dbPath );

    if( !m_db.open() )
    {
        m_lastError = m_db.lastError().text();
        m_lastErrors << m_lastError;
        error() << "Failed to open SQLite database:" << m_lastError;
        return false;
    }

    m_databaseName = QFileInfo( dbPath ).absoluteFilePath();

    QSqlQuery query( m_db );

    if( !query.exec( QStringLiteral("PRAGMA journal_mode=WAL;") ) )
        warning() << "Failed to set WAL mode:" << query.lastError().text();

    if( !query.exec( QStringLiteral("PRAGMA busy_timeout=5000;") ) )
        warning() << "Failed to set busy timeout:" << query.lastError().text();

    if( !query.exec( QStringLiteral("PRAGMA synchronous=NORMAL;") ) )
        warning() << "Failed to set synchronous mode:" << query.lastError().text();

    debug() << "SQLite database opened at:" << m_databaseName;

    return true;
}

QString
SqliteStorage::databaseName() const
{
    return m_databaseName;
}

QStringList
SqliteStorage::query( const QString &statement )
{
    QMutexLocker locker( &m_mutex );

    QStringList values;

    if( !m_db.isOpen() )
    {
        error() << "Tried to perform query on uninitialized SQLite";
        return values;
    }

    QSqlQuery query( m_db );
    query.setForwardOnly( true );

    if( !query.exec( statement ) )
    {
        reportError( statement );
        return values;
    }

    while( query.next() )
    {
        for( int i = 0; i < query.record().count(); ++i )
        {
            values << query.value( i ).toString();
        }
    }

    return values;
}

int
SqliteStorage::insert( const QString &statement, const QString & )
{
    QMutexLocker locker( &m_mutex );

    if( !m_db.isOpen() )
    {
        error() << "Tried to perform insert on uninitialized SQLite";
        return 0;
    }

    QSqlQuery query( m_db );

    if( !query.exec( statement ) )
    {
        reportError( statement );
        return 0;
    }

    int lastId = query.lastInsertId().toInt();
    if( lastId <= 0 )
        return 0;

    // SQLite's lastInsertId() returns the ROWID of the last row in a batch
    // INSERT, but Amarok's committer code expects the first (MySQL behavior).
    // Count rows in the VALUES clause to compute the correct first ID.
    int valuesIdx = statement.indexOf( QStringLiteral("VALUES ") );
    if( valuesIdx < 0 )
        return lastId;

    int rowCount = 1;
    int depth = 0;
    bool inString = false;
    for( int i = valuesIdx + 7; i < statement.length(); i++ )
    {
        const QChar c = statement[i];
        if( inString )
        {
            if( c == QLatin1Char('\'') )
            {
                if( i + 1 < statement.length() && statement[i+1] == QLatin1Char('\'') )
                    i++; // escaped single quote in string
                else
                    inString = false;
            }
        }
        else if( c == QLatin1Char('\'') )
            inString = true;
        else if( c == QLatin1Char('(') )
            depth++;
        else if( c == QLatin1Char(')') )
        {
            depth--;
            if( depth == 0 )
            {
                int j = i + 1;
                while( j < statement.length() && statement[j].isSpace() )
                    j++;
                if( j < statement.length() && statement[j] == QLatin1Char(',') )
                    rowCount++;
            }
        }
    }

    if( rowCount > 1 )
        return lastId - rowCount + 1;

    return lastId;
}

QString
SqliteStorage::escape( const QString &text ) const
{
    QMutexLocker locker( &m_mutex );

    if( !m_db.isOpen() )
    {
        error() << "Tried to perform escape() on uninitialized SQLite";
        return QString();
    }

    QString escaped = text;
    escaped.replace( QStringLiteral("'"), QStringLiteral("''") );
    return escaped;
}

QString
SqliteStorage::randomFunc() const
{
    return QStringLiteral("RANDOM()");
}

QString
SqliteStorage::sqlCreateTableOptions() const
{
    return QString();
}

bool
SqliteStorage::supportsPrefixIndexes() const
{
    return false;
}

QStringList
SqliteStorage::queryTables()
{
    return query( QStringLiteral("SELECT name FROM sqlite_master WHERE type='table'") );
}

QStringList
SqliteStorage::queryColumns( const QString &table )
{
    QStringList result;
    QStringList rows = query( QStringLiteral("SELECT name FROM pragma_table_info('%1')").arg( escape( table ) ) );
    // PRAGMA table_info returns flat list: cid, name, type, notnull, dflt_value, pk per column
    for( int i = 1; i < rows.size(); i += 6 )
        result << rows[i];
    return result;
}

void
SqliteStorage::dropIndex( const QString &indexName, const QString &tableName )
{
    Q_UNUSED( tableName )
    query( QStringLiteral("DROP INDEX IF EXISTS %1").arg( indexName ) );
}

QString
SqliteStorage::showCreateTable( const QString &table )
{
    QStringList result = query( QStringLiteral("SELECT sql FROM sqlite_master WHERE type='table' AND name='%1'").arg( escape( table ) ) );
    return result.isEmpty() ? QString() : result.first();
}

QString
SqliteStorage::boolTrue() const
{
    return QStringLiteral("1");
}

QString
SqliteStorage::boolFalse() const
{
    return QStringLiteral("0");
}

QString
SqliteStorage::idType() const
{
    return QStringLiteral("INTEGER PRIMARY KEY AUTOINCREMENT");
}

QString
SqliteStorage::textColumnType( int length ) const
{
    Q_UNUSED( length );
    return QStringLiteral("TEXT");
}

QString
SqliteStorage::exactTextColumnType( int length ) const
{
    Q_UNUSED( length );
    return QStringLiteral("TEXT");
}

QString
SqliteStorage::exactIndexableTextColumnType( int length ) const
{
    Q_UNUSED( length );
    return QStringLiteral("TEXT");
}

QString
SqliteStorage::longTextColumnType() const
{
    return QStringLiteral("TEXT");
}

QStringList
SqliteStorage::getLastErrors() const
{
    return m_lastErrors;
}

void
SqliteStorage::clearLastErrors()
{
    m_lastErrors.clear();
}

void
SqliteStorage::reportError( const QString &message )
{
    QSqlError error = m_db.lastError();
    QString errorMessage = QStringLiteral("SQLite error: %1 (query: %2)").arg( error.text(), message );
    m_lastErrors << errorMessage;
    m_lastError = errorMessage;
    debug() << errorMessage;
}
