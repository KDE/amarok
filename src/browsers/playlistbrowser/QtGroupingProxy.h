/****************************************************************************************
 * Copyright (c) 2007-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#ifndef GROUPINGPROXY_H
#define GROUPINGPROXY_H

#include <QAbstractProxyModel>
#include <QModelIndex>

typedef QMap<int, QVariant> ItemData;
typedef QMap<int, ItemData> RowData;


class QtGroupingProxy : public QAbstractProxyModel
{
    Q_OBJECT
    public:
        explicit QtGroupingProxy( QObject *parent = nullptr );
        explicit QtGroupingProxy(QAbstractItemModel *model, const QModelIndex &rootIndex = QModelIndex(),
                                  int groupedColumn = -1, QObject *parent = nullptr );
        ~QtGroupingProxy() override;

        /* QtGroupingProxy methods */
        void setRootIndex( const QModelIndex &rootIndex );
        void setGroupedColumn( int groupedColumn );
        virtual QModelIndex addEmptyGroup( const RowData &data );
        virtual bool removeGroup( const QModelIndex &idx );

        /* QAbstractProxyModel methods */
        //re-implemented to connect to source signals
        void setSourceModel( QAbstractItemModel *sourceModel ) override;
        QModelIndex index( int row, int column = 0,
                                   const QModelIndex& parent = QModelIndex() ) const override;
        Qt::ItemFlags flags( const QModelIndex &idx ) const override;
        QModelIndex buddy( const QModelIndex &index ) const override;
        QModelIndex parent( const QModelIndex &idx ) const override;
        int rowCount( const QModelIndex &idx = QModelIndex() ) const override;
        int columnCount( const QModelIndex &idx ) const override;
        QModelIndex mapToSource( const QModelIndex &idx ) const override;
        virtual QModelIndexList mapToSource( const QModelIndexList &list ) const;
        QModelIndex mapFromSource( const QModelIndex &idx ) const override;
        QVariant data( const QModelIndex &idx, int role ) const override;
        bool setData( const QModelIndex &index, const QVariant &value,
                              int role = Qt::EditRole ) override;
        QVariant headerData ( int section, Qt::Orientation orientation,
                                      int role ) const override;
        bool canFetchMore( const QModelIndex &parent ) const override;
        void fetchMore( const QModelIndex &parent ) override;
        bool hasChildren( const QModelIndex &parent = QModelIndex() ) const override;

    Q_SIGNALS:
        void renameIndex( const QModelIndex &idx );

    protected Q_SLOTS:
        virtual void buildTree();

    private Q_SLOTS:
        void modelDataChanged( const QModelIndex &, const QModelIndex & );
        void modelRowsInserted( const QModelIndex &, int, int );
        void modelRowsAboutToBeInserted( const QModelIndex &, int ,int );
        void modelRowsRemoved( const QModelIndex &, int, int );
        void modelRowsAboutToBeRemoved( const QModelIndex &, int ,int );

    protected:
        /** Maps an item to a group.
          * The return value is a list because an item can put in multiple groups.
          * Inside the list is a 2 dimensional map.
          * Mapped to column-number is another map of role-number to QVariant.
          * This data prepolulates the group-data cache. The rest is gathered on demand
          * from the children of the group.
          */
        virtual QList<RowData> belongsTo( const QModelIndex &idx );

        /**
          * calls belongsTo(), checks cached data and adds the index to existing or new groups.
          * @returns the groups this index was added to where -1 means it was added to the root.
          */
        QList<int> addSourceRow( const QModelIndex &idx );
        
        bool isGroup( const QModelIndex &index ) const;
        bool isAGroupSelected( const QModelIndexList &list ) const;

        /** Maintains the group -> sourcemodel row mapping
          * The reason a QList<int> is use instead of a QMultiHash is that the values have to be
          * reordered when rows are inserted or removed.
          * TODO:use some auto-incrementing container class (steveire's?) for the list
          * NOTE: due to functionality changes in Qt6, renamed from m_groupHash -> m_groupMap,
          * even though there is also a m_groupMaps. Confuse warning.
          */
        QMap<quint32, QList<int> > m_groupMap;
        /** The data cache of the groups.
          * This can be pre-loaded with data in belongsTo()
          */
        QList<RowData> m_groupMaps;

        /** "instructions" how to create an item in the tree.
          * This is used by parent( QModelIndex )
        */
        struct ParentCreate
        {
            quintptr parentCreateIndex;
            int row;
        };
        mutable QList<struct ParentCreate> m_parentCreateList;
        /** @returns index of the "instructions" to recreate the parent. Will create new if it doesn't exist yet.
        */
        int indexOfParentCreate( const QModelIndex &parent ) const;

        QModelIndexList m_selectedGroups;

        QModelIndex m_rootIndex;
        int m_groupedColumn;

};

#endif //GROUPINGPROXY_H
