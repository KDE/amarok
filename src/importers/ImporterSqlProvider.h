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

#ifndef STATSYNCING_IMPORTER_SQL_PROVIDER_H
#define STATSYNCING_IMPORTER_SQL_PROVIDER_H

#include "importers/ImporterProvider.h"

#include <QSqlDatabase>

namespace StatSyncing
{

/**
 * A convenience class for ImporterProvider implementations using QtSql framework.
 * It guarantees that QSqlDatabase object is used in the right thread, as per
 * @link http://qt-project.org/doc/qt-4.8/threads-modules.html#threads-and-the-sql-module
 */
class AMAROK_EXPORT ImporterSqlProvider : public ImporterProvider
{
    Q_OBJECT

public:
    /**
     * Constructor. Creates database connection from settings stored in @param config .
     * Following settings are used:
     * config["dbDriver"] - QtSql driver name, see
     *         @link http://qt-project.org/doc/qt-4.8/sql-driver.html#supported-databases
     * config["dbName"] - database name
     * config["dbHost"] - hostname
     * config["dbUser"] - username
     * config["dbPass"] - user's password
     * config["dbPort"] - database port
     */
    ImporterSqlProvider( const QVariantMap &config, ImporterManager *importer );
    virtual ~ImporterSqlProvider();

    QSet<QString> artists();
    TrackList artistTracks( const QString &artistName );

protected:
    /**
     * This method must behave in the same way @see StatSyncing::Provider::artists() .
     * db is QSqlDatabase object *connected* to database specified in the config.
     */
    virtual QSet<QString> getArtists( QSqlDatabase db ) = 0;

    /**
     * This method must behave in the same way @see StatSyncing::Provider::artistTracks()
     * db is QSqlDatabase object *connected* to database specified in the config.
     */
    virtual TrackList getArtistTracks( const QString &artistName, QSqlDatabase db ) = 0;

    /**
     * Name of the connection created in the constructor.
     */
    const QString m_connectionName;

private:
    Qt::ConnectionType getConnectionType() const;

    QSet<QString> m_artistsResult;
    TrackList m_artistTracksResult;

private slots:
    void artistsSearch();
    void artistTracksSearch( const QString &artistName );
};

} // namespace StatSyncing

#endif // STATSYNCING_IMPORTER_SQL_PROVIDER_H
