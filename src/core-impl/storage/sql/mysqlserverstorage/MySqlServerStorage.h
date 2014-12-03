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
        /** Connect to the server defined by the configuration options. */
        MySqlServerStorage();
        virtual ~MySqlServerStorage();

        virtual bool init();

        virtual QStringList query( const QString &query );

        /** Returns true if the given settings allow to connect to a sql server. */
        static bool testSettings( const QString &host, const QString &user, const QString &password, int port );
};

#endif
