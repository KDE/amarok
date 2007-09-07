/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTGRAPHICSSCENE_H
#define AMAROK_PLAYLISTGRAPHICSSCENE_H

#include "debug.h"
#include "PlaylistGraphicsDropVis.h"
#include "PlaylistModel.h"
#include "TheInstances.h"

#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <QObject>

namespace Playlist
{
    class GraphicsScene : public QGraphicsScene
    {
        public:
            GraphicsScene( QObject *parent = 0 )
                : QGraphicsScene( parent )
            {
            }
        protected:
            virtual void dragLeaveEvent( QGraphicsSceneDragDropEvent *event )
            {
                Playlist::DropVis::instance()->hide();
                QGraphicsScene::dragLeaveEvent( event );
            }

            virtual void dragEnterEvent( QGraphicsSceneDragDropEvent *event )
            {
                foreach( QString mime, The::playlistModel()->mimeTypes() )
                {
                    if( event->mimeData()->hasFormat( mime ) )
                    {
                        event->accept();
                        Playlist::DropVis::instance()->showDropIndicator();
                        break;
                    }
                }
            }

            virtual void dropEvent( QGraphicsSceneDragDropEvent *event )
            {
                The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, -1, 0, QModelIndex() );
                Playlist::DropVis::instance()->hide();
            }
    };
}

#endif

