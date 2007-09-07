/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "PlaylistGraphicsScene.h"
#include "PlaylistGraphicsDropVis.h"
#include "PlaylistModel.h"
#include "TheInstances.h"

#include <QMimeData>

Playlist::GraphicsScene::GraphicsScene( QObject* parent )
    : QGraphicsScene( parent )
{
}

void
Playlist::GraphicsScene::dragLeaveEvent( QGraphicsSceneDragDropEvent *event )
{
    Playlist::DropVis::instance()->hide();
    QGraphicsScene::dragLeaveEvent( event );
}

void
Playlist::GraphicsScene::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
{
    DEBUG_BLOCK
    foreach( QString mime, The::playlistModel()->mimeTypes() )
    {
        if( event->mimeData()->hasFormat( mime ) )
        {
            debug() << "Accepting!";
            event->setAccepted( true );
            Playlist::DropVis::instance()->showDropIndicator();
            break;
        }
    }
}

void
Playlist::GraphicsScene::dropEvent( QGraphicsSceneDragDropEvent *event )
{
    The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, -1, 0, QModelIndex() );
    Playlist::DropVis::instance()->hide();
}

