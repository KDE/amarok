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

#ifndef AMAROK_STORAGE_MYSQLSERVERSTORAGE_H
#define AMAROK_STORAGE_MYSQLSERVERSTORAGE_H

#include "../amarok_sqlstorage_export.h"
#include "../mysql-shared/MySqlStorage.h"

/**
 * Implements a MySqlStorage using a MySQL Server
 */
class AMAROK_SQLSTORAGE_MYSQLE_EXPORT MySqlServerStorage: public MySqlStorage
{
    public:
        /** Constructor for the server based mysql storage. */
        MySqlServerStorage();
        ~MySqlServerStorage() override;

        /** Try to connect to the server indicated by the options.
         *
         *  Error messages are in the store error log.
         *
         *  @return true if connection works.
         */
        virtual bool init( const QString &host, const QString &user, const QString &password, int port, const QString &databaseName );

        QStringList query( const QString &query ) override;

    private:
        QString m_databaseName; ///< remember the name given at init for reconnects
};

#endif
