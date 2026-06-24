/****************************************************************************************
 * Copyright (c) 2025 Amarok Team <amarok@kde.org>                                     *
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

#include "SqliteCollectionFactory.h"

#include <core-impl/storage/StorageManager.h>
#include <core-impl/collections/db/sql/SqlCollection.h>
#include <core-impl/collections/db/sql/SqlCollectionFactory.h>
#include <core/support/Amarok.h>

#include <KLocalizedString>

using namespace Collections;


void
SqliteCollectionFactory::init()
{
    if( m_initialized )
        return;

    // DatabaseBackend: 0 = Embedded MySQL, 1 = External MySQL, 2 = SQLite
    const int backend = Amarok::config( QStringLiteral("MySQL") ).readEntry( "DatabaseBackend", 0 );
    if( backend != 2 )
        return;

    SqlCollectionFactory fac;
    auto storage = StorageManager::instance()->sqlStorage();
    SqlCollection *collection = fac.createSqlCollection( storage );
    m_initialized = true;

    Q_EMIT newCollection( collection );
}
