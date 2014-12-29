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

#include "MySqlEmbeddedStorageFactory.h"
#include "MySqlEmbeddedStorage.h"

#include <core/support/Amarok.h>

AMAROK_EXPORT_STORAGE( MySqleStorageFactory, mysqlestorage )

MySqleStorageFactory::MySqleStorageFactory( QObject *parent, const QVariantList &args )
    : StorageFactory( parent, args )
{
    m_info = KPluginInfo( "amarok_storage-mysqlestorage.desktop", "services" );
}

MySqleStorageFactory::~MySqleStorageFactory()
{
}

void
MySqleStorageFactory::init()
{
    if( m_initialized )
        return;

    m_initialized = true;

    if( ! Amarok::config( "MySQL" ).readEntry( "UseServer", false ) )
        emit newStorage( new MySqlEmbeddedStorage() );
}

#include "MySqlEmbeddedStorageFactory.moc"

