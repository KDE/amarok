/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
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
#ifndef AMAROK_POPUPDROPPER_BASEITEM_H
#define AMAROK_POPUPDROPPER_BASEITEM_H

#include "amarok_export.h"

#include <QGraphicsItem>
#include <QObject>
#include <QtGlobal>

class QGraphicsSceneDropDropEvent;
class QPainter;
class QRectF;

/**
  * This class contructs the PopupDropperBaseItem
  * @author Jeff Mitchell <kde-dev@emailgoeshere.com>
  */

namespace PopupDropperNS {

    class PopupDropperBaseItem : public QObject, public QGraphicsItem
    {
        
        Q_OBJECT

        public:
    
            /**
            * Creates a new PopupDropperBaseItem.
            * 
            */
            PopupDropperBaseItem( int whichami, int total, QGraphicsItem* parent = 0 );
            ~PopupDropperBaseItem();

            qreal scaledPercent() const { return m_scaledPercent; }
            void setScaledPercent( qreal pct ) { m_scaledPercent = pct; }

            //Reimplementations
            QRectF boundingRect() const;
            void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );
            void dropEvent( QGraphicsSceneDragDropEvent *e );

            qreal       m_scaledPercent;
            int         m_whichami;
            int         m_totalEntries;
    };
}
#endif /* AMAROK_POPUPDROPPER_BASEITEM_H */

