
/****************************************************************************************
 * Copyright (c) 2014 Ralf Engels <ralf-engels@gmx.de>                                   *
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

#ifndef AMAROK_STORAGE_MYSQLSERVERSTORAGEFACTORY_H
#define AMAROK_STORAGE_MYSQLSERVERSTORAGEFACTORY_H

#include "core/storage/StorageFactory.h"

class MySqlServerStorageFactory : public StorageFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_storage-mysqlserverstorage.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

public:
    MySqlServerStorageFactory();
    ~MySqlServerStorageFactory() override;

    void init() override;

public Q_SLOTS:

    /** Returns the error messages created during establishing the connection.
     */
    QStringList testSettings( const QString &host, const QString &user, const QString &password, int port, const QString &databaseName );
};


#endif
