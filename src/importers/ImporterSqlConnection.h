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

#ifndef STATSYNCING_IMPORTER_SQL_CONNECTION_H
#define STATSYNCING_IMPORTER_SQL_CONNECTION_H

#include <QObject>

#include "amarok_export.h"

#include <QList>
#include <QRecursiveMutex>
#include <QPointer>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace StatSyncing
{

/**
 * A helper class encapsulating communication with the database. It guarantees that
 * a connection is only used in thread that created it, as per
 * http://doc.qt.io/qt-5/threads-modules.html#threads-and-the-sql-module
 * This class is very basic, e.g. returns a whole query result as a list of lists,
 * so it may not be suitable for more advanced usage.
 */
class AMAROK_EXPORT ImporterSqlConnection : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor. Creates a new external database connection characterized by parameters
     * given. See @see QSqlDatabase class for details.
     */
    ImporterSqlConnection( const QString &driver, const QString &hostname,
                           const quint16 port, const QString &dbName, const QString &user,
                           const QString &password );

    /**
     * Constructor. Overload for creating a connection to SQLite database.
     */
    explicit ImporterSqlConnection( const QString &dbPath );

    /**
     * Destructor. Removes connection created in the constructor. If a transaction was
     * in progress it is rolled back.
     */
    ~ImporterSqlConnection() override;

    /**
     * Executes a query given in QString @p query, first using QSqlQuery::bindValue
     * to bind parameters given in @p bindValues. If @p ok is not null the bool
     * it points at is set to true if the query was successful and false if error occurred.
     * Note that if no transaction is started, the connection is opened for the query and
     * then closed again.
     * @param query the query
     * @param bindValues the bind parameters
     * @param ok whether the query was successful
     *
     * @returns The result of the query.
     */
    QList<QVariantList> query( const QString &query,
                               const QVariantMap &bindValues = QVariantMap(),
                               bool* const ok = nullptr );

    /**
     * Starts a transaction. Transaction is not started if the underlying driver has no
     * support for it. While transaction is started, the connection to the database
     * remains open and no other thread can use the ImporterSqlConnection object's api.
     */
    void transaction();

    /**
     * Rolls the transaction back. Nothing happens if the transaction was not started.
     * This method must be used from the same thread that called @see transaction() .
     */
    void rollback();

    /**
     * Commits the transaction. Nothing happens if the transaction was not started.
     * This method must be used from the same thread that called @see transaction() .
     */
    void commit();

protected:
    /**
     * Constructor. Use this overload if you don't want to create a new connection in
     * constructor of a derived class.
     */
    ImporterSqlConnection();

    /**
     * Opens and returns current QSqlDatabase connection. Override this method if you need
     * to do something before the connection is used. This method should only be called
     * from object's main thread.
     */
    virtual QSqlDatabase connection();

    /**
     * Returns true if transaction is in progress. This method should only be called
     * from object's main thread.
     */
    bool isTransaction() const;

    /**
     * Name of the current connection.
     */
    const QString m_connectionName;

private:
    Q_DISABLE_COPY( ImporterSqlConnection )

    Qt::ConnectionType blockingConnectionType() const;

    QRecursiveMutex m_apiMutex;
    bool m_openTransaction;
    QList<QVariantList> m_result;

private Q_SLOTS:
    void slotQuery( const QString &query, const QVariantMap &bindValues, bool* const ok );
    void slotTransaction();
    void slotRollback();
    void slotCommit();
};

typedef QSharedPointer<ImporterSqlConnection> ImporterSqlConnectionPtr;

} // namespace StatSyncing

#endif // STATSYNCING_IMPORTER_SQL_CONNECTION_H
