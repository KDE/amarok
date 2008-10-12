/****************************************************************************
 * copyright            : (C) 2008 Andreas Muetzel <andreas.muetzel@gmx.net>*
 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "AlbumsModel.h"
#include <AmarokMimeData.h>
#include "AlbumItem.h"
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
            items << static_cast<QStandardItem*>( index.internalPointer() );
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
    Meta::AlbumList albums;
    foreach( QStandardItem *item, items )
    {
        TrackItem* track = dynamic_cast<TrackItem*>(item);
        AlbumItem* album = dynamic_cast<AlbumItem*>(item);
        if( track )
        {
            tracks << track->track();
            debug() << "Requested mimedata for track" << item->text();
        }
        else if( album )
        {
            tracks << album->album()->tracks();
            debug() << "Requested mimedata for album" << item->text();
        }
        else
            warning() << "Requested mimedata for something else" << item->text();
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


