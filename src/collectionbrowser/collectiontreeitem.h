 /*
  Copyright (c) 2007  Alexandre Pereira de Oliveira <aleprj@gmail.com>

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
#include "sqlmeta.h"

#include <QList>


class CollectionTreeItem {

    public:
        CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent );

        ~CollectionTreeItem();

        CollectionTreeItem* parent()  { return m_parent; }

        void appendChild(CollectionTreeItem *child);

        CollectionTreeItem *child(int row) { return m_childItems.value(row); }

        int childCount() const { return m_childItems.count(); }
        int columnCount() const { return 1; }

        QVariant data(int column) const;

        int row() const;

        int level() const;

        QueryBuilder queryBuilder() const;

/*        bool childrenSet() { return  m_childrenSet; }
        void setChildrenSet( bool childrenSet ) { m_childrenSet = childrenSet; } */

    private:
        Meta::DataPtr m_data;
        CollectionTreeItem *m_parent;
        QList<CollectionTreeItem*> m_childItems;
        //bool m_childrenSet;
};


#endif
