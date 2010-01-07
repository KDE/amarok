/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef CONTAINMENT_SELECTION_LAYER_H
#define CONTAINMENT_SELECTION_LAYER_H

#include "amarok_export.h"

#include <plasma/containment.h>

#include <KIcon>

#include <QGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSimpleTextItem>

/**
 * @class ContainmentSelectionLayer
 * @short A layer to add hover effects in the containments when in zoomed out mode.
 */
class AMAROK_EXPORT ContainmentSelectionLayer: public QObject, public QGraphicsItem
{
    Q_OBJECT
    public:
        ContainmentSelectionLayer( QGraphicsItem *parent = 0 );
        QRectF boundingRect() const;
        
    protected:
        virtual void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
        virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );     
        virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );
        virtual void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
                
    private:
        Plasma::Containment *m_containment;
        bool m_mouseHover;
        QGraphicsSimpleTextItem *m_zoomInText;
        KIcon *m_zoomInIcon;
    Q_SIGNALS:
        void zoomRequested( Plasma::Containment *containment, Plasma::ZoomDirection direction );

};

#endif
