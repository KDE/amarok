/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com> *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#ifndef COLLECTIONTREEITEMMODEL_H
#define COLLECTIONTREEITEMMODEL_H

#include "meta.h"
#include "../querybuilder.h"

#include <QAbstractItemModel>
#include <QMap>
#include <QPair>

using namespace Meta;

class CollectionTreeItem;
class Collection;
class QueryMaker;
typedef QPair<Collection*, CollectionTreeItem* > CollectionRoot;

namespace CategoryId
{
    enum CatMenuId {
    None = 0,
    Album = QueryBuilder::tabAlbum,
    Artist = QueryBuilder::tabArtist,
    Composer = QueryBuilder::tabComposer,
    Genre = QueryBuilder::tabGenre,
    Year = QueryBuilder::tabYear
    };
}

class CollectionTreeItemModel: public QAbstractItemModel {
Q_OBJECT

    public:
        CollectionTreeItemModel( const QList<int> &levelType );
        virtual ~CollectionTreeItemModel();

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


        QPixmap iconForLevel( int level ) const;
        void listForLevel( int level, QueryMaker *qm, CollectionTreeItem* parent ) const;


        void setLevels( const QList<int> &levelType );
        QList<int> levels() const { return m_levelType; }

    public slots:
        void collectionAdded( Collection *newCollection );
        void collectionRemoved( const QString &collectionId );

        void queryDone();

        void newResultReady( const QString &collectionId, Meta::DataList data );

    protected:
        void addFilters( QueryMaker *qm ) const;

    private:

        void populateChildren(const DataList &dataList, CollectionTreeItem *parent) const;
        void ensureChildrenLoaded( CollectionTreeItem *item ) const;
        void updateHeaderText();
        QString nameForLevel( int level ) const;

        QString m_headerText;


        CollectionTreeItem *m_rootItem;

        QList<int> m_levelType;

        class Private;
        Private * const d;
};

#endif
