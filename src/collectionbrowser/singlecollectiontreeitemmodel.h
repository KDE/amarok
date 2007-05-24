/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com> *
 *            (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>         *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#ifndef SINGLECOLLECTIONTREEITEMMODEL_H
#define SINGLECOLLECTIONTREEITEMMODEL_H

#include "collectiontreeitemmodel.h"
#include "meta.h"
#include "../querybuilder.h"
//#include "collection/collection.h"

#include <QAbstractItemModel>
#include <QMap>
#include <QPair>

using namespace Meta;

class CollectionTreeItem;
class Collection;
//typedef QPair<Collection*, CollectionTreeItem* > CollectionRoot;

class SingleCollectionTreeItemModel: public CollectionTreeItemModel {
Q_OBJECT

    public:
        SingleCollectionTreeItemModel( Collection * collection,  const QList<int> &levelType );
        virtual ~SingleCollectionTreeItemModel();

        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        virtual QModelIndex index(int row, int column,
                        const QModelIndex &parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex &index) const;
        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

        virtual QMimeData* mimeData( const QModelIndexList &indices ) const;

        virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;

        virtual bool canFetchMore( const QModelIndex &parent ) const;
        virtual void fetchMore( const QModelIndex &parent );


        virtual QPixmap iconForLevel( int level ) const;
        virtual void listForLevel( int level, QueryMaker *qm, CollectionTreeItem* parent ) const;


        virtual void setLevels( const QList<int> &levelType );
        virtual QList<int> levels() const { return m_levelType; }

    public slots:

        void queryDone();
        void newResultReady( const QString &collectionId, Meta::DataList data );

    private:

        void populateChildren(const DataList &dataList, CollectionTreeItem *parent) const;
        void ensureChildrenLoaded( CollectionTreeItem *item ) const;
        void updateHeaderText();
        QString nameForLevel( int level ) const;

        QString m_headerText;

        Collection* m_collection;
        CollectionTreeItem *m_rootItem;
        CollectionTreeItem *m_itemBeingQueried;

        QueryMaker* m_queryMaker;

        QList<int> m_levelType;

        class Private;
            Private * const d;
};

#endif
