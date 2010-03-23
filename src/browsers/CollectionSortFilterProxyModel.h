/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "core/meta/Meta.h"

#include <QSortFilterProxyModel>

/**
    This is a custom QSortFilterProxyModel that solves some issues with sorting a model that usses lazy loading
    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/

class CollectionTreeItem;

class CollectionSortFilterProxyModel : public QSortFilterProxyModel
{
    public:
        CollectionSortFilterProxyModel( QObject * parent = 0 );

        virtual ~CollectionSortFilterProxyModel();

        bool hasChildren(const QModelIndex &parent) const;

    protected:
        virtual bool lessThan( const QModelIndex &left, const QModelIndex &right ) const;

    private:
        int albumYear( Meta::AlbumPtr album, bool *ok = 0 ) const;
        CollectionTreeItem* treeItem( const QModelIndex &index ) const;
        bool lessThanTrack( const QModelIndex &left, const QModelIndex &right ) const;
        bool lessThanAlbum( const QModelIndex &left, const QModelIndex &right ) const;
        bool lessThanIndex( const QModelIndex &left, const QModelIndex &right ) const;
        bool lessThanString( const QString &a, const QString &b ) const;
};

#endif
