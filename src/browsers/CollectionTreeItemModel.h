/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef COLLECTIONTREEITEMMODEL_H
#define COLLECTIONTREEITEMMODEL_H

#include "CollectionTreeItemModelBase.h"
#include "meta/Meta.h"


#include <QMap>
#include <QPair>

class CollectionTreeItem;
class Collection;

class CollectionTreeItemModel: public CollectionTreeItemModelBase
{
        Q_OBJECT

    public:
        CollectionTreeItemModel( const QList<int> &levelType );
        ~CollectionTreeItemModel();

        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
        virtual bool canFetchMore( const QModelIndex &parent ) const;
        virtual void fetchMore( const QModelIndex &parent );
        virtual void setLevels( const QList<int> &levelType );

    public slots:
        virtual void collectionAdded( Amarok::Collection *newCollection );
        virtual void collectionRemoved( const QString &collectionId );

    protected:
        virtual void filterChildren();

    private slots:
        virtual void requestCollectionsExpansion();
        void update();

    private:
        void ensureChildrenLoaded( CollectionTreeItem *item ) const;
};

#endif
