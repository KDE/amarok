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
#include "Debug.h"

#include <QVariant>
#include <QString>

CollectionSortFilterProxyModel::CollectionSortFilterProxyModel(  QObject * parent )
    : QSortFilterProxyModel( parent )
{
    setSortLocaleAware( true );
    setSortCaseSensitivity( Qt::CaseInsensitive );

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

    //Here we catch the bottom 'track' level
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
            {
                //Disc #'s are equal, compare by track number
                if( leftTrack->trackNumber() != 0 && rightTrack->trackNumber() != 0 )
                    return leftTrack->trackNumber() < rightTrack->trackNumber();
                //fallback to name sorting
                return QSortFilterProxyModel::lessThan( left, right );
            }
            else // Right discNum > left discNum
                return false;
        }
    }

    // This should catch everything else
    QVariant leftData = left.data( CustomRoles::SortRole );
    QVariant rightData = right.data( CustomRoles::SortRole );
    if( leftData.canConvert( QVariant::String ) && rightData.canConvert( QVariant::String ) )
        return lessThanString( leftData.toString().toLower(), rightData.toString().toLower() );
   
    warning() << "failed: an unexpected comparison was made";
    
    //Just in case
    return QSortFilterProxyModel::lessThan( left, right );
}

// This method tries to do a smart comparison where a lexographical sort might not be the
// most intelligent sort method. For example the following output would be sorted natually:
//   Symphony 1         Symphony 1
//   Symphony 10        Symphony 2
//   Symphony 11  -->   Symphony 10
//   Symphony 2         Symphony 11
//   Symphony 21        Symphony 21
bool
CollectionSortFilterProxyModel::lessThanString( const QString &a, const QString &b ) const
{
    int compareIndices[2];
    compareIndices[0] = a.indexOf( QRegExp("\\d") );

    if ( compareIndices[0] == -1 || ( compareIndices[1] = b.indexOf(QRegExp("\\d")) ) == -1
        || compareIndices[0] != compareIndices[1] )
        return QString::localeAwareCompare( a, b ) < 0;

    QString toCompare[2];
    int  intToCompare[2];
    toCompare[0] = a.left( compareIndices[0] );
    toCompare[1] = b.left( compareIndices[1] );

    int rv = QString::localeAwareCompare( toCompare[0], toCompare[1] );
    if( rv != 0 )
        return rv < 0;

    toCompare[0] = a.mid( compareIndices[0] );
    toCompare[1] = b.mid( compareIndices[1] );
    for( int i = 0; i < 2; ++i )
    {
        compareIndices[i] = toCompare[i].indexOf( QRegExp("\\D") );
        if( compareIndices[i] == -1 )
            compareIndices[i] = toCompare[i].length();

        intToCompare[i] = toCompare[i].left( compareIndices[i] ).toInt();
    }

    rv = intToCompare[0] - intToCompare[1];
    if( rv != 0 )
        return rv < 0;

    for( int i = 0; i < 2; ++i )
        toCompare[i] = toCompare[i].mid( compareIndices[i] );

    return CollectionSortFilterProxyModel::lessThanString( toCompare[0], toCompare[1] );
}
