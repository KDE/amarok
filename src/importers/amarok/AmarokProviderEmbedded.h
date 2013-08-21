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

#ifndef STATSYNCING_AMAROK_PROVIDER_EMBEDDED_H
#define STATSYNCING_AMAROK_PROVIDER_EMBEDDED_H

#include "AmarokProvider.h"

#include <QMutex>
#include <QProcess>
#include <QTimer>

namespace StatSyncing
{

class AmarokProviderEmbedded : public AmarokProvider
{
    Q_OBJECT

public:
    AmarokProviderEmbedded( const QVariantMap &config, ImporterManager *importer );
    ~AmarokProviderEmbedded();

protected:
    void prepareConnection();

private:
    bool startServer( const int port, const QString &socketPath, const QString &pidPath );

    // Number of msecs after which server will shut down
    static const int SERVER_SHUTDOWN_AFTER = 30000;
    // Number of msecs to wait for server to start up
    static const int SERVER_START_TIMEOUT = 30000;

    QTimer m_shutdownTimer;
    QProcess m_srv;
    QMutex m_srvMutex;

private slots:
    void stopServer();
};

} // namespace StatSyncing

#endif // STATSYNCING_AMAROK_PROVIDER_EMBEDDED_H
