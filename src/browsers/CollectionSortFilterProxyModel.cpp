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

#include "CollectionSortFilterProxyModel.h"

#include "amarokconfig.h"
#include "browsers/CollectionTreeItem.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "widgets/PrettyTreeRoles.h"

#include <QCollator>
#include <QString>

CollectionSortFilterProxyModel::CollectionSortFilterProxyModel(  QObject * parent )
    : QSortFilterProxyModel( parent )
    , m_col( new QCollator )
{
    setSortLocaleAware( true );

    setSortRole( PrettyTreeRoles::SortRole );
    setFilterRole( PrettyTreeRoles::FilterRole );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setFilterCaseSensitivity( Qt::CaseInsensitive );

    setDynamicSortFilter( true );

    m_col->setCaseSensitivity( Qt::CaseInsensitive );
}


CollectionSortFilterProxyModel::~CollectionSortFilterProxyModel()
{
    delete m_col;
}

bool
CollectionSortFilterProxyModel::hasChildren(const QModelIndex & parent) const
{
    QModelIndex sourceParent = mapToSource(parent);
    return sourceModel()->hasChildren(sourceParent);
}

bool
CollectionSortFilterProxyModel::filterAcceptsRow( int source_row, const QModelIndex& source_parent ) const
{
    bool stringAccepted = QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent );

    if( AmarokConfig::showYears())
    {
        QModelIndex index = sourceModel()->index( source_row, 0, source_parent );
        if( treeItem( index )->isAlbumItem() )
        {
            bool yearLoaded = index.data( PrettyTreeRoles::YearRole ).toInt() >= 0;
            return yearLoaded && stringAccepted;
        }
    }

    return stringAccepted;
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

    if( leftItem->isDataItem() && rightItem->isDataItem() )
        return lessThanItem( left, right );

    return QSortFilterProxyModel::lessThan( left, right );
}

bool
CollectionSortFilterProxyModel::lessThanTrack( const QModelIndex &left, const QModelIndex &right ) const
{
    const Meta::TrackPtr leftTrack = Meta::TrackPtr::dynamicCast( treeItem(left)->data() );
    const Meta::TrackPtr rightTrack = Meta::TrackPtr::dynamicCast( treeItem(right)->data() );
    if( !leftTrack || !rightTrack )
    {
        DEBUG_BLOCK
        error() << "Should never have compared these two indexes"
            << left.data(Qt::DisplayRole) << "and" << right.data(Qt::DisplayRole);
        return QSortFilterProxyModel::lessThan( left, right );
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

    // compare by name
    {
        int comp = m_col->compare( leftTrack->sortableName(), rightTrack->sortableName() );
        if( comp < 0 )
            return true;
        if( comp > 0 )
            return false;
    }

    return leftTrack.data() < rightTrack.data(); // prevent expanded tracks from switching places (if that ever happens)
}

bool
CollectionSortFilterProxyModel::lessThanAlbum( const QModelIndex &left, const QModelIndex &right ) const
{
    Meta::AlbumPtr leftAlbum = Meta::AlbumPtr::dynamicCast( treeItem(left)->data() );
    Meta::AlbumPtr rightAlbum = Meta::AlbumPtr::dynamicCast( treeItem(right)->data() );

    if( !leftAlbum || !rightAlbum )
    {
        DEBUG_BLOCK
        error() << "Should never have compared these two indexes"
            << left.data(Qt::DisplayRole) << "and" << right.data(Qt::DisplayRole);
        return QSortFilterProxyModel::lessThan( left, right );
    }

    // compare by year
    if( AmarokConfig::showYears() )
    {
        int leftYear = left.data( PrettyTreeRoles::YearRole ).toInt();
        int rightYear = right.data( PrettyTreeRoles::YearRole ).toInt();

        if( leftYear < rightYear )
            return false; // left album is newer
        if( leftYear > rightYear )
            return true;
    }

    // compare by name
    {
        int comp = m_col->compare( leftAlbum->sortableName(), rightAlbum->sortableName() );
        if( comp < 0 )
            return true;
        if( comp > 0 )
            return false;
    }

    return leftAlbum.data() < rightAlbum.data(); // prevent expanded albums from switching places
}

bool
CollectionSortFilterProxyModel::lessThanItem( const QModelIndex &left, const QModelIndex &right ) const
{
    Meta::DataPtr leftData = treeItem(left)->data();
    Meta::DataPtr rightData = treeItem(right)->data();

    if( !leftData || !rightData )
    {
        DEBUG_BLOCK
        error() << "Should never have compared these two indexes"
            << left.data(Qt::DisplayRole) << "and" << right.data(Qt::DisplayRole);
        return QSortFilterProxyModel::lessThan( left, right );
    }

    // compare by name
    {
        int comp = m_col->compare( leftData->sortableName(), rightData->sortableName() );
        if( comp < 0 )
            return true;
        if( comp > 0 )
            return false;
    }

    return leftData.data() < rightData.data(); // prevent expanded data from switching places
}

inline CollectionTreeItem*
CollectionSortFilterProxyModel::treeItem( const QModelIndex &index ) const
{
    return static_cast<CollectionTreeItem*>( index.internalPointer() );
}
