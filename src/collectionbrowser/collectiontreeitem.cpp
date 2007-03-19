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

#include "collectiontreeitem.h"

#include "querybuilder.h"

CollectionTreeItem::CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent )
    : m_data( data )
    , m_parent( parent )
    //, m_childrenSet( false )
{
    if ( m_parent )
        m_parent->appendChild( this );
}

CollectionTreeItem::~CollectionTreeItem() {
    qDeleteAll(m_childItems);
}

void
CollectionTreeItem::appendChild(CollectionTreeItem *child) {
    m_childItems.append(child);
}

QVariant
CollectionTreeItem::data(int column) const {
    if ( column == 0 && m_data )
        return m_data->prettyName();
    else
        return QVariant();
}

int
CollectionTreeItem::row() const {
    if (m_parent)
        return m_parent->m_childItems.indexOf(const_cast<CollectionTreeItem*>(this));

    return 0;
}

int
CollectionTreeItem::level() const {
    if ( !m_parent )
        return 0;
    else
        return m_parent->level() + 1;
}

QueryBuilder
CollectionTreeItem::queryBuilder() const {
    QueryBuilder qb = ( !m_parent ? QueryBuilder() : m_parent->queryBuilder() );
    const SqlSearchable *searchable = dynamic_cast<const SqlSearchable*>(m_data.data());
    if ( searchable )
        searchable->addToQueryFilter( qb );
    return qb;
}
