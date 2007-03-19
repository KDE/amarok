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

#ifndef COLLECTIONTREEITEMMODEL_H
#define COLLECTIONTREEITEMMODEL_H

#include "meta.h"
#include "querybuilder.h"

#include <QAbstractItemModel>


class CollectionTreeItem;

namespace CategoryId
{
    enum CatMenuId {
    Album = QueryBuilder::tabAlbum,
    Artist = QueryBuilder::tabArtist,
    Composer = QueryBuilder::tabComposer,
    Genre = QueryBuilder::tabGenre,
    Year = QueryBuilder::tabYear
    };
}



class CollectionTreeItemModel: public QAbstractItemModel {

    public:
        CollectionTreeItemModel( const QList<int> &levelType );
        virtual ~CollectionTreeItemModel();

        QVariant data(const QModelIndex &index, int role) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        QModelIndex index(int row, int column,
                        const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex &index) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;

        QList<Meta::DataPtr> listForLevel( int level, QueryBuilder qb = QueryBuilder() ) const;

    private:

        void setupModelData(const QList<Meta::DataPtr> &dataList, CollectionTreeItem *parent);

        CollectionTreeItem *m_rootItem;

        QList<int> m_levelType;

};

#endif
