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

#include "CollectionSortFilterProxyModel.h"

#include "amarokconfig.h"
#include "CollectionTreeItem.h"
#include "core/support/Debug.h"

#include <KStringHandler>

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
    // setDynamicSortFilter( true );
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
    CollectionTreeItem *leftItem = treeItem( left );
    CollectionTreeItem *rightItem = treeItem( right );

    // various artists and no label items are always at the top
    if( !leftItem || leftItem->isVariousArtistItem() || leftItem->isNoLabelItem() )
        return true;
    if( !rightItem || rightItem->isVariousArtistItem() || rightItem->isNoLabelItem() )
        return false;

    if( leftItem->isTrackItem() && rightItem->isTrackItem() )
        return lessThanTrack( left, right );

    if( leftItem->isAlbumItem() && rightItem->isAlbumItem() )
        return lessThanAlbum( left, right );

    return lessThanIndex( left, right );
}

bool
CollectionSortFilterProxyModel::lessThanTrack( const QModelIndex &left, const QModelIndex &right ) const
{
    const Meta::TrackPtr leftTrack = Meta::TrackPtr::dynamicCast( treeItem(left)->data() );
    const Meta::TrackPtr rightTrack = Meta::TrackPtr::dynamicCast( treeItem(right)->data() );
    if( !leftTrack || !rightTrack )
    {
        DEBUG_BLOCK
        error() << "Should never have compared these two indexes";
        return lessThanIndex( left, right );
    }

    if( AmarokConfig::showTrackNumbers() )
    {
        //First compare by disc number
        if ( leftTrack->discNumber() < rightTrack->discNumber() )
            return true;
        if ( leftTrack->discNumber() > rightTrack->discNumber() )
            return false;

        //Disc #'s are equal, compare by track number
        if( leftTrack->trackNumber() < rightTrack->trackNumber() )
            return true;
        if( leftTrack->trackNumber() > rightTrack->trackNumber() )
            return false;
    }

    //fallback to name sorting
    return lessThanIndex( left, right );
}

bool
CollectionSortFilterProxyModel::lessThanAlbum( const QModelIndex &left, const QModelIndex &right ) const
{
    Meta::AlbumPtr leftAlbum = Meta::AlbumPtr::dynamicCast( treeItem(left)->data() );
    Meta::AlbumPtr rightAlbum = Meta::AlbumPtr::dynamicCast( treeItem(right)->data() );

    if( !leftAlbum || !rightAlbum )
    {
        DEBUG_BLOCK
        error() << "Should never have compared these two indexes";
        return lessThanIndex( left, right );
    }

    // compare by year
    if( AmarokConfig::showYears() )
    {
        int leftYear = albumYear( leftAlbum );
        int rightYear = albumYear( rightAlbum );

        if( leftYear < rightYear )
            return false; // left album is newer
        if( leftYear > rightYear )
            return true;
    }

    return lessThanIndex( left, right );
}

inline CollectionTreeItem*
CollectionSortFilterProxyModel::treeItem( const QModelIndex &index ) const
{
    return static_cast<CollectionTreeItem*>( index.internalPointer() );
}

int
CollectionSortFilterProxyModel::albumYear( Meta::AlbumPtr album ) const
{
    if( album->name().isEmpty() ) // an unnamed album has no year
        return 0;

    Meta::TrackList tracks = album->tracks();
    if( !tracks.isEmpty() )
    {
        Meta::YearPtr year = tracks.first()->year();
        if( year && (year->year() != 0) )
            return year->year();
    }
    return 0;
}

bool
CollectionSortFilterProxyModel::lessThanIndex( const QModelIndex &left, const QModelIndex &right ) const
{
    // This should catch everything else
    QVariant leftData = left.data( CustomRoles::SortRole );
    QVariant rightData = right.data( CustomRoles::SortRole );

    if( leftData.canConvert( QVariant::String ) && rightData.canConvert( QVariant::String ) )
        return lessThanString( leftData.toString(), rightData.toString() );

    warning() << "failed: an unexpected comparison was made between"<<left.data(Qt::DisplayRole)<<"and"<<right.data(Qt::DisplayRole);

    //Just in case
    return QSortFilterProxyModel::lessThan( left, right );
}

bool
CollectionSortFilterProxyModel::lessThanString( const QString &a, const QString &b ) const
{
    return KStringHandler::naturalCompare( a, b, Qt::CaseInsensitive ) < 0;
}
