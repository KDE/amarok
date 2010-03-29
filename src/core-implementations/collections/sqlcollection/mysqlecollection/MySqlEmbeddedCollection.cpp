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

#include "MySqlEmbeddedCollection.h"

#include "MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlCollectionFactory.h"

#include <KLocale>

using namespace Collections;

AMAROK_EXPORT_COLLECTION( MySqlEmbeddedCollectionFactory, mysqlecollection )

void
MySqlEmbeddedCollectionFactory::init()
{
    SqlCollectionFactory fac;
    SqlStorage *storage = new MySqlEmbeddedStorage();
    SqlCollection *collection = fac.createSqlCollection( "localCollection", i18n( "Local Collection" ), storage );

    emit newCollection( collection );
}

#include "MySqlEmbeddedCollection.moc"

