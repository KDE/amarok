//
// C++ Interface: collectionsortfilterproxymodel
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef COLLECTIONSORTFILTERPROXYMODEL_H
#define COLLECTIONSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

/**
This is a custom QSortFilterProxyModel that solves some issues with sorting a model that usses lazy loading

	@author 
*/
class CollectionSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    CollectionSortFilterProxyModel( QObject * parent = 0 );

    ~CollectionSortFilterProxyModel();

    bool hasChildren(const QModelIndex &parent) const;


};

#endif
