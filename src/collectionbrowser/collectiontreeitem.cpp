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

#include "collectiontreeitem.h"

#include "querybuilder.h"

#include <KLocale>

#include <QtAlgorithms>

CollectionTreeItem::CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent )
    : m_data( data )
    , m_parent( parent )
    , m_parentCollection( 0 )
    , m_childrenLoaded( false )
{
    if ( m_parent )
        m_parent->appendChild( this );
}

CollectionTreeItem::CollectionTreeItem( Collection *parentCollection, CollectionTreeItem *parent )
    : m_data( Meta::DataPtr( 0 ) )
    , m_parent( parent )
    , m_parentCollection( parentCollection )
    , m_childrenLoaded( false )
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

CollectionTreeItem*
CollectionTreeItem::child( int row ) {
    if ( row >= 0 && row < m_childItems.count() )
        return m_childItems.value(row);
    else
        return 0;
}

QVariant
CollectionTreeItem::data( int role ) const {

    if ( !m_data.isNull() ) {
        if ( role == Qt::DisplayRole || role == CustomRoles::FilterRole ) {
            QString name = m_data->prettyName();
            if ( name.isEmpty() )
                return i18n( "Unknown" );
            return name;
        }
        else if ( role == CustomRoles::SortRole )
            return m_data->sortableName();

        return QVariant();
    }
    else {
        if ( m_parentCollection && ( role == Qt::DisplayRole || role == CustomRoles::FilterRole ) )
            return m_parentCollection->prettyName();

        return QVariant();
    }
}

int
CollectionTreeItem::row() const {
    if (m_parent)
        return m_parent->m_childItems.indexOf( const_cast<CollectionTreeItem*>(this) );

    return 0;
}

int
CollectionTreeItem::level() const {
    if ( !m_parent )
        return -1;
    else
        return m_parent->level() + 1;
}

bool
CollectionTreeItem::isDataItem() const
{
    return !m_data.isNull();
}

QueryMaker*
CollectionTreeItem::queryMaker() const {
    if ( m_data.isNull() )
        return m_parentCollection->queryBuilder();
    else {
        CollectionTreeItem *tmp = m_parent;
        while( tmp->isDataItem() )
            tmp = tmp->parent();
        QueryMaker *qm = tmp->parentCollection()->queryBuilder();
        return qm;
    }
}

KUrl::List CollectionTreeItem::urls() const {
    /*QueryBuilder qb = queryBuilder();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    QStringList values = qb.run();
    KUrl::List list;
    foreach( QString s, values ) {
        list += KUrl( s );
    }
    return list;*/
    KUrl::List list;
    return list;
}

bool
CollectionTreeItem::operator<( const CollectionTreeItem& other ) const {
    return m_data->sortableName() < other.m_data->sortableName();
}
