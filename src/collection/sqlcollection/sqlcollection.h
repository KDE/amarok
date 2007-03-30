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
#ifndef AMAROK_COLLECTION_SQLCOLLECTION_H
#define AMAROK_COLLECTION_SQLCOLLECTION_H

#include "collection.h"

class SqlCollectionFactory : public CollectionFactory
{
    Q_OBJECT

    public:
        SqlCollectionFactory() {}
        virtual ~SqlCollectionFactory() {}

        virtual void init();
};

class CollectionDB;

class SqlCollection : public Collection
{
    public:
        SqlCollection();
        virtual ~SqlCollection() {}

        virtual void startFullScan() {} //TODO
        virtual QueryMaker* queryBuilder();

        virtual QString collectionId() const;
        virtual QString prettyName() const;

        QStringList query( const QString &statement );

    private:
        //reuse CollectionDB until we replace it completely
        CollectionDB *m_collectionDb;
};

#endif /* AMAROK_COLLECTION_SQLCOLLECTION_H */

