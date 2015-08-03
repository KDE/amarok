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

#include "MySqlServerStorageFactory.h"
#include "MySqlServerStorage.h"

#include <amarokconfig.h>
#include <core/support/Amarok.h>

AMAROK_EXPORT_STORAGE( MySqlServerStorageFactory, mysqlserverstorage )

MySqlServerStorageFactory::MySqlServerStorageFactory( QObject *parent, const QVariantList &args )
    : StorageFactory( parent, args )
{
    m_info = KPluginInfo( "amarok_storage-mysqlserverstorage.desktop" );
}

MySqlServerStorageFactory::~MySqlServerStorageFactory()
{
}

void
MySqlServerStorageFactory::init()
{
    if( m_initialized )
        return;

    m_initialized = true;

    if( Amarok::config( "MySQL" ).readEntry( "UseServer", false ) )
    {
        MySqlServerStorage* storage = new MySqlServerStorage();
        bool initResult = storage->init(
                Amarok::config( "MySQL" ).readEntry( "Host", "localhost" ),
                Amarok::config( "MySQL" ).readEntry( "User", "amarokuser" ),
                Amarok::config( "MySQL" ).readEntry( "Password", "password" ),
                Amarok::config( "MySQL" ).readEntry( "Port", "3306" ).toInt(),
                Amarok::config( "MySQL" ).readEntry( "Database", "amarokdb" ) );

        // handle errors during creation
        if( !storage->getLastErrors().isEmpty() )
            emit newError( storage->getLastErrors() );
        storage->clearLastErrors();

        if( initResult )
            emit newStorage( storage );
        else
            delete storage;
    }
}

QStringList
MySqlServerStorageFactory::testSettings( const QString &host, const QString &user, const QString &password, int port, const QString &databaseName )
{
    QStringList errors;

    MySqlServerStorage* storage = new MySqlServerStorage();
    bool initResult = storage->init( host, user, password, port, databaseName );

    // we are just interested in the errors.
    errors = storage->getLastErrors();

    delete storage;

    return errors;
}


