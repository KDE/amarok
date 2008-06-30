/***************************************************************************
 * Copyright 2007  Seb Ruiz <ruiz@kde.org>                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU General Public License as          *
 * published by the Free Software Foundation; either version 2 of          *
 * the License or (at your option) version 3 or any later version          *
 * accepted by the membership of KDE e.V. (or its successor approved       *
 * by the membership of KDE e.V.), which shall act as a proxy              *
 * defined in Section 14 of version 3 of the license.                      *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "PlaylistGraphicsScene.h"

#include "Debug.h"
#include "PlaylistDropVis.h"
#include "PlaylistModel.h"

#include <QMimeData>

using namespace Playlist;

GraphicsScene::GraphicsScene( QObject* parent )
    : QGraphicsScene( parent )
{
}

void
GraphicsScene::dragLeaveEvent( QGraphicsSceneDragDropEvent *event )
{
    QGraphicsScene::dragLeaveEvent( event );
}

void
GraphicsScene::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
{
    QGraphicsScene::dragEnterEvent( event );
}

void
GraphicsScene::dropEvent( QGraphicsSceneDragDropEvent *event )
{
    DEBUG_BLOCK
    QGraphicsScene::dropEvent( event );

    if( itemAt( event->pos() ) )
    {
        debug() << "[GS] ignoring";
        event->ignore();
        QGraphicsScene::dropEvent( event );
    }
    else
    {
        debug() << "[GS] accepting drop";
        event->accept();
        The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, -1, 0, QModelIndex() );
        DropVis::instance()->hide();
    }
}

