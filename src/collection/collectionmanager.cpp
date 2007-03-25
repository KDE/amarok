/*
 *  Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "collectionmanager.h"

#include "collection.h"
#include "metaquerybuilder.h"

#include <QGlobal>
#include <QList>

struct CollectionManager::Private
{
    QList<Collection*> collections;
};

CollectionManager::CollectionManager()
    : super()
    , d( new Private )
{
    //init collections
}

CollectionManager::~CollectionManager()
{
    foreach( Collection *coll, d->collections )
        delete coll;

    delete d;
}

CollectionManager *
CollectionManager::instance( )
{
    static CollectionManager instance;
    return &instance;
}

void
CollectionManager::startFullScan()
{
    foreach( Collection *coll, d->collections )
    {
        coll->startFullScan();
    }
}

QueryBuilder*
CollectionManager::queryBuilder()
{
    return new MetaQueryBuilder( d->collections );
}