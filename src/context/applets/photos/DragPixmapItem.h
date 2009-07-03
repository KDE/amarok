/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DRAGPIXMAPITEM_H
#define DRAGPIXMAPITEM_H

#include "amarok_export.h"

#include <QGraphicsPixmapItem>

//forward
class QGraphicsSceneMouseEvent;

/**
* \brief A drag-able QGraphicsPixmapItem
*
* Display a pixmap which is draggable and clickable.
*
* \sa QGraphicsPixmapItem
*
* \author Simon Esneault <simon.esneault@gmail.com>
*/

class AMAROK_EXPORT DragPixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    public:
        DragPixmapItem( QGraphicsItem* parent = 0 );

        void SetClickableUrl( QString );
        
    protected slots:
        /**
        * Reimplement mouse event
        */
        virtual void mousePressEvent( QGraphicsSceneMouseEvent * );
        virtual void mouseMoveEvent( QGraphicsSceneMouseEvent * );
        virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * );

    private:
        QPoint    m_dragPos;
        QString   m_url;
};

#endif // DROPPIXMAPITEM_H
