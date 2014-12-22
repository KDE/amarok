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

AMAROK_EXPORT_STORAGE( MySqlServerStorageFactory, mysqlserverstorage )

MySqlServerStorageFactory::MySqlServerStorageFactory( QObject *parent, const QVariantList &args )
    : StorageFactory( parent, args )
{
    m_info = KPluginInfo( "amarok_storage-mysqlserverstorage.desktop", "services" );
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

    emit newStorage( new MySqlServerStorage() );
}

#include "MySqlServerStorageFactory.moc"

