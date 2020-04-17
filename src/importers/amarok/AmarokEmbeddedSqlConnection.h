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

#ifndef STATSYNCING_AMAROK_EMBEDDED_SQL_CONNECTION_H
#define STATSYNCING_AMAROK_EMBEDDED_SQL_CONNECTION_H

#include "importers/ImporterSqlConnection.h"

#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QProcess>
#include <QTimer>

namespace StatSyncing
{

class AmarokEmbeddedSqlConnection : public ImporterSqlConnection
{
    Q_OBJECT

public:
    AmarokEmbeddedSqlConnection( const QFileInfo &mysqld, const QDir &datadir );
    ~AmarokEmbeddedSqlConnection() override;

protected:
    QSqlDatabase connection() override;

private:
    bool startServer( const int port, const QString &socketPath, const QString &pidPath );

    const QFileInfo m_mysqld;
    const QDir m_datadir;

    QProcess m_srv;
    QMutex m_srvMutex;
    QTimer m_shutdownTimer;

    /// Number of msecs after which server will shut down
    static const int SERVER_SHUTDOWN_AFTER = 30000;
    /// Number of msecs to wait for server to start up
    static const int SERVER_START_TIMEOUT = 30000;

private Q_SLOTS:
    void stopServer();
};

} // namespace StatSyncing

#endif // STATSYNCING_AMAROK_EMBEDDED_SQL_CONNECTION_H
