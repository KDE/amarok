/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "PlaylistGraphicsScene.h"
#include "PlaylistDropVis.h"
#include "PlaylistModel.h"
#include "TheInstances.h"

#include <QMimeData>

using namespace Playlist;

GraphicsScene::GraphicsScene( QObject* parent )
    : QGraphicsScene( parent )
{
}

void
GraphicsScene::dragLeaveEvent( QGraphicsSceneDragDropEvent *event )
{
    DropVis::instance()->hide();
    QGraphicsScene::dragLeaveEvent( event );
}

void
GraphicsScene::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
{
    DEBUG_BLOCK
    foreach( QString mime, The::playlistModel()->mimeTypes() )
    {
        if( event->mimeData()->hasFormat( mime ) )
        {
            debug() << "Accepting!";
            event->accept();
            DropVis::instance()->showDropIndicator();
            break;
        }
    }
    QGraphicsScene::dragEnterEvent( event );
}

void
GraphicsScene::dropEvent( QGraphicsSceneDragDropEvent *event )
{
    DEBUG_BLOCK
    if( itemAt( event->pos() ) )
    {
        event->ignore();
        QGraphicsScene::dropEvent( event );
    }
    else
    {
        event->accept();
        The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, -1, 0, QModelIndex() );
        DropVis::instance()->hide();
    }
}

