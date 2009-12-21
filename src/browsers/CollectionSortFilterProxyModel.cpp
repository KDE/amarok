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
    CollectionTreeItem *leftItem = treeItem( left );
    CollectionTreeItem *rightItem = treeItem( right );

    if( leftItem->level() == rightItem->level() )
    {
        if( leftItem->isTrackItem() && rightItem->isTrackItem() )
            return lessThanTrack( left, right );

        if( leftItem->isAlbumItem() && rightItem->isAlbumItem() )
            return lessThanAlbum( left, right );
        
        if( leftItem->isDataItem() && !leftItem->data() ) //left item is a various artists node
            return true;
        
        if( rightItem->isDataItem() && !rightItem->data() ) //rightItem is a various artists node
            return false;
    }

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

    //First compare by disc number
    if ( leftTrack->discNumber() < rightTrack->discNumber() )
        return true;
    else if( leftTrack->discNumber() == rightTrack->discNumber() )
    {
        //Disc #'s are equal, compare by track number
        if( leftTrack->trackNumber() != 0 && rightTrack->trackNumber() != 0 )
            return leftTrack->trackNumber() < rightTrack->trackNumber();
        
        //fallback to name sorting
        return lessThanIndex( left, right );
    }
    // Right discNum > left discNum
    return false;
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

    //First compare by year
    bool ok = true;
    int leftYear = albumYear( leftAlbum, &ok );
    int rightYear = 0;

    if( !AmarokConfig::showYears() )
        ok = false;
    
    if( ok )
        rightYear = albumYear( rightAlbum, &ok );

    // compare if the years are valid numbers
    if( ok && leftYear < rightYear )
        return false; // left album is newer

    // if the year conversions failed or the years are the same, compare by name
    if( !ok || leftYear == rightYear )
        return lessThanIndex( left, right );

    return true; // left album is older
}

inline CollectionTreeItem*
CollectionSortFilterProxyModel::treeItem( const QModelIndex &index ) const
{
    return static_cast<CollectionTreeItem*>( index.internalPointer() );
}

int
CollectionSortFilterProxyModel::albumYear( Meta::AlbumPtr album, bool *ok ) const
{
    int year = 0;
    if( !album->tracks().isEmpty() )
    {
        const Meta::TrackPtr track = album->tracks()[0];
        if( track && track->year() )
            year = track->year()->prettyName().toInt( ok );
    }
    return year;
}

bool
CollectionSortFilterProxyModel::lessThanIndex( const QModelIndex &left, const QModelIndex &right ) const
{
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
