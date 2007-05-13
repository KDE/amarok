//
// C++ Implementation: collectionsortfilterproxymodel
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "collectionsortfilterproxymodel.h"

CollectionSortFilterProxyModel::CollectionSortFilterProxyModel(  QObject * parent )
 : QSortFilterProxyModel( parent )
{
}


CollectionSortFilterProxyModel::~CollectionSortFilterProxyModel()
{
}

bool CollectionSortFilterProxyModel::hasChildren(const QModelIndex & parent) const
{
    QModelIndex sourceParent = mapToSource(parent);
    return sourceModel()->hasChildren(sourceParent);
}


