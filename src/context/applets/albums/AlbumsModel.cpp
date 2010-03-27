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
#include <AmarokMimeData.h>
#include "AlbumItem.h"
#include "core/support/Debug.h"
#include "TrackItem.h"

QMimeData*
AlbumsModel::mimeData(const QModelIndexList & indices) const
{
    DEBUG_BLOCK
    if ( indices.isEmpty() )
        return 0;

    QList<QStandardItem*> items;

    foreach( const QModelIndex &index, indices )
    {
        if ( index.isValid() )
        {
            items << itemFromIndex(index);
        }
    }

    return mimeData( items );
}

QMimeData*
AlbumsModel::mimeData(const QList<QStandardItem*> & items) const
{
    DEBUG_BLOCK
    if ( items.isEmpty() )
        return 0;

    Meta::TrackList tracks;

    foreach( QStandardItem *item, items )
    {
        AlbumItem* album = dynamic_cast<AlbumItem*>( item );
        if( album )
        {
            tracks << album->album()->tracks();
            debug() << "Requested mimedata for album" << item->text();
        }
    }
    foreach( QStandardItem *item, items )
    {
        TrackItem* track = dynamic_cast<TrackItem*>( item );
        if( track && !tracks.contains( track->track() ) )
        {
            tracks << track->track();
            debug() << "Requested mimedata for track" << item->text();
        }
    }

    // http://doc.trolltech.com/4.4/qabstractitemmodel.html#mimeData
    // If the list of indexes is empty, or there are no supported MIME types, 
    // 0 is returned rather than a serialized empty list.
    if( tracks.isEmpty() )
        return 0;

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( tracks );
    return mimeData;
}

QStringList
AlbumsModel::mimeTypes() const
{
    QStringList types;
    types << AmarokMimeData::TRACK_MIME;
    return types;
}


