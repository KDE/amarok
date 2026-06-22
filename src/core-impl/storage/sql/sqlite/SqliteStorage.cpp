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

    return query.lastInsertId().toInt();
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

bool
SqliteStorage::isMySQL() const
{
    return false;
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
