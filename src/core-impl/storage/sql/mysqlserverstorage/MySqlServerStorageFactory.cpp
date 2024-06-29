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


MySqlServerStorageFactory::MySqlServerStorageFactory()
    : StorageFactory()
{}

MySqlServerStorageFactory::~MySqlServerStorageFactory()
{
}

void
MySqlServerStorageFactory::init()
{
    if( m_initialized )
        return;

    m_initialized = true;

    if( Amarok::config( QStringLiteral("MySQL") ).readEntry( "UseServer", false ) )
    {
        auto storage = QSharedPointer<MySqlServerStorage>::create();
        bool initResult = storage->init( Amarok::config( QStringLiteral("MySQL") ).readEntry( "Host", "localhost" ),
                                         Amarok::config( QStringLiteral("MySQL") ).readEntry( "User", "amarokuser" ),
                                         Amarok::config( QStringLiteral("MySQL") ).readEntry( "Password", "password" ),
                                         Amarok::config( QStringLiteral("MySQL") ).readEntry( "Port", "3306" ).toInt(),
                                         Amarok::config( QStringLiteral("MySQL") ).readEntry( "Database", "amarokdb" ) );

        // handle errors during creation
        if( !storage->getLastErrors().isEmpty() )
            Q_EMIT newError( storage->getLastErrors() );
        storage->clearLastErrors();

        if( initResult )
            Q_EMIT newStorage( storage );
    }
}

QStringList
MySqlServerStorageFactory::testSettings( const QString &host, const QString &user, const QString &password, int port, const QString &databaseName )
{
    QStringList errors;

    MySqlServerStorage* storage = new MySqlServerStorage();
    bool initResult = storage->init( host, user, password, port, databaseName );
    Q_UNUSED( initResult );

    // we are just interested in the errors.
    errors = storage->getLastErrors();

    delete storage;

    return errors;
}
