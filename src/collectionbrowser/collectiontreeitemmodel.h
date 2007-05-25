/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com> *
 *            (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>         *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#ifndef COLLECTIONTREEITEMMODEL_H
#define COLLECTIONTREEITEMMODEL_H

#include "CollectionTreeItemModelBase.h"
#include "meta.h"
#include "../querybuilder.h"


#include <QMap>
#include <QPair>

using namespace Meta;

class CollectionTreeItem;
class Collection;
class QueryMaker;


class CollectionTreeItemModel: public CollectionTreeItemModelBase {
Q_OBJECT

    public:
        CollectionTreeItemModel( const QList<int> &levelType );

        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
        virtual bool canFetchMore( const QModelIndex &parent ) const;
        virtual void fetchMore( const QModelIndex &parent );
        virtual void setLevels( const QList<int> &levelType );


    public slots:
        virtual void collectionAdded( Collection *newCollection );
        virtual void collectionRemoved( const QString &collectionId );


    private:

        void ensureChildrenLoaded( CollectionTreeItem *item ) const;


};

#endif
