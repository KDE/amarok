/****************************************************************************************
 * Copyright (c) 2008 Andreas Muetzel <andreas.muetzel@gmx.net>                         *
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

#include "AlbumsModel.h"
#include "AlbumsDefs.h"
#include "AlbumItem.h"
#include "AmarokMimeData.h"
#include "core/support/Debug.h"
#include "TrackItem.h"

AlbumsModel::AlbumsModel( QObject *parent )
    : QStandardItemModel( parent )
{
}

QMimeData*
AlbumsModel::mimeData( const QModelIndexList &indices ) const
{
    DEBUG_BLOCK
    if( indices.isEmpty() )
        return 0;

    Meta::TrackList tracks;
    foreach( const QModelIndex &index, indices )
        tracks << tracksForIndex( index );
    tracks = tracks.toSet().toList();

    // http://doc.trolltech.com/4.4/qabstractitemmodel.html#mimeData
    // If the list of indexes is empty, or there are no supported MIME types,
    // 0 is returned rather than a serialized empty list.
    if( tracks.isEmpty() )
        return 0;

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( tracks );
    return mimeData;
}

Meta::TrackList
AlbumsModel::tracksForIndex( const QModelIndex &index ) const
{
    Meta::TrackList tracks;
    if( !index.isValid() )
        return tracks;

    if( hasChildren( index ) )
    {
        for( int i = 0, rows = rowCount( index ); i < rows; ++i )
            tracks << tracksForIndex( index.child( i, 0 ) );
    }
    else if( QStandardItem *item = itemFromIndex( index ) )
    {
        if( item->type() == TrackType )
        {
            TrackItem* trackItem = static_cast<TrackItem*>( item );
            if( trackItem )
                tracks << trackItem->track();
        }
    }
    return tracks;
}

QStringList
AlbumsModel::mimeTypes() const
{
    QStringList types;
    types << AmarokMimeData::TRACK_MIME;
    return types;
}

#include "AlbumsModel.moc"
