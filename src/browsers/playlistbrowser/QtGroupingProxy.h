/* This file is part of the KDE project
   Copyright (C) 2009 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef GROUPINGPROXY_H
#define GROUPINGPROXY_H

#include <QAbstractProxyModel>
#include <QModelIndex>
#include <QMultiHash>
#include <QStringList>
#include <QIcon>

typedef QMap<int, QVariant> RoleVariantMap;
typedef QMap<int, RoleVariantMap> ColumnVariantMap;

static bool compareColumnVariantMap( const ColumnVariantMap &a, const ColumnVariantMap &b );

class QtGroupingProxy : public QAbstractProxyModel
{
    Q_OBJECT
    public:
        explicit QtGroupingProxy( QAbstractItemModel *model, QModelIndex rootNode = QModelIndex(),
                                  int groupedColumn = -1 );
        ~QtGroupingProxy();

        void setGroupedColumn( int groupedColumn );

        /* QAbstractProxyModel methods */
        virtual QModelIndex index( int, int c = 0,
                                   const QModelIndex& parent = QModelIndex() ) const;
        virtual Qt::ItemFlags flags( const QModelIndex &idx ) const;
        virtual QModelIndex parent( const QModelIndex &idx ) const;
        virtual int rowCount( const QModelIndex &idx = QModelIndex() ) const;
        virtual int columnCount( const QModelIndex &idx ) const;
        virtual QModelIndex mapToSource( const QModelIndex &idx ) const;
        virtual QModelIndexList mapToSource( const QModelIndexList &list ) const;
        virtual QModelIndex mapFromSource( const QModelIndex &idx ) const;
        virtual QVariant data( const QModelIndex &idx, int role ) const;
        virtual bool setData( const QModelIndex &index, const QVariant &value,
                              int role = Qt::EditRole );
        virtual QVariant headerData ( int section, Qt::Orientation orientation,
                                      int role ) const;
        virtual bool canFetchMore( const QModelIndex &parent ) const;
        virtual void fetchMore( const QModelIndex &parent );
        virtual bool hasChildren( const QModelIndex &parent = QModelIndex() ) const;

        /* QtGroupingProxy methods */
        virtual QModelIndex addEmptyGroup( const ColumnVariantMap &data );

    signals:
        void rowsInserted( const QModelIndex&, int, int );
        void rowsRemoved( const QModelIndex&, int, int );
        void layoutAboutToBeChanged();
        void layoutChanged();

    protected slots:
        void buildTree();

    private slots:
        void modelDataChanged( const QModelIndex&, const QModelIndex& );
        void modelRowsInserted( const QModelIndex&, int, int );
        void modelRowsRemoved( const QModelIndex&, int, int );

        void slotDeleteGroup();
        void slotRenameGroup();

    protected:
        /** Maps an item to a group.
          * The return value is a list because an item can put in multiple groups.
          * Inside the list is a 2 dimensional map.
          * Mapped to column-number is another map of role-number to QVariant.
          * This data prepolulates the group-data cache. The rest is gathered on demand
          * from the children of the group.
          */
        virtual QList<ColumnVariantMap> belongsTo( const QModelIndex &idx );
        
        bool isGroup( const QModelIndex &index ) const;
        bool isAGroupSelected( const QModelIndexList& list ) const;

        QAbstractItemModel *m_model;

    private:
        QMultiHash<quint32, int> m_groupHash;
        QList<ColumnVariantMap> m_groupMaps;

        /** "instuctions" how to create an item in the tree.
        This is used by parent( QModelIndex )
        */
        struct ParentCreate
        {
            int parentCreateIndex;
            int row;
        };
        mutable QList<struct ParentCreate> m_parentCreateList;
        /** @returns index of the "instructions" to recreate the parent. Will create new if it doesn't exist yet.
        */
        int indexOfParentCreate( const QModelIndex &parent ) const;

        QModelIndexList m_selectedGroups;

        QModelIndex m_rootNode;
        int m_groupedColumn;
        QVariant m_folderIcon;
        void dumpGroups();
};

#endif //GROUPINGPROXY_H
