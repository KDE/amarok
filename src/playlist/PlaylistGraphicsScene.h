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

#ifndef AMAROK_PLAYLISTGRAPHICSSCENE_H
#define AMAROK_PLAYLISTGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>

namespace Playlist
{
    class GraphicsScene : public QGraphicsScene
    {
        Q_OBJECT

        public:
            GraphicsScene( QObject *parent = 0 );

        protected:
            virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );
            virtual void dragLeaveEvent( QGraphicsSceneDragDropEvent *event );
            virtual void dragEnterEvent( QGraphicsSceneDragDropEvent *event );
            virtual void dropEvent( QGraphicsSceneDragDropEvent *event );

        private slots:
            void axisDeleted();

        private:
            // what item shift-click selections are centered arround
            QGraphicsItem* m_selectionAxis;
    };
}

#endif

