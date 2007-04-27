/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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
#include "collection.h"

CollectionFactory::CollectionFactory() : Amarok::Plugin()
{
}

CollectionFactory::~CollectionFactory()
{
}


Collection::Collection()
{
}

Collection::~Collection()
{
}

bool
Collection::isSqlDatabase() const
{
    return false;
}

int
Collection::sqlDatabasePriority() const
{
    return 0;
}

QStringList
Collection::query( const QString &query )
{
    Q_UNUSED( query )
    return QStringList();
}

int
Collection::insert( const QString &statement, const QString &table )
{
    Q_UNUSED( statement )
    Q_UNUSED( table )
    return 0;
}

#include "collection.moc"
