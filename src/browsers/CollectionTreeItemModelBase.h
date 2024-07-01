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

#include "core/collections/QueryMaker.h"
#include "core/meta/forward_declarations.h"
#include "CollectionTreeItem.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QPair>
#include <QPixmap>
#include <QSet>

namespace Collections
{
    class Collection;
}
class CollectionTreeItem;
class QTimeLine;
class TrackLoaderJob;

typedef QPair<Collections::Collection*, CollectionTreeItem* > CollectionRoot;

/**
	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROK_EXPORT CollectionTreeItemModelBase : public QAbstractItemModel
{
        Q_OBJECT

    friend class TrackLoaderJob;

    public:
        CollectionTreeItemModelBase();
        ~CollectionTreeItemModelBase() override;

        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override;
        QModelIndex index(int row, int column,
                        const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &index) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const override;

        // Writable..
        bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) override;

        QStringList mimeTypes() const override;
        QMimeData* mimeData( const QModelIndexList &indices ) const override;
        virtual QMimeData* mimeData( const QList<CollectionTreeItem *> &items ) const;

        virtual void listForLevel( int level, Collections::QueryMaker *qm, CollectionTreeItem* parent );

        virtual void setLevels( const QList<CategoryId::CatMenuId> &levelType );
        virtual QList<CategoryId::CatMenuId> levels() const { return m_levelType; }
        virtual CategoryId::CatMenuId levelCategory( const int level ) const;

        QString currentFilter() const;
        void setCurrentFilter( const QString &filter );

        void itemAboutToBeDeleted( CollectionTreeItem *item );

        /**
         * This should be called every time a drag enters collection browser
         */
        void setDragSourceCollections( const QSet<Collections::Collection*> &collections );

        /**
         * Return true if there are any queries still running. If this returns true,
         * you can expect allQueriesFinished(bool) signal in some time.
         */
        bool hasRunningQueries() const;

        static QIcon iconForCategory( CategoryId::CatMenuId category );
        static QString nameForCategory( CategoryId::CatMenuId category, bool showYears = false );

        void ensureChildrenLoaded( CollectionTreeItem *item );

        /**
         * Get a pointer to collection tree item given its index. It is not safe to
         * cache this pointer unless QPointer is used.
         */
        CollectionTreeItem *treeItem( const QModelIndex &index ) const;

        /**
         * Get (create) index for a collection tree item. The caller must ensure this
         * item is in this model. Invalid model index is returned on null or root item.
         */
        QModelIndex itemIndex( CollectionTreeItem *item ) const;

    Q_SIGNALS:
        void expandIndex( const QModelIndex &index );
        void allQueriesFinished( bool autoExpand );

    public Q_SLOTS:
        virtual void queryDone();
        void newTracksReady( const Meta::TrackList &);
        void newArtistsReady( const Meta::ArtistList &);
        void newAlbumsReady( const Meta::AlbumList &);
        void newGenresReady( const Meta::GenreList &);
        void newComposersReady( const Meta::ComposerList &);
        void newYearsReady( const Meta::YearList &);
        void newLabelsReady( const Meta::LabelList &);
        virtual void newDataReady( const Meta::DataList &data );

        /**
         * Apply the current filter.
         *
         * @param autoExpand whether to trigger automatic expansion of the tree after
         * filtering is done. This should be set to true only if filter is run after
         * user has actually just typed something and defaults to false.
         */
        void slotFilter( bool autoExpand = false );
        void slotFilterWithoutAutoExpand() { slotFilter( false ); }

        void slotCollapsed( const QModelIndex &index );
        void slotExpanded( const QModelIndex &index );

    private:
        void handleSpecialQueryResult( CollectionTreeItem::Type type, Collections::QueryMaker *qm, const Meta::DataList &dataList );
        void handleNormalQueryResult( Collections::QueryMaker *qm, const Meta::DataList &dataList );

        Collections::QueryMaker::QueryType mapCategoryToQueryType( int levelType ) const;
        void tracksLoaded( const Meta::AlbumPtr &album, const QModelIndex &index, const Meta::TrackList &tracks );

        QHash<Meta::Album *, int> m_years;
        mutable QSet<Meta::AlbumPtr> m_loadingAlbums;

    protected:
        /** Adds the query maker to the running queries and connects the slots */
        void addQueryMaker( CollectionTreeItem* item,
                            Collections::QueryMaker *qm ) const;

        virtual void populateChildren(const Meta::DataList &dataList, CollectionTreeItem *parent, const QModelIndex &parentIndex );
        virtual void updateHeaderText();

        virtual QIcon iconForLevel( int level ) const;
        virtual QString nameForLevel( int level ) const;

        virtual int levelModifier() const = 0;
        virtual QVariant dataForItem( CollectionTreeItem *item, int role, int level = -1 ) const;

        virtual void filterChildren() = 0;

        void markSubTreeAsDirty( CollectionTreeItem *item );

        /** Initiates a special search for albums without artists */
        void handleCompilations( Collections::QueryMaker::QueryType queryType, CollectionTreeItem *parent ) const;

        /** Initiates a special search for tracks without label */
        void handleTracksWithoutLabels( Collections::QueryMaker::QueryType queryType, CollectionTreeItem *parent ) const;

        QString m_headerText;
        CollectionTreeItem *m_rootItem;
        QList<CategoryId::CatMenuId> m_levelType;

        QTimeLine *m_timeLine;
        int m_animFrame;
        QPixmap m_loading1, m_loading2, m_currentAnimPixmap;    //icons for loading animation

        QString m_currentFilter;
        QSet<Meta::DataPtr> m_expandedItems;
        QSet<Collections::Collection*> m_expandedCollections;
        QSet<Collections::Collection*> m_expandedSpecialNodes;

        /**
         * Contents of this set are undefined if there is no active drag 'n drop operation.
         * Additionally, you may _never_ dereference pointers in this set, just compare
         * them with other pointers
         */
        QSet<Collections::Collection*> m_dragSourceCollections;

        QHash<QString, CollectionRoot > m_collections;  //I'll concide this one... :-)
        mutable QHash<Collections::QueryMaker* , CollectionTreeItem* > m_childQueries;
        mutable QHash<Collections::QueryMaker* , CollectionTreeItem* > m_compilationQueries;
        mutable QHash<Collections::QueryMaker* , CollectionTreeItem* > m_noLabelsQueries;
        mutable QMultiHash<CollectionTreeItem*, Collections::QueryMaker*> m_runningQueries;
        bool m_autoExpand; // whether to expand tree after queries are done

    protected Q_SLOTS:
        void startAnimationTick();
        void loadingAnimationTick();
};


#endif
