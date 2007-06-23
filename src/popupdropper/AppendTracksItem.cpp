/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "AppendTracksItem.h"

#include "AmarokMimeData.h"
#include "debug.h"
#include "meta/meta.h"
#include "playlist/PlaylistModel.h"
#include "TheInstances.h"

using namespace PopupDropperNS;

AppendTracksItem::AppendTracksItem( int whoami, int total, QGraphicsItem *parent )
    : PopupDropperBaseItem( whoami, total, parent )
{
    setAcceptDrops( true );
}

AppendTracksItem::~AppendTracksItem()
{
    //nothing to do
}

void
AppendTracksItem::dropEvent( QGraphicsSceneDragDropEvent *event )
{
    DEBUG_BLOCK
    if( event->mimeData()->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        const AmarokMimeData *mimeData = dynamic_cast<const AmarokMimeData*>( event->mimeData() );
        if( mimeData )
        {
            Meta::TrackList tracks = mimeData->tracks();
            The::playlistModel()->insertOptioned( tracks, PlaylistNS::Append );
        }
    }
}

#include "AppendTracksItem.moc"
