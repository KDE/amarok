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

namespace Playlist
{
    class GraphicsScene : public QGraphicsScene
    {
        public:
            GraphicsScene( QObject *parent = 0 );
        
        protected:
            virtual void dragLeaveEvent( QGraphicsSceneDragDropEvent *event );
            virtual void dragEnterEvent( QGraphicsSceneDragDropEvent *event );
            virtual void dropEvent( QGraphicsSceneDragDropEvent *event );
    };
}

#endif

