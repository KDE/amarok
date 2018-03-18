/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef COLLECTIONTREEITEMMODEL_H
#define COLLECTIONTREEITEMMODEL_H

#include "CollectionTreeItemModelBase.h"
#include "core/meta/forward_declarations.h"

#include "core/collections/Collection.h"

#include <QMap>
#include <QPair>

class CollectionTreeItem;

class CollectionTreeItemModel: public CollectionTreeItemModelBase
{
        Q_OBJECT

    public:
        explicit CollectionTreeItemModel( const QList<CategoryId::CatMenuId> &levelType );

        /* QAbstractItemModel methods */
        virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
        virtual QVariant data( const QModelIndex &index, int role ) const;
        virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row,
                                  int column, const QModelIndex &parent );
        virtual bool canFetchMore( const QModelIndex &parent ) const;
        virtual void fetchMore( const QModelIndex &parent );
        virtual Qt::DropActions supportedDropActions() const;

    public Q_SLOTS:
        virtual void collectionAdded( Collections::Collection *newCollection );
        virtual void collectionRemoved( const QString &collectionId );

    protected:
        virtual void filterChildren();
        virtual int levelModifier() const { return 0; }

    private Q_SLOTS:
        void requestCollectionsExpansion();
};

#endif
