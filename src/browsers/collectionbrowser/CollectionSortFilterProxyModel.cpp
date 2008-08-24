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

#include "CollectionSortFilterProxyModel.h"
#include "CollectionTreeItem.h"

CollectionSortFilterProxyModel::CollectionSortFilterProxyModel(  QObject * parent )
 : QSortFilterProxyModel( parent )
{
    //NOTE: This does not work properly with our lazy loaded model.  Every time we get new data (usually around an expand)
    // the view scrolls to make the selected item visible.  This is probably a bug in qt,
    // but as the view appears to behave without it, I'm just disabling.
//     setDynamicSortFilter( true );
}


CollectionSortFilterProxyModel::~CollectionSortFilterProxyModel()
{
}

bool
CollectionSortFilterProxyModel::hasChildren(const QModelIndex & parent) const
{
    QModelIndex sourceParent = mapToSource(parent);
    return sourceModel()->hasChildren(sourceParent);
}

bool
CollectionSortFilterProxyModel::lessThan( const QModelIndex &left, const QModelIndex &right ) const
{
    CollectionTreeItem *leftItem = static_cast<CollectionTreeItem*>( left.internalPointer() );
    CollectionTreeItem *rightItem = static_cast<CollectionTreeItem*>( right.internalPointer() );

    if( leftItem->level() == rightItem->level() )
    {
        const Meta::TrackPtr leftTrack = Meta::TrackPtr::dynamicCast( leftItem->data() );
        const Meta::TrackPtr rightTrack = Meta::TrackPtr::dynamicCast( rightItem->data() );
        if( !leftTrack.isNull() && !rightTrack.isNull() )
        {

            //First compare by disc number
            if ( leftTrack->discNumber() < rightTrack->discNumber() )
                return true;
            else if( leftTrack->discNumber() == rightTrack->discNumber() )
            { //Disc #'s are equal, compare by track number
                if ( leftTrack->trackNumber() != 0 && rightTrack->trackNumber() != 0 )
                    return leftTrack->trackNumber() < rightTrack->trackNumber();
                else
                    //fallback to name sorting
                    return QSortFilterProxyModel::lessThan( left, right );
            }
            else // Right discNum > left discNum
                return false;
        }
    }
    return QSortFilterProxyModel::lessThan( left, right ); //Bad idea fallthrough
}


