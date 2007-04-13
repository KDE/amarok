 /*
  Copyright (c) 2007  Alexandre Pereira de Oliveira <aleprj@gmail.com>
  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef COLLECTIONTREEITEM_H
#define COLLECTIONTREEITEM_H

#include "meta.h"
//#include "sqlmeta.h"
#include "querybuilder.h"
#include "collection.h"

#include <QList>

namespace CustomRoles
{
    enum CustomRolesId {
    SortRole = Qt::UserRole + 1,
    FilterRole = Qt::UserRole + 2
    };
}

class CollectionTreeItem {
    public:
        CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent );
        CollectionTreeItem( Collection *parentCollection, CollectionTreeItem *parent );

        ~CollectionTreeItem();

        CollectionTreeItem* parent()  { return m_parent; }

        void appendChild(CollectionTreeItem *child);

        CollectionTreeItem *child(int row);

        int childCount() const { return m_childItems.count(); }
        int columnCount() const { return 1; }

        QVariant data(int role) const;

        int row() const;

        int level() const;

        bool isDataItem() const;

        QueryMaker* queryMaker() const;

        bool operator<( const CollectionTreeItem& other ) const;
        bool childrenLoaded() const { return  m_childrenLoaded; }
        void setChildrenLoaded( bool childrenLoaded ) { m_childrenLoaded = childrenLoaded; }

        const Meta::DataPtr data() const { return m_data; }
        Collection* parentCollection() const { return m_parentCollection; }

        KUrl::List urls() const;

    private:
        Meta::DataPtr m_data;
        CollectionTreeItem *m_parent;
        Collection* m_parentCollection;

        QList<CollectionTreeItem*> m_childItems;
        bool m_childrenLoaded;
};

#endif
