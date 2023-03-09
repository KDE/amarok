/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "ImporterSqlConnection.h"

#include "core/support/Debug.h"

#include <ThreadWeaver/Thread>

#include <QMutexLocker>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

using namespace StatSyncing;

ImporterSqlConnection::ImporterSqlConnection( const QString &driver,
                                              const QString &hostname,
                                              const quint16 port, const QString &dbName,
                                              const QString &user,
                                              const QString &password )
    : m_connectionName( QUuid::createUuid().toString() )
    , m_openTransaction( false )
{
    QSqlDatabase db = QSqlDatabase::addDatabase( driver, m_connectionName );
    db.setHostName( hostname );
    db.setPort( port );
    db.setDatabaseName( dbName );
    db.setUserName( user );
    db.setPassword( password );
}

ImporterSqlConnection::ImporterSqlConnection( const QString &dbPath )
    : m_connectionName( QUuid::createUuid().toString() )
    , m_openTransaction( false )
{
    QSqlDatabase db = QSqlDatabase::addDatabase( QStringLiteral("QSQLITE"), m_connectionName );
    db.setDatabaseName( dbPath );
}

ImporterSqlConnection::ImporterSqlConnection()
    : m_connectionName( QUuid::createUuid().toString() )
    , m_openTransaction( false )
{
}

ImporterSqlConnection::~ImporterSqlConnection()
{
    if( isTransaction() )
    {
        QSqlDatabase db = connection();
        if( db.isOpen() )
        {
            warning() << __PRETTY_FUNCTION__ << "Rolling back unfinished transaction for"
                      << "database" << db.databaseName() << "(" << db.hostName() << ":"
                      << db.port() << ")";

            db.rollback();
        }
    }

    QSqlDatabase::removeDatabase( m_connectionName );
}

QSqlDatabase
ImporterSqlConnection::connection()
{
    Q_ASSERT( this->thread() == ThreadWeaver::Thread::currentThread() );
    return QSqlDatabase::database( m_connectionName );
}

bool
ImporterSqlConnection::isTransaction() const
{
    return m_openTransaction;
}

QList<QVariantList>
ImporterSqlConnection::query( const QString &query, const QVariantMap &bindValues,
                              bool* const ok )
{
    QMutexLocker lock( &m_apiMutex );

    QMetaObject::invokeMethod( this, "slotQuery", blockingConnectionType(),
                               Q_ARG( QString, query ), Q_ARG( QVariantMap, bindValues ),
                               Q_ARG( bool* const, ok ) );

    QList<QVariantList> result;
    result.swap( m_result );
    return result;
}

void
ImporterSqlConnection::transaction()
{
    QMutexLocker lock( &m_apiMutex );
    if( isTransaction() )
        return;

    QMetaObject::invokeMethod( this, "slotTransaction", blockingConnectionType() );
    if( isTransaction() ) // keep a lock for the duration of transaction
        m_apiMutex.lock();
}

void
ImporterSqlConnection::rollback()
{
    QMutexLocker lock( &m_apiMutex );
    if( !isTransaction() )
        return;

    QMetaObject::invokeMethod( this, "slotRollback", blockingConnectionType() );
    m_apiMutex.unlock(); // unlock second lock after releasing transaction
}

void
ImporterSqlConnection::commit()
{
    QMutexLocker lock( &m_apiMutex );
    if( !isTransaction() )
        return;

    QMetaObject::invokeMethod( this, "slotCommit", blockingConnectionType() );
    m_apiMutex.unlock(); // unlock second lock after releasing transaction
}

inline Qt::ConnectionType
ImporterSqlConnection::blockingConnectionType() const
{
    return this->thread() == ThreadWeaver::Thread::currentThread()
            ? Qt::DirectConnection : Qt::BlockingQueuedConnection;
}

void
ImporterSqlConnection::slotQuery( const QString &query, const QVariantMap &bindValues,
                                  bool* const ok )
{
    Q_ASSERT( this->thread() == ThreadWeaver::Thread::currentThread() );

    if( ok != nullptr )
        *ok = false;

    QSqlDatabase db = connection();
    if( !db.isOpen() )
        return;

    QSqlQuery q( db );
    q.setForwardOnly( true );
    q.prepare( query );
    for( QVariantMap::ConstIterator bindValue = bindValues.constBegin();
                                     bindValue != bindValues.constEnd(); ++bindValue )
        q.bindValue( bindValue.key(), bindValue.value() );

    if( q.exec() )
    {
        if( ok != nullptr )
            *ok = true;

        m_result.reserve( q.size() );
        while( q.next() )
        {
            const int fields = q.record().count();

            QVariantList row;
            row.reserve( fields );
            for( int field = 0; field < fields; ++field )
                row.append( q.value( field ) );

            m_result.append( row );
        }
    }
    else
        warning() << __PRETTY_FUNCTION__ << q.lastError().text();

    // This is a stupid QSqlDatabase connection manager; we don't want to leave connection
    // open unless we're inside a transaction.
    if( !isTransaction() )
        db.close();
}

void
ImporterSqlConnection::slotTransaction()
{
    Q_ASSERT( this->thread() == ThreadWeaver::Thread::currentThread() );

    if( isTransaction() )
        return;

    QSqlDatabase db = connection();
    if( db.isOpen() )
    {
        if( db.driver()->hasFeature( QSqlDriver::Transactions ) && db.transaction() )
            m_openTransaction = true;
        else
            db.close();
    }
}

void
ImporterSqlConnection::slotRollback()
{
    Q_ASSERT( this->thread() == ThreadWeaver::Thread::currentThread() );

    if( !isTransaction() )
        return;

    QSqlDatabase db = connection();
    if( db.isOpen() )
    {
        db.rollback();
        db.close();
    }

    m_openTransaction = false;
}

void
ImporterSqlConnection::slotCommit()
{
    Q_ASSERT( this->thread() == ThreadWeaver::Thread::currentThread() );

    if( !isTransaction() )
        return;

    QSqlDatabase db = connection();
    if( db.isOpen() )
    {
        db.commit();
        db.close();
    }

    m_openTransaction = false;
}
