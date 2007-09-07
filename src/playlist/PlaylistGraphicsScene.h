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
            virtual void dragLeaveEvent( QGraphicsSceneDragDropEvent * )
            {
                Playlist::DropVis::instance()->hide();
            }
    };
}

#endif

