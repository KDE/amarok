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

#ifndef AMAROK_COLLECTION_SQLITECOLLECTION_FACTORY_H
#define AMAROK_COLLECTION_SQLITECOLLECTION_FACTORY_H

#include "core/collections/Collection.h"

namespace Collections {

class SqliteCollectionFactory : public Collections::CollectionFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_collection-sqlitecollection.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        SqliteCollectionFactory()
            : Collections::CollectionFactory() {}

        ~SqliteCollectionFactory() override {}

        void init() override;
};

} //namespace Collections

#endif
