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

#include "collectiontreeitemmodel.h"
#include "collectiontreeitem.h"
#include "collection/sqlregistry.h"
#include "debug.h"
#include "amarok.h"

#include <KLocale>
#include <KIcon>
#include <KIconLoader>
#include <QMimeData>
#include <QPixmap>

CollectionTreeItemModel::CollectionTreeItemModel( const QList<int> &levelType )
    :QAbstractItemModel()
    , m_rootItem( 0 )
{
    setLevels( levelType );
}


CollectionTreeItemModel::~CollectionTreeItemModel() {
    delete m_rootItem;
}


void
CollectionTreeItemModel::setLevels( const QList<int> &levelType ) {
    delete m_rootItem; //clears the whole tree!
    m_levelType = levelType;
    m_rootItem = new CollectionTreeItem( Meta::DataPtr(0), 0 );
    updateHeaderText();
    populateChildren( listForLevel(0), m_rootItem );

    reset(); //resets the whole model, as the data changed
}

QModelIndex
CollectionTreeItemModel::index(int row, int column, const QModelIndex &parent) const
{

    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

    ensureChildrenLoaded( parentItem );
    CollectionTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex
CollectionTreeItemModel::parent(const QModelIndex &index) const
{
     if (!index.isValid())
         return QModelIndex();

     CollectionTreeItem *childItem = static_cast<CollectionTreeItem*>(index.internalPointer());
     CollectionTreeItem *parentItem = childItem->parent();

     if (parentItem == m_rootItem)
         return QModelIndex();

     return createIndex(parentItem->row(), 0, parentItem);
}

int
CollectionTreeItemModel::rowCount(const QModelIndex &parent) const
{
    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

    ensureChildrenLoaded( parentItem );
    return parentItem->childCount();
}

int
CollectionTreeItemModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}


QVariant
CollectionTreeItemModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid())
         return QVariant();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());

    if ( role == Qt::DecorationRole ) {
        int level = item->level();
        if ( level < m_levelType.count() )
            return iconForLevel( item->level() );
    }

    return item->data( role );
}

Qt::ItemFlags
CollectionTreeItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant
CollectionTreeItemModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        if (section == 0)
            return m_headerText;
    return QVariant();
}

QMimeData*
CollectionTreeItemModel::mimeData( const QModelIndexList &indices ) const {
    if ( indices.isEmpty() )
        return 0;

    KUrl::List urls;

    foreach( QModelIndex index, indices ) {
        if (index.isValid()) {
            CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());
            ensureChildrenLoaded( item );
            urls += item->urls();
        }
    }

    QMimeData *mimeData = new QMimeData();
    urls.populateMimeData(mimeData);

    return mimeData;
}

void
CollectionTreeItemModel::populateChildren(const QList<Meta::DataPtr> &dataList, CollectionTreeItem *parent) const {
    foreach( Meta::DataPtr data, dataList ) {
        CollectionTreeItem *item = new CollectionTreeItem( data, parent );
    }
    parent->setChildrenLoaded( true );
}

QList<Meta::DataPtr>
CollectionTreeItemModel::listForLevel( int level, QueryBuilder qb ) const {
    if ( level > m_levelType.count() )
        return QList<Meta::DataPtr>();
    if ( level == m_levelType.count() ) {
        return SqlRegistry::instance()->getTracks( qb );
    }
    switch( m_levelType[level] ) {
        case CategoryId::Album : return SqlRegistry::instance()->getAlbums( qb );
        case CategoryId::Artist : return SqlRegistry::instance()->getArtists( qb );
        case CategoryId::Composer : return SqlRegistry::instance()->getComposers( qb );
        case CategoryId::Genre : return SqlRegistry::instance()->getGenres( qb );
        case CategoryId::Year : return SqlRegistry::instance()->getYears( qb );
        default: return QList<Meta::DataPtr>();
    }
}

void
CollectionTreeItemModel::updateHeaderText() {
    m_headerText.clear();
    for( int i=0; i< m_levelType.count(); ++i ) {
        m_headerText += nameForLevel( i ) + " / ";
    }
    m_headerText.chop( 3 );
}

QString
CollectionTreeItemModel::nameForLevel( int level ) const {
    switch( m_levelType[level] ) {
        case CategoryId::Album : return i18n( "Album" );
        case CategoryId::Artist : return i18n( "Artist" );
        case CategoryId::Composer : return i18n( "Composer" );
        case CategoryId::Genre : return i18n( "Genre" );
        case CategoryId::Year : return i18n( "Year" );
        default: return QString();
    }
}

QPixmap
CollectionTreeItemModel::iconForLevel( int level ) const {
    QString icon;
        switch( m_levelType[level] ) {
        case CategoryId::Album :
            icon = "album";
            break;
        case CategoryId::Artist :
            icon = "artist";
            break;
        case CategoryId::Composer :
            icon = "artist";
            break;

        case CategoryId::Genre :
            icon = "kfm";
            break;

        case CategoryId::Year :
            icon = "clock";
            break;
    }
    return KIconLoader::global()->loadIcon( Amarok::icon( icon ), K3Icon::Toolbar, K3Icon::SizeSmall );
}


bool
CollectionTreeItemModel::hasChildren ( const QModelIndex & parent ) const {
     if (!parent.isValid())
         return true; // must be root item!

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>(parent.internalPointer());
    return item->level() < m_levelType.count();

}

void
CollectionTreeItemModel::ensureChildrenLoaded( CollectionTreeItem *item ) const {
    if ( !item->childrenLoaded() ) {
        QList<Meta::DataPtr> childrenData = listForLevel( item->level() + 1, item->queryBuilder() );
        populateChildren( childrenData, item );
    }
}

