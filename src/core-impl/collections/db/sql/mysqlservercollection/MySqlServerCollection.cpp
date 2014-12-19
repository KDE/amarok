/****************************************************************************************
 * Copyright (c) 2008 Edward Toroshchin <edward.hades@gmail.com>                        *
 * Copyright (c) 2009 Jeff Mitchell <mitchell@kde.org>                                  *
 * Copyright (c) 2012 Lachlan Dufton <dufton@gmail.com>                                 *
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

#include "MySqlServerCollection.h"
#include "MySqlServerStorage.h"

#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "core-impl/collections/db/sql/SqlCollectionFactory.h"

using namespace Collections;

AMAROK_EXPORT_COLLECTION( MySqlServerCollectionFactory, mysqlservercollection )

void
MySqlServerCollectionFactory::init()
{
    if( m_initialized )
        return;

    SqlCollectionFactory fac;
    SqlStorage *storage = new MySqlServerStorage();
    SqlCollection *collection = fac.createSqlCollection( storage );
    m_initialized = true;

    emit newStorage( storage );
    emit newCollection( collection );
}

#include "MySqlServerCollection.moc"

