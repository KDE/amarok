/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef COLLECTIONSORTFILTERPROXYMODEL_H
#define COLLECTIONSORTFILTERPROXYMODEL_H

#include "core/meta/forward_declarations.h"

#include <QSortFilterProxyModel>

class CollectionTreeItem;
class QCollator;

/**
    This is a custom QSortFilterProxyModel that gives special sort orders for
    our meta objects.
    e.g. it sorts tracks by disc number and track number.

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class CollectionSortFilterProxyModel : public QSortFilterProxyModel
{
    public:
        explicit CollectionSortFilterProxyModel( QObject *parent = nullptr );

        ~CollectionSortFilterProxyModel() override;

        bool hasChildren(const QModelIndex &parent) const override;

        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    protected:
        bool lessThan( const QModelIndex &left, const QModelIndex &right ) const override;
        bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override;

    private:
        QCollator *m_col;

        CollectionTreeItem* treeItem( const QModelIndex &index ) const;
        bool lessThanTrack( const QModelIndex &left, const QModelIndex &right ) const;
        bool lessThanAlbum( const QModelIndex &left, const QModelIndex &right ) const;
        bool lessThanItem( const QModelIndex &left, const QModelIndex &right ) const;
};

#endif
