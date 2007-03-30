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
#ifndef AMAROK_COLLECTION_H
#define AMAROK_COLLECTION_H

#include "amarok.h"
#include "amarok_export.h"
#include "plugin/plugin.h"
#include "querybuilder.h"

#include <QObject>
#include <QString>

class Collection;

class AMAROK_EXPORT CollectionFactory : public Amarok::Plugin, QObject
{
    Q_OBJECT
    public:
        CollectionFactory() {};
        virtual ~CollectionFactory() {};

        virtual void init() = 0;

    public signals:
        void newCollection( Collection *newCollection );

};

class Collection
{
    public:
        virtual QueryBuilder * queryBuilder() = 0;
        virtual void startFullScan() { }

        virtual QString collectionId() = 0;
        virtual QString prettyName() = 0;
}

#endif /* AMAROK_COLLECTION_H */