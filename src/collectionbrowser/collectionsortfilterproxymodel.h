/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef COLLECTIONSORTFILTERPROXYMODEL_H
#define COLLECTIONSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

/**
This is a custom QSortFilterProxyModel that solves some issues with sorting a model that usses lazy loading

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class CollectionSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    CollectionSortFilterProxyModel( QObject * parent = 0 );

    virtual ~CollectionSortFilterProxyModel();

    bool hasChildren(const QModelIndex &parent) const;


};

#endif
