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

#ifndef COLLECTIONTREEITEMMODELBASE_H
#define COLLECTIONTREEITEMMODELBASE_H

#include "amarok_export.h"

#include "core/meta/Meta.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QPair>
#include <QSet>

class Collection;
class CollectionTreeItem;
class QTimeLine;

namespace Collections {
    class QueryMaker;
}

typedef QPair<Collections::Collection*, CollectionTreeItem* > CollectionRoot;

namespace CategoryId
{
    enum CatMenuId {
    None = 0,
    Album,
    Artist,
    Composer,
    Genre,
    Year
    };
}

enum {
    AlternateCollectionRowRole = Qt::UserRole + 20
};



/**
	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROK_EXPORT CollectionTreeItemModelBase : public QAbstractItemModel
{
        Q_OBJECT

    public:
        CollectionTreeItemModelBase( );
        virtual ~CollectionTreeItemModelBase();

        virtual Qt::ItemFlags flags(const QModelIndex &index) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        virtual QModelIndex index(int row, int column,
                        const QModelIndex &parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex &index) const;
        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
        virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
        
        // Writable..
        virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

        virtual QStringList mimeTypes() const;
        virtual QMimeData* mimeData( const QList<CollectionTreeItem*> &items ) const;
        virtual QMimeData* mimeData( const QModelIndexList &indices ) const;

        virtual QPixmap iconForLevel( int level ) const;
        virtual void listForLevel( int level, Collections::QueryMaker *qm, CollectionTreeItem* parent );


        virtual void setLevels( const QList<int> &levelType ) = 0;
        virtual QList<int> levels() const { return m_levelType; }

        virtual void addFilters( Collections::QueryMaker *qm ) const;

        void setCurrentFilter( const QString &filter );

        void itemAboutToBeDeleted( CollectionTreeItem *item );

    signals:
        void expandIndex( const QModelIndex &index );
        void queryFinished();

    public slots:
        virtual void queryDone();
        virtual void newResultReady( const QString &collectionId, Meta::DataList data );

        void slotFilter();

        void slotCollapsed( const QModelIndex &index );
    
        void slotExpanded( const QModelIndex &index );

    private:
        void handleCompilationQueryResult( Collections::QueryMaker *qm, const Meta::DataList &dataList );
        void handleNormalQueryResult( Collections::QueryMaker *qm, const Meta::DataList &dataList );

    protected:
        virtual void populateChildren(const Meta::DataList &dataList, CollectionTreeItem *parent, const QModelIndex &parentIndex );
        virtual void updateHeaderText();
        virtual QString nameForLevel( int level ) const;
        virtual int levelModifier() const = 0;

        virtual void filterChildren() = 0;
        void ensureChildrenLoaded( CollectionTreeItem *item );

        void markSubTreeAsDirty( CollectionTreeItem *item );

        void handleCompilations( CollectionTreeItem *parent ) const;

        QString m_headerText;
        CollectionTreeItem *m_rootItem;
        QList<int> m_levelType;

        class Private;
        Private * const d;

        QTimeLine *m_timeLine;
        int m_animFrame;
        QPixmap m_loading1, m_loading2, m_currentAnimPixmap;    //icons for loading animation

        QString m_currentFilter;
        QSet<Meta::DataPtr> m_expandedItems;
        QSet<Collections::Collection*> m_expandedCollections;
        QSet<Collections::Collection*> m_expandedVariousArtistsNodes;
        
    protected slots:
        void startAnimationTick();
        void loadingAnimationTick();
        void update();
};

class CollectionTreeItemModelBase::Private
{
 public:
    QHash<QString, CollectionRoot > m_collections;  //I'll concide this one... :-)
    QHash<Collections::QueryMaker* , CollectionTreeItem* > m_childQueries;
    QHash<Collections::QueryMaker* , CollectionTreeItem* > m_compilationQueries;
    QHash<CollectionTreeItem*, Collections::QueryMaker*> m_runningQueries;
};

#endif
